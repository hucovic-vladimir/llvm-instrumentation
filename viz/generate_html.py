import graphviz as gv
import json as js
from bs4 import BeautifulSoup
import re
import os

patterns_dir = ".patterns/"
blocks_dir = ".basicblocks/"
llfiles_dir = ".llfiles/"

def insert_cfg(soup: BeautifulSoup, function_blocks, function_total_instructions, func_name, module_name) -> None:
    #print(function_total_instructions)
    dot = gv.Digraph(comment='Basic Blocks')
    blocks = function_blocks

    blocks2 = {block["id"] : block for block in blocks}


    #print(blocks2)
    for i, block in enumerate(blocks):
        node = dot.node(str(block["id"]), block["name"], shape="rectangle", width="1.5", height="1", fontsize="15", tooltip=block["name"])

    for block in blocks:
        for successorId in block["successors"]:
            dot.edge(str(block["id"]), str(successorId))

    svg_graph = dot.pipe("svg").decode("utf-8")
    soup_svg_graph = BeautifulSoup(svg_graph, features="xml")
    pattern = re.compile(r"^node\d+")
    for i, node in enumerate(soup_svg_graph.find_all("g", id=pattern)):
        title = node.find("title")
        lines = list(str(debug["line"]) for debug in blocks2[int(title.string)]["debug"])
        print(title)
        print(blocks2[int(title.string)]["executionCount"])
        bb_details = soup.new_tag("div", attrs=
            {
                "id": f"data_{node["id"]}",
                "ir": "\\n".join(blocks2[int(title.string)]["ir"]),
                "style": "display: none",
                "executionCount": blocks2[int(title.string)]["executionCount"],
                "blockInstructionCount": len(blocks2[int(title.string)]["ir"]),
                "modulename" : module_name,
                "funcname" : function_name,
                "lines": ",".join(lines)
            }
        )
        soup.find("body").append(bb_details)

    body = soup.find("body")
    body.append(soup_svg_graph)
    body.append(soup.new_tag("div", attrs={"functionTotalInstructions": function_total_instructions}))
    return

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

grouped_by_modules = {}
blocks_files = os.listdir(blocks_dir)
for blocks_file in blocks_files:
    with open(blocks_dir + blocks_file, "r") as file:
        json = js.load(file)
        blocks = json["blocks"]
        grouped_by_function = {}
        for block in blocks:
            function_name = block["function"]
            if function_name not in grouped_by_function:
                grouped_by_function[function_name] = []
            del block["function"]
            grouped_by_function[function_name].append(block)
    grouped_by_modules[blocks_file.replace(".json", "")] = grouped_by_function
            

profile = js.load(open(".profiles/profile_data_pid_10576.json", "r"))["modules"]


profile_grouped_by_module = {}

for module in profile:
    profile_grouped_by_module[module["name"]] = module["basicBlocks"]

module_block_execution_counts = {}
for module, info in profile_grouped_by_module.items():
    patterns_json = None
    if(module.replace(".c", ".c.json") in os.listdir(patterns_dir)):
        patterns_file = open(f"{patterns_dir}{module.replace(".c", ".c.json")}")
        patterns_json = js.load(patterns_file)
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
        
functions = js.load(open(patterns_dir + "bio.c.json", "r"))["functions"]

index_html_template = open("index_template.html").read()

functions_html_template = open("functions_template.html").read()

blocks_html_template = open("blocks_template.html").read()

index_soup = BeautifulSoup(index_html_template.replace(".project_name.", "ccsds"), "html.parser")

body = index_soup.find("body")

new_paragraph = index_soup.new_tag('p')
new_paragraph.string = "List of modules:"
body.append(new_paragraph)

table = index_soup.find("table", id="modules")
ul = index_soup.new_tag("tbody")
table.append(ul)

total_instructions_program = 0
total_instructions_for_modules = {}
for i, file in enumerate(blocks_files):
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


for i, file in enumerate(blocks_files):
    file = file.replace(".json", "")
    tbody = table.find("tbody", id="modules-tbody")
    orig_file_name = file.replace(".json", "")
    module_file = BeautifulSoup(functions_html_template.replace(".module_name.", orig_file_name), "html.parser")
    module_body = module_file.find("body")
    ul_functions = module_file.new_tag("ul")

    for func_name, blocks in grouped_by_modules[file].items():
        graph_soup = BeautifulSoup(blocks_html_template.replace(".function_name.", func_name), "html.parser")
        insert_cfg(graph_soup, blocks, total_instructions_for_modules[file][func_name], func_name, file)
        graph_file_name = f"{orig_file_name}_{func_name}.html"
        with open(graph_file_name, "w") as graph_html_file:
            graph_html_file.write(str(graph_soup))
            
        li_func = module_file.new_tag("li")
        link_func = module_file.new_tag("a", href=graph_file_name)
        link_func.string = func_name
        li_func.append(link_func)
        ul_functions.append(li_func)

    
    module_body.append(ul_functions)
    with open(file.replace(".c", ".c.html"), "w") as module_html_file:
        module_html_file.write(str(module_file))

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


with open("index.html", "w") as index_file:
    index_file.write(str(index_soup))