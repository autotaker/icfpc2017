/* GLOBALS */
let punterID = -1;
let numPunters = -1;
let initialised = false;

let queuedClaims = [];
let queuedPass = false;

const relayPort = 9998;

/* Graph rendering */

const colours =
      ["#1f77b4",
       "#aec7e8",
       "#ff7f0e",
       "#ffbb78",
       "#2ca02c",
       "#98df8a",
       "#d62728",
       "#ff9896",
       "#9467bd",
       "#c5b0d5",
       "#8c564b",
       "#c49c94",
       "#e377c2",
       "#f7b6d2",
       "#7f7f7f",
       "#c7c7c7",
       "#bcbd22",
       "#dbdb8d",
       "#17becf",
       "#9edae5"];

function getPunterColour(punter) {
    return colours[punter % colours.length];
}

function renderGraph(graph) {
    initCy(graph,
           function() {
               initialised = true;
               cy.autolock(true);
               bindCoreHandlers();
               if (queuedClaims.length > 0 || queuedPass) {
                   playQueuedClaims();
                   ourTurn();
               } else {
                   theirTurn();
               }
           }
          );

    adjacent_graph = buildAdjacentGraph(graph);
    for (let i = 0; i < graph.moves.length; i++) {
        const move = graph.moves[i];
        if (move.claim !== undefined) {
            const claim = move.claim;
            updateEdgeOwner(claim.punter, claim.source, claim.target);
        }
    }

    mine_distance = {}
    for (let i = 0; i < graph.setup.mines.length; i++) {
        mine_distance[graph.setup.mines[i]] = BFS(graph.setup.mines[i]);
    }

    let punter_scores = []
    for (let i = 0; i < graph.punters; i++) {
        let score = 0;
        for (let j = 0; j < graph.setup.mines.length; j++) {
            score += BFSScore(graph.setup.mines[j], i);
        }
        punter_scores.push(score);
    }

    let score_root = document.getElementById("final-score")
    let table = document.createElement('table');
    table.setAttribute('class', 'settings-table');
    for (let i = 0; i < graph.punters; i++) {
        let tr = document.createElement('tr');
        tr.innerHTML = 'Player ' + i + ': ' + punter_scores[i];
        table.appendChild(tr);
    }
    score_root.appendChild(table);
}

function BFS(mine) {
    let queue=[mine];
    let distance = new Array(adjacent_graph.length);
    distance.fill(-1);
    distance[mine] = 0;
    while (queue.length > 0) {
        const v = queue.shift();
        const d = distance[v];
        for (var i = 0; i< adjacent_graph[v].length; i++) {
            const target = adjacent_graph[v][i].target;
            if (distance[target] < 0) {
                distance[target] = d + 1;
                queue.push(target);
            }
        }
    }
    return distance;
}

function BFSScore(mine, punter) {
    let queue=[mine];
    let distance = new Array(adjacent_graph.length);
    let score = 0;
    distance.fill(-1);
    distance[mine] = 0;
    while (queue.length > 0) {
        const v = queue.shift();
        const d = distance[v];
        for (var i = 0; i< adjacent_graph[v].length; i++) {
            const target = adjacent_graph[v][i].target;
            const owner = adjacent_graph[v][i].owned_by;
            if (distance[target] < 0 && owner == punter) {
                distance[target] = d + 1;
                queue.push(target);
                let md = mine_distance[mine][target];
                score += md * md;
            }
        }
    }
    return score;
}


function buildAdjacentGraph(graph) {
    const graph_info = graph.setup;
    
    let adj_graph = new Array(graph_info.sites.length);
    for (let i = 0; i < graph_info.sites.length; i++) {
        adj_graph[graph_info.sites[i].id] = [];
    }

    for (let i = 0; i < graph_info.rivers.length; i++) {
        const river = graph_info.rivers[i];
        adj_graph[river.source].push({"target": river.target, "owned_by": river.owned_by});
        adj_graph[river.target].push({"target": river.source, "owned_by": river.owned_by});
    }
    return adj_graph;
}

function toggleButton(buttonId, st) {
    $("#" + buttonId).attr("disabled", st);
}

function disableButton(buttonId) {
    toggleButton(buttonId, true);
}

