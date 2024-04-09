import graphviz as gv
import json as js
from bs4 import BeautifulSoup
import re
import os

index_html_template = """
<!DOCTYPE html>
<html>
    <head>
        <title>.project_name.</title>
    </head>
    <body>
        <h1>Index<h1>
    </body>
</html>
"""

functions_html_template = """
<!DOCTYPE html>
<html>
    <head>
        <title>.module_name.</title>
    </head>
    <body>
        <h1>Functions in .module_name.</h1>
    </body>
</html>
"""

blocks_html_template = """
<!DOCTYPE html>
<html>
    <head>
        <title>.function_name.</title>
    </head>
    <body>
        <h1>Control Flow Graph of function .function_name.</h1>
    </body>
</html>
"""

def insert_cfg(soup: BeautifulSoup, function_blocks) -> None:
    dot = gv.Digraph(comment='Basic Blocks')
    blocks = function_blocks

    for i, block in enumerate(blocks):
        print(block)
        node = dot.node(str(block["id"]), block["name"], shape="rectangle", width="3", height="2", fontsize="30")

    for block in blocks:
        for successorId in block["successors"]:
            dot.edge(str(block["id"]), str(successorId))

    svg_graph = dot.pipe("svg").decode("utf-8")
    soup_svg_graph = BeautifulSoup(svg_graph, features="xml")
    body = soup.find("body")
    body.append(soup_svg_graph)
    return


grouped_by_modules = {}

blocks_dir = ".basicblocks/"
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
            grouped_by_function[function_name].append(block)
    grouped_by_modules[blocks_file] = grouped_by_function
            
index_soup = BeautifulSoup(index_html_template.replace(".project_name.", "ccsds"), "html.parser")

body = index_soup.find("body")

new_paragraph = index_soup.new_tag('p')
new_paragraph.string = "List of modules:"
body.append(new_paragraph)

# Create and add an unordered list with items
ul = index_soup.new_tag("ul")

for file in blocks_files:
    orig_file_name = file.replace(".json", "")
    module_file = BeautifulSoup(functions_html_template.replace(".module_name.", orig_file_name), "html.parser")
    module_body = module_file.find("body")
    ul_functions = module_file.new_tag("ul")
    for func_name in grouped_by_modules[file].keys():
        blocks = grouped_by_modules[file][func_name]
        graph_soup = BeautifulSoup(blocks_html_template.replace(".function_name.", func_name), "html.parser")
        insert_cfg(graph_soup, blocks)
        graph_file_name = f"{orig_file_name}_{func_name}.html"
        with open(graph_file_name, "w") as graph_html_file:
            graph_html_file.write(str(graph_soup))
            

        li_func = module_file.new_tag("li")
        link_func = module_file.new_tag("a", href=graph_file_name)
        link_func.string = func_name
        li_func.append(link_func)
        ul_functions.append(li_func)
    module_body.append(ul_functions)
    with open(file.replace(".json", ".html"), "w") as module_html_file:
        module_html_file.write(str(module_file))

    li = index_soup.new_tag("li")
    link = index_soup.new_tag("a", href=f"{file.replace(".json", ".html")}")
    link.string = f"{file.replace("json", "")}"
    li.append(link)
    ul.append(li)

body.append(ul)

with open("index.html", "w") as index_file:
    index_file.write(str(index_soup))

        
        
soup = BeautifulSoup(blocks_html_template.replace(".function_name.", "function a()"), features="lxml")

"""with open("test2.html", "r") as html_file:
    soup = BeautifulSoup(html_file, "html.parser")
    svg_graph = BeautifulSoup(svg_graph, features="xml")
    pattern = re.compile(r'^node(\d+)$')
    nodes = svg_graph.find_all(id=pattern)
    nodes = sorted(nodes, key=lambda x: int(pattern.search(x['id']).group(1)))
    for i, rect in enumerate(nodes):
        rect["data-custom"] = f"bio.c-{blocks[i]["function"]}-{blocks[i]["name"]}.ll"

    soup.body.append(svg_graph)
    module_viz = open("bio.c.html", "w+")
    module_viz.write(str(soup))
    
"""