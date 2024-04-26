
const formatSourceCodeLocationInfo = (moduleName, lines) => {
    return (
        lines.map((line) => {
            return `<span>${moduleName} : line ${line}</span><br>`
        })
    )
}

const formatIrInstructions = (ir) => {
    // Correctly split the string by actual new lines
    let instructions = ir.split("\\n");
    console.log(instructions); // Good for debugging

    // Fix the missing quote for the class attribute
    return `<pre class="language-llvm"><code>${instructions.map((inst) => {
        return `${inst}`;
    }).join('\n')}</code></pre>`; // Join the instructions back with a newline
}


const openBlockDetail = (node) => {
    window.scrollTo(0, 0)
    $("div#block-info").show()
    console.log(node)
    let dataNodeId = `data_${$(node).attr("id")}`
    let dataNode = $(`#${dataNodeId}`)
    $("div#block-ir").html(formatIrInstructions($(dataNode).attr("ir")))
    let lines = $(dataNode).attr("lines").split(",")
    let moduleName = $(dataNode).attr("modulename")
    let blockName = $(node).find("text").text()
    $("div#location").html(formatSourceCodeLocationInfo(moduleName, lines))
    $("div#num-instructions").html(`Number of LLVM IR instructions: ${$(dataNode).attr("blockinstructioncount")}`)
    $("div#num-executions").html(`Number of block executions: ${$(dataNode).attr("executioncount")}`)
    $("svg").hide()
    console.log($("h1#pagetitle"))
    $("h1#pagetitle").hide()
    Prism.highlightAll();

    $("button#goto-src").on("click", () => {
        window.location.href = moduleName + `.html?lines=${lines}&blockname=${blockName}#premodulecode.${lines[0]}`
    })
}


const setupBlockDetailDiv = () => {
    $("div#block-info #back-button").on("click", () => {
        $("div#block-info").hide()
        $("svg").show()
        $("h1#pagetitle").show()
    })
}


const insertViewLLVMIRButton = () => {
    var patternNodes = /^node(\d+)$/;
    var patternNodesData = /^data_node(\d+)$/;

    var nodes = {};
    var dataNodes = {};

    $('[id]').filter(function() {
        return patternNodes.test(this.id) || patternNodesData.test(this.id);
    }).each(function() {
        const id = this.id;
        let match = patternNodes.exec(id);
        if (match) {
            nodes[match[1]] = this;
        } 
        else {
            match = patternNodesData.exec(id);
            if (match) {
                dataNodes[match[1]] = this;
            }
        }
    });

    Object.keys(nodes).forEach(number => {
        if (dataNodes[number]) {
            let link = $(nodes[number]).find(`#a_node${number}`).find("a")
            text = $(link).find("text")
            $(link).attr({
                class: "block-polygon"
            })
            $(text).attr({
                class: "block-text"
            })
            let blockPolygon = $(nodes[number]).find("polygon")
            $(blockPolygon).on("click", () => {openBlockDetail($(blockPolygon.parents("g.node")))})
            $(blockPolygon).removeAttr("fill")
            $(blockPolygon).attr({
                class: "block-polygon"
            })
        }
    });
}

const getColor = (t) => {
    var r = Math.round(255 * (1 - Math.exp(-10 * t)));
    var g = Math.round((1 - t) * 255);
    var b = 0;
    return `rgb(${r}, ${g}, ${b})`;
};


const colorBlocksBasedOnExecutionCount = () => {
    let totalFuncInstructionCount = $("div#func_total").attr("functiontotalinstructions")
    console.log(totalFuncInstructionCount)

    var patternNodes = /^node(\d+)$/;
    var patternNodesData = /^data_node(\d+)$/;

    var nodes = {};
    var dataNodes = {};

    $('[id]').filter(function() {
        return patternNodes.test(this.id) || patternNodesData.test(this.id);
    }).each(function() {
        const id = this.id;
        let match = patternNodes.exec(id);
        if (match) {
            nodes[match[1]] = this;
        } 
        else {
            match = patternNodesData.exec(id);
            if (match) {
                dataNodes[match[1]] = this;
            }
        }
    });

    Object.keys(nodes).forEach(number => {
        if (dataNodes[number]) {
            let thisBlockInstructions = $(dataNodes[number]).attr("blockinstructioncount") * $(dataNodes[number]).attr("executioncount")
            let color = getColor(thisBlockInstructions / totalFuncInstructionCount)
            let polygon = $(nodes[number]).find("polygon")
            $(polygon).css("fill", color)
            $(polygon).css("fill-opacity", "0.5")
            $(polygon).css("transition", "fill 0.3s ease;")
            $(polygon).css("cursor", "pointer")
            polygon.hover(
                function() {
                    $(this).remo
                    $(this).addClass('polygon-hovered');
                }, 
                function() {
                    $(this).removeClass('polygon-hovered');
                }
            );
            console.log(polygon)
        }
    });
}


$(document).ready(() => {
    insertViewLLVMIRButton();
    setupBlockDetailDiv();
    colorBlocksBasedOnExecutionCount();
});