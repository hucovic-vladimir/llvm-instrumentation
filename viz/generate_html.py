import graphviz as gv
import json as js
from bs4 import BeautifulSoup
import re
import os
import pathlib
from concurrent.futures import ProcessPoolExecutor
import copy
import sys

def format_instruction_count(num):
    if num < 1000:
        return str(num)
    elif num < 1000000:
        return f'{num / 1000:.2f}K'.replace('.', ',')
    elif num < 1000000000:
        return f'{num / 1000000:.2f}M'.replace('.', ',')
    else:
        return f'{num / 1000000000:.2f}B'.replace('.', ',')

def process_json_files(blocks_dir):
    grouped_by_modules = {}

    # Walk through all directories and files in blocks_dir
    for dirpath, dirnames, filenames in os.walk(blocks_dir):
        for filename in filenames:
            # Check if the file is a JSON file
            if filename.endswith(".json"):
                full_path = os.path.join(dirpath, filename)
                
                # Open and process each JSON file
                with open(full_path, 'r') as file:
                    data = js.load(file)
                    blocks = data["blocks"]
                    grouped_by_function = {}
                    
                    for block in blocks:
                        function_name = block["function"]
                        if function_name not in grouped_by_function:
                            grouped_by_function[function_name] = []
                        del block["function"]
                        grouped_by_function[function_name].append(block)
                    
                    # Use the file path relative to the blocks_dir as the module key
                    module_name = os.path.relpath(full_path, blocks_dir).replace(".json", "")
                    grouped_by_modules[module_name] = grouped_by_function

    return grouped_by_modules

def process_patterns(patterns, module_blocks):
    for func in patterns:
        for pattern in func["patterns"]:
            type = pattern["type"]
            if(type == "halfDiamond"):
                if(pattern["joinBlock"] in module_blocks):
                    module_blocks[pattern["condBlock"]] = module_blocks[pattern["joinBlock"]]
                else:
                    module_blocks[pattern["condBlock"]] = 0
            elif(type == "diamond"):
                for branch_block in pattern["branchBlocks"]:
                    module_blocks[pattern["condBlock"]] = 0
                    if(branch_block in module_blocks):
                        module_blocks[pattern["condBlock"]] += module_blocks[branch_block]
                    else:
                        module_blocks[branch_block] = 0
                    module_blocks[pattern["joinBlock"]] = 0
                    module_blocks[pattern["joinBlock"]] += module_blocks[branch_block]
            elif(type == "unconditionalJump"):
                module_blocks[pattern["start"]] = module_blocks[pattern["jumpDestination"]]

def render_and_save_graph(blocks_html_template, module_name, func_name, blocks, total_instructions, idx):
    graph_soup = BeautifulSoup(replace_script_and_style_links_in_template(blocks_html_template.replace(".function_name.", func_name), module_name), "html.parser")
    insert_cfg(graph_soup, blocks, total_instructions, func_name, module_name)
    graph_file_name = f"{out_dir}{module_name}_{func_name}.html"
    with open(graph_file_name, "w") as graph_html_file:
        graph_html_file.write(str(graph_soup))
    return f"Processed {graph_file_name}"

