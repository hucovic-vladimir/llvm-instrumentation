
const firstColumnContent = (data, type, row) => {
    return "<button id='downArrowButton' class='button expand-row'><i class='fa-solid fa-caret-right fa'></i></button>"
}

const formatNumberString = (data, type, row) => {
    return Number(data).toLocaleString()
}

const formatChildTable = (rowData, rowId, moduleName) => {
    let rows = []
    for(let i = 0; i < rowData.names.length; i++) {
        rows.push({name: rowData.names[i], instructions: rowData.instructions[i], ratio: rowData.ratios[i]})
    }
    return (
        `
        <table class="child-table display" id="details${rowId}" style="width: 100%">
            <thead>
                <th>Function name</th>
                <th>Number of executed instructions</th>
                <th>% of module</th>
            </thead>
            <tbody>
                ${rows.map(item => { 
                    return(
                        `<tr>
                        <td>
                            <a href="${moduleName + "_" + item.name + ".html"}">${item.name}</a>
                        </td>
                        <td>${item.instructions}</td>
                        <td>${item.ratio}%</td>
                        </tr>`
                    )
                }).join('')}
            </tbody>
        </table>
        `
    )
}

$(document).ready(function() {
    var table = $('#modules').DataTable({
        responsive: true,
        layout: {
            topStart: "search",
            bottomStart: "pageLength",
            topEnd: null
        },
        "columns": [
            { "orderable": false, "width": "2%", "render": firstColumnContent },
            { "orderable": true, "width": "48%"},
            { "orderable": true, "width": "25%", "render": formatNumberString },
            { "orderable": true, "width": "25%" }
        ],
        "order": [[2, "desc"]]
    });

    $("div#program-total-instructions").html("Total number of executed instructions: " + Number($("#program-total-instructions").html()).toLocaleString())

    $("#modules tbody").on('click', 'button.expand-row', function(){
        console.log("clicked")
        let tr = $(this).closest('tr');
        let row = table.row(tr)
        let rowId = tr.attr("id")
        const funcNames = tr.attr('data-functions').split(",")
        const funcInstructions = tr.attr('data-instruction-counts').split(",")
        const funcRatios = tr.attr('data-ratio').split(",")
        link = tr.find("a")

        if (row.child.isShown()) {
            // This row is already open - close it
            row.child.hide();
            tr.removeClass('shown');
            $(this).html("<i class='fa-solid fa-caret-right fa'></i>");
        } else {
            // Open this row
            row.child(formatChildTable({names: funcNames, instructions: funcInstructions, ratios: funcRatios}, rowId, link.html())).show();
            tr.addClass('shown');
            $(this).html("<i class='fa-solid fa-caret-down fa'></i>");
            let rowDetails = "#details" + rowId
            var childTable = $(rowDetails).DataTable({
                responsive: true,
                layout: {
                    topStart: "search",
                    bottomStart: "pageLength",
                    topEnd: null                
                },
                "columns": [
                    { "orderable": true, "width": "50%", },
                    { "orderable": true, "width": "25%" , className: "dt-body-right", render: formatNumberString },
                    { "orderable": true, "width": "25%" },
                ],
                "order": [[2, "desc"]]
            })
        }
    });
});