function enableButton(buttonId) {
    toggleButton(buttonId, false);
}


$(function() {
    $(document).ready(function() {
        enableButton('connect');
    });
});


function setStatus(status) {
    $("#game-status").text(status);
}

function writeLog(msg) {
    let id = "log";
    let now = new Date(new Date().getTime()).toLocaleTimeString();
    document.getElementById(id).innerHTML += "(" + now + ") " + msg + "\n";
    document.getElementById(id).scrollTop = document.getElementById(id).scrollHeight;
    return;
}

function logMove(move) {
    if (move.claim != undefined) {
        logClaim(move.claim);
    } else if (move.pass != undefined) {
        logPass(move.pass);
    }
}

function send(json) {
    const str = JSON.stringify(json);
    socket.send(str.length + ":" + str);
}

function sendClaim(source, target) {
    const req = {
        claim: {
            punter: punterID,
            source: source,
            target: target
        }
    };

    send(req);
}

function sendPass() {
    const req = {
        pass: { punter: punterID }
    };

    send(req);
}

/* EVENT HANDLING LOGIC */

function handleEdgeClick(edge) {
    const source = edge.data("source");
    const target = edge.data("target");

    console.log("edge data; " + edge.data("owner"));
    if (edge.data("owner") == undefined) {
        sendClaim(parseInt(source), parseInt(target));
        cy.edges().unselect();
        updateEdgeOwner(punterID, source, target);
        theirTurn();
    } else {
        logError("That edge is already claimed! (" + source + " -- " + target + ")");
    }
}

function handlePass() {
    sendPass();
    writeLog("Passed!");
    theirTurn();
}

function bindCoreHandlers() {
    cy.edges().on("mouseover", function(evt) {
        this.style("content", this.data("owner"));
    });
    cy.edges().on("mouseout", function(evt) {
        this.style("content", "");
    });
}

function bindOurTurnHandlers() {
    cy.edges().off("select");
    cy.edges().on("select", function(evt) { handleEdgeClick(this) } );
    $("#pass-button").removeAttr("disabled");
}

function bindTheirTurnHandlers() {
    cy.edges().off("select");
    cy.edges().on("select", function(evt) {
        cy.edges().unselect();
    } );
    $("#pass-button").attr("disabled", true);
}

function ourTurn() {
    bindOurTurnHandlers();
    setStatus("Your move!");
}

function theirTurn() {
    bindTheirTurnHandlers();
    setStatus("Waiting for others to make a move...");
}


/* GAME UPDATE LOGIC */

function updateEdgeOwner(punter, source, target) {
    const es = cy.edges("[source=\"" + source + "\"][target=\"" + target + "\"]");
    if (es.length > 0) {
        const e = es[0];
        e.data()["owner"] = punter;
        e.style("line-color", getPunterColour(punter));
    } else {
        logError("Trying to update nonexistent edge! (" + source + " -- " + target + ")");
    }
}

function printFinalScores(scores) {
    logInfo("Game finished!");
    for (let i = 0; i < scores.length; i++) {
        logScore(scores[i].punter, scores[i].score);
    }
}

function handleIncomingMoves(moves) {
    for (let i = 0; i < moves.length; i++) {
        handleIncomingMove(moves[i]);
    }

    if (initialised) {
        ourTurn();
    }
}

function handleIncomingMove(move) {
    logMove(move);
    if (move.claim !== undefined) {
        const claim = move.claim;
        if (initialised) {
            updateEdgeOwner(claim.punter, claim.source, claim.target);
        } else {
            queueClaim(claim);
        }
    } else if (move.pass !== undefined) {
        if (!initialised) {
            queuedPass = true;
        }
    }
}

function queueClaim(claim) {
    queuedClaims.push(claim);
}

function playQueuedClaims() {
    for (let i = 0; i < queuedClaims.length; i++) {
        const claim = queuedClaims[i];
        updateEdgeOwner(claim.punter, claim.source, claim.target);
    }
    queuedClaims = [];
    queuedPass = false;
    ourTurn();
}

function upload(files) {
    const file = files[0]
    const reader = new FileReader();
    reader.onload = function(ev){
        renderGraph(JSON.parse(reader.result));
    }
    reader.readAsText(file);
}