def insert_cfg(soup: BeautifulSoup, function_blocks, function_total_instructions, func_name, module_name) -> None:
    print("inserting cfg of " + func_name + " in module " + module_name)
    dot = gv.Digraph(comment='Basic Blocks')
    blocks = function_blocks

    if(len(blocks) > 100):
        body = soup.find("body").string = "The CFG of this function is too complicated and was not rendered."
        return

    blocks2 = {block["id"] : block for block in blocks}

    for i, block in enumerate(blocks):
        node = dot.node(str(block["id"]), 
                        f"{block["name"]}\n\n{format_instruction_count(block["executionCount"] * len(block["ir"]))}",
                            shape="rectangle",
                            width="1.5", 
                            height="1", 
                            fontsize="15", 
                            tooltip=block["name"])

    for block in blocks:
        for successorId in block["successors"]:
            dot.edge(str(block["id"]), str(successorId))

    svg_graph = dot.pipe(format="svg").decode("utf-8")
    soup_svg_graph = BeautifulSoup(svg_graph, features="xml")
    pattern = re.compile(r"^node\d+")
    for i, node in enumerate(soup_svg_graph.find_all("g", id=pattern)):
        title = node.find("title")
        lines = list(str(debug["line"]) for debug in blocks2[int(title.string)]["debug"])
        bb_details = soup.new_tag("div", attrs=
            {
                "id": f"data_{node["id"]}",
                "ir": "\\n".join(blocks2[int(title.string)]["ir"]),
                "style": "display: none",
                "executionCount": blocks2[int(title.string)]["executionCount"],
                "blockInstructionCount": len(blocks2[int(title.string)]["ir"]),
                "modulename" : pathlib.Path(module_name).name,
                "funcname" : func_name,
                "lines": ",".join(lines)
            }
        )
        soup.find("body").append(bb_details)


    body = soup.find("body")
    body.append(soup_svg_graph)
    body.append(soup.new_tag("div", attrs={"functionTotalInstructions": function_total_instructions, "id": "func_total"}))
    print("finished inserting cfg of " + func_name)
    return

def count_dirs_in_path(path):
    p  = pathlib.Path(path)
    dirs = len(p.parts) - (1 if p.is_file() else 0)
    print("PATH: ", path, "DIRS: ", dirs)
    return dirs


def process_func(args):
    blocks_html_template, orig_file_name, func_name, blocks, total_instructions, idx = args
    return render_and_save_graph(blocks_html_template, orig_file_name, func_name, blocks, total_instructions, idx)

def replace_script_and_style_links_in_template(template, module_path):
    resolved_paths = {
        "index.table.js": count_dirs_in_path(module_path) * "../" + "index_table.js",
        "prism.js.script": count_dirs_in_path(module_path) * "../" + "vizlib/prism.js",
        "prism.css.link": count_dirs_in_path(module_path) * "../" + "vizlib/prism.css",
        "code.view.script": count_dirs_in_path(module_path) * "../" + "code_view_script.js",
        "cfg.script": count_dirs_in_path(module_path) * "../" + "cfg_script.js"
    }
    for string, resolved_path in resolved_paths.items():
        template = template.replace(string, resolved_path)
    return template

