import graphviz as gv
import json as js
from bs4 import BeautifulSoup
import re
import os
import pathlib
from concurrent.futures import ProcessPoolExecutor
import sys
import shutil
import argparse

script_dir = pathlib.Path(__file__).parent

def process_arguments():
    parser = argparse.ArgumentParser(description="Process command line options for file and directory paths")

    parser.add_argument('--profile', type=str, required=True, help='File path for the profile')
    parser.add_argument('--basicblocks', type=str, required=True, help='Directory path for basic blocks')
    parser.add_argument('--patterns', type=str, required=True, help='Directory path for patterns')
    parser.add_argument('--outdir', type=str, required=True, help='Output directory path')

    args = parser.parse_args()

    if not os.path.isfile(args.profile):
        raise ValueError(f"The specified profile file does not exist: {args.profile}")
    
    if not os.path.isdir(args.basicblocks):
        raise ValueError(f"The specified directory for basic blocks does not exist: {args.basicblocks}")
    
    if not os.path.isdir(args.patterns):
        raise ValueError(f"The specified directory for patterns does not exist: {args.patterns}")
    
    if not os.path.isdir(args.outdir):
        os.makedirs(args.outdir) 
        print(f"Created output directory at: {args.outdir}")
    else:
        print(f"Output directory already exists at: {args.outdir}")

    return args

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
    for dirpath, dirnames, filenames in os.walk(blocks_dir):
        for filename in filenames:
            if filename.endswith(".json"):
                full_path = os.path.join(dirpath, filename)
                
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
                if(pattern["jumpDestination"] in module_blocks):
                    module_blocks[pattern["start"]] = module_blocks[pattern["jumpDestination"]]
                else:
                    module_blocks[pattern["start"]] = 0
            elif(type == "sumOfExits"):
                module_blocks[pattern["entry"]] = 0
                for exit in pattern["exitBlocks"]:
                    if(exit in module_blocks):
                        module_blocks[pattern["entry"]] += module_blocks[exit]
                    else:
                        module_blocks[exit] = 0


def render_and_save_graph(blocks_html_template, module_name, func_name, blocks, total_instructions, out_dir):
    graph_file_name = pathlib.Path(out_dir / pathlib.Path(f"{module_name}_{func_name}.html"))
    graph_soup = BeautifulSoup(replace_script_and_style_links_in_template(blocks_html_template.replace(".function_name.", func_name), graph_file_name, out_dir), "html.parser")
    print(graph_file_name)
    insert_cfg(graph_soup, blocks, total_instructions, func_name, module_name)
    with open(graph_file_name, "w") as graph_html_file:
        graph_html_file.write(str(graph_soup))
    return f"Processed {graph_file_name}"

def insert_cfg(soup: BeautifulSoup, function_blocks, function_total_instructions, func_name, module_name) -> None:
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
    return

def count_dirs_in_path(path):
    p  = pathlib.Path(path)
    dirs = len(p.parts) - (1 if p.is_file() else 0)
    return dirs


def process_func(args):
    blocks_html_template, orig_file_name, func_name, blocks, total_instructions, out_dir = args
    return render_and_save_graph(blocks_html_template, orig_file_name, func_name, blocks, total_instructions, out_dir)

def replace_script_and_style_links_in_template(template, module_path, out_dir):
    resolved_paths = {
        "index.table.js": ("vizlib/index_table.js"),
        "prism.js.script": (out_dir / pathlib.Path("vizlib/prism.js")).relative_to(pathlib.Path(module_path).resolve().parent) if module_path else "vizlib/prism.js",
        "prism.css.link": (out_dir / pathlib.Path("vizlib/prism.css")).relative_to(pathlib.Path(module_path).resolve().parent) if module_path else "vizlib/prism.css",
        "code.view.script": (out_dir / pathlib.Path("vizlib/code_view_script.js")).relative_to(pathlib.Path(module_path).resolve().parent) if module_path else "vizlib/code_view_script.js",
        "cfg.script": (out_dir / pathlib.Path("vizlib/cfg_script.js")).relative_to(pathlib.Path(module_path).resolve().parent) if module_path else "vizlib/cfg_script.js",
    }

    for string, resolved_path in resolved_paths.items():
        template = template.replace(string, str(resolved_path))
    return template

if __name__ == "__main__":
    args = process_arguments()

    profile_file_path = pathlib.Path(args.profile).resolve()
    patterns_dir = pathlib.Path(args.patterns)
    blocks_dir = pathlib.Path(args.basicblocks)
    lib_dir = script_dir / pathlib.Path("vizlib/")
    out_dir = pathlib.Path(args.outdir).resolve()
    pathlib.Path(out_dir / "vizlib/").mkdir(parents=True, exist_ok=True)

    shutil.copytree(lib_dir, out_dir / "vizlib/", dirs_exist_ok=True)

    index_html_template = open(script_dir / "index_template.html").read()

    functions_html_template = open(script_dir / "functions_template.html").read()

    blocks_html_template = open(script_dir / "blocks_template.html").read()

    code_html_template = open(script_dir / "source_code_template.html").read()

    index_soup = BeautifulSoup(replace_script_and_style_links_in_template(index_html_template, "", out_dir), "html.parser")

    body = index_soup.find("body")

    grouped_by_modules = process_json_files(str(blocks_dir))

    profile = js.load(open(profile_file_path, "r"))["modules"]

    profile_grouped_by_module = {}

    for module in profile:
        profile_grouped_by_module[os.path.normpath(module["name"])] = module["basicBlocks"]

    module_block_execution_counts = {}

    for module, info in profile_grouped_by_module.items():
        patterns_json = None
        json_file = module.replace(".c", ".c.json")
        if(json_file in os.listdir(patterns_dir)):
            patterns_file = open(f"{patterns_dir}/{json_file}")
            patterns_json = js.load(patterns_file)

        code_html_path = pathlib.Path(out_dir / pathlib.Path(module.replace(".c", ".c.html")))
        code_html_path.parent.mkdir(parents=True, exist_ok=True)
        print(module)
        source_code_soup = BeautifulSoup(replace_script_and_style_links_in_template(code_html_template, code_html_path, out_dir), "html.parser")
        with open(profile_file_path.parent / module, "r", encoding="utf-8") as code_file:
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
            task_data.append((blocks_html_template, orig_file_name, func_name, blocks, total_instructions, out_dir))

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
    body.find("div", id="program-total-instructions").string = str(total_instructions_program)


    with open(out_dir / pathlib.Path("index.html"), "w") as index_file:
        index_file.write(str(index_soup))