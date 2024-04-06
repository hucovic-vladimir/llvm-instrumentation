// Setup SVG container
var svg = d3.select("svg");

var connections = [
    { source: 0, target: 1 },
    { source: 0, target: 2 },
];

// Data for rectangles
var rectangles = [
    { id: 0, x: 350, y: 250, width: 100, height: 50, stroke: 'red', strokeWidth: 2 },
    { id: 1, x: 525, y: 250, width: 100, height: 50, stroke: 'black', strokeWidth: 2 },
    { id: 2, x: 175, y: 250, width: 100, height: 50, stroke: 'black', strokeWidth: 2 }
];

// Create rectangles
svg.selectAll("rect")
    .data(rectangles)
    .enter()
    .append("rect")
    .attr("x", function(d) { return d.x; })
    .attr("y", function(d) { return d.y; })
    .attr("width", function(d) { return d.width; })
    .attr("height", function(d) { return d.height; })
    .style("fill", "none") // No fill color
    .style("stroke", function(d) { return d.stroke; }) // Stroke color
    .style("stroke-width", function(d) { return d.strokeWidth; }); // Stroke width

// Calculate center points and border points of rectangles
var centerPoints = rectangles.map(function(rect) {
    return {
        id: rect.id,
        cx: rect.x + rect.width / 2,
        cy: rect.y + rect.height / 2,
        x1: rect.x, // left border
        x2: rect.x + rect.width, // right border
        y1: rect.y, // top border
        y2: rect.y + rect.height // bottom border
    };
});

// Create lines with arrowheads for connections
var lines = svg.selectAll("line")
    .data(connections)
    .enter()
    .append("line")
    .attr("stroke", "black")
    .attr("stroke-width", 2)
    .attr("marker-end", "url(#arrowhead)"); // Arrowhead marker

// Define the arrowhead marker
svg.append("defs").append("marker")
    .attr("id", "arrowhead")
    .attr("viewBox", "-0 -5 10 10")
    .attr("refX", 5)
    .attr("refY", 0)
    .attr("orient", "auto")
    .attr("markerWidth", 6)
    .attr("markerHeight", 6)
    .attr("xoverflow", "visible")
  .append("svg:path")
    .attr("d", "M 0,-5 L 10 ,0 L 0,5")
    .attr("fill", "black");

// Update lines with calculated path
lines.each(function(d) {
    var sourceRect = centerPoints.find(rect => rect.id === d.source);
    var targetRect = centerPoints.find(rect => rect.id === d.target);
    
    // Find the closest border points
    var sourceX, sourceY, targetX, targetY;
    if (sourceRect.cy < targetRect.cy) {
        // Source is above target
        sourceX = sourceRect.cx;
        sourceY = sourceRect.y2;
        targetX = targetRect.cx;
        targetY = targetRect.y1;
    } else if (sourceRect.cy > targetRect.cy) {
        // Source is below target
        sourceX = sourceRect.cx;
        sourceY = sourceRect.y1;
        targetX = targetRect.cx;
        targetY = targetRect.y2;
    } else {
        // Source and target are on the same level
        if (sourceRect.cx < targetRect.cx) {
            // Source is to the left of target
            sourceX = sourceRect.x2;
            sourceY = sourceRect.cy;
            targetX = targetRect.x1;
            targetY = targetRect.cy;
        } else {
            // Source is to the right of target
            sourceX = sourceRect.x1;
            sourceY = sourceRect.cy;
            targetX = targetRect.x2;
            targetY = targetRect.cy;
        }
    }

    d3.select(this)
        .attr("x1", sourceX)
        .attr("y1", sourceY)
        .attr("x2", targetX)
        .attr("y2", targetY);
});