if __name__ == "__main__":
    patterns_dir = ".patterns/"
    blocks_dir = ".basicblocks/"
    llfiles_dir = ".llfiles/"
    prism_lib_dir = "vizlib/"
    out_dir = "viz/.profile_viz/"

    if(len(sys.argv) == 2):
        profile_file = sys.argv[1]
    else:
        print("Error: no profile specified for visualization.")
        exit()


    absolute_paths = {
        "index.table.js": pathlib.Path("./index_table.js").resolve(),
        "prism.js.script": pathlib.Path(prism_lib_dir + "prism.js").resolve(),
        "prism.css.link": pathlib.Path(prism_lib_dir + "prism.css").resolve(),
        "code.view.script": pathlib.Path("./code_view_script").resolve(),
        "cfg.script": pathlib.Path("./cfg_script.js").resolve()
    }

    index_html_template = open("index_template.html").read()

    functions_html_template = open("functions_template.html").read()

    blocks_html_template = open("blocks_template.html").read()

    code_html_template = open("source_code_template.html").read()

    index_soup = BeautifulSoup(replace_script_and_style_links_in_template(index_html_template, "./"), "html.parser")

    body = index_soup.find("body")

    grouped_by_modules = process_json_files(blocks_dir)

    profile = js.load(open(profile_file, "r"))["modules"]

    profile_grouped_by_module = {}

    for module in profile:
        profile_grouped_by_module[os.path.normpath(module["name"])] = module["basicBlocks"]

    module_block_execution_counts = {}

    for module, info in profile_grouped_by_module.items():
        patterns_json = None
        if(module.replace(".c", ".c.json") in os.listdir(patterns_dir)):
            patterns_file = open(f"{patterns_dir}{module.replace(".c", ".c.json")}")
            patterns_json = js.load(patterns_file)

        code_html_path = pathlib.Path(out_dir + module.replace(".c", ".c.html"))
        code_html_path.parent.mkdir(parents=True, exist_ok=True)
        source_code_soup = BeautifulSoup(replace_script_and_style_links_in_template(code_html_template, module), "html.parser")
        with open(module, "r", encoding="utf-8") as code_file:
            code = code_file.read()
            html_code_element = source_code_soup.find(id="modulecode")
            html_code_element.string = code
            with open(code_html_path, "w", encoding="utf-8") as code_html_file:
                code_html_file.write(str(source_code_soup))

            
        id_map = {}
        id_map = {block["id"] : block["executionCount"] for block in info}
        module_block_execution_counts[module] = id_map
        if(patterns_json):
            process_patterns(patterns_json["functions"], module_block_execution_counts[module])
        

    for module, info in grouped_by_modules.items():
        for function, blocks in info.items():
            for block in blocks:
                if(block["id"] in module_block_execution_counts[module]):
                    block.update({"executionCount":module_block_execution_counts[module][block["id"]]})
                else:
                    block.update({"executionCount": 0})
            


    new_paragraph = index_soup.new_tag('p')
    new_paragraph.string = "List of modules:"
    body.append(new_paragraph)

    table = index_soup.find("table", id="modules")
    ul = index_soup.new_tag("tbody")
    table.append(ul)

    total_instructions_program = 0
    total_instructions_for_modules = {}
    for i, file in enumerate(grouped_by_modules.keys()):
        file = file.replace(".json", "")
        total_instructions_for_modules[file] = {"total": 0}
        total_instructions_for_functions = {}
        for function, blocks in grouped_by_modules[file].items():
            total_instructions_for_functions[function] = 0
            for block in blocks:
                total_instructions_for_functions[function] += block["executionCount"] * len(block["ir"])
            total_instructions_for_modules[file]["total"] += total_instructions_for_functions[function]
            total_instructions_for_modules[file][function] = total_instructions_for_functions[function]
        total_instructions_program += total_instructions_for_modules[file]["total"]

    task_data = []
    for i, file in enumerate(grouped_by_modules.keys()):
        file = file.replace(".json", "")
        tbody = table.find("tbody", id="modules-tbody")
        orig_file_name = file.replace(".json", "")

        for func_name, blocks in grouped_by_modules[orig_file_name].items():
            total_instructions = total_instructions_for_modules[orig_file_name][func_name]
            task_data.append((blocks_html_template, orig_file_name, func_name, blocks, total_instructions, i))

    with ProcessPoolExecutor() as executor:
        results = executor.map(process_func, task_data)
        for result in results:
            print(result)
        
    for i, file in enumerate(grouped_by_modules.keys()):
        tr = index_soup.new_tag("tr")
        td_module_name = index_soup.new_tag("td")
        
        link = index_soup.new_tag("a", href=f"{file.replace('.c', '.c.html')}")
        link.string = f"{file}"
        td_module_name.append(link)
        td_instructions = index_soup.new_tag("td")
        td_instructions.string = str(total_instructions_for_modules[file]["total"])
        td_percent_instructions = index_soup.new_tag("td")
        td_percent_instructions.string = f"{(total_instructions_for_modules[file]["total"] / total_instructions_program * 100):.2f}%"
        tr.append(index_soup.new_tag("td", id="expand-td"))
        tr.append(td_module_name)
        tr.append(td_instructions)
        tr.append(td_percent_instructions)
        tr["data-functions"] = ",".join(grouped_by_modules[file].keys())
        tr["data-instruction-counts"] = ",".join([f"{count}" for count in [total_instructions_for_modules[file][key] for key in grouped_by_modules[file]]])
        try:
            tr["data-ratio"] = ",".join([f"{int(count) / total_instructions_for_modules[file]["total"] * 100:.2f}" for count in tr["data-instruction-counts"].split(",")])
        except:
            tr["data-ratio"] = ",".join(f"{0:.2f}" for _ in grouped_by_modules[file])

        tr["id"] = f"row_{i}"
        tbody.append(tr)

    body.append(table)


    with open(out_dir + "index.html", "w") as index_file:
        index_file.write(str(index_soup))