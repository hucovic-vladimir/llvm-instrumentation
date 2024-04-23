const navigateToLine = (line) => {
    let regex = /#premodulecode\.\d+/;
    window.location.href = window.location.href.replace(regex, `#premodulecode.${line}`)
    Prism.highlightAll();
}

document.addEventListener('DOMContentLoaded', function() {
    const url = new URL(window.location.href);
    const searchParams = new URLSearchParams(url.search);
    const lines = searchParams.get('lines');
    if(lines) {
        document.getElementById("premodulecode").setAttribute("data-line", lines)
        document.getElementById("lines-span").textContent = "Jump to line: "
    }
    else{
        document.getElementById("lines-links").setAttribute("style", "visibility: hidden")
    }
    if (searchParams.has('blockname')) {
        document.getElementById("blockname").textContent = `Highlighted basic block ${searchParams.get("blockname")}`
    }
    lines.split(",").forEach((line) => {
        let lineLink = document.createElement("button")
        lineLink.setAttribute("onclick", `navigateToLine(${line})`)
        lineLink.textContent = line + "\t"
        document.getElementById("lines-links").appendChild(lineLink)
    })
    Prism.highlightAll();
})