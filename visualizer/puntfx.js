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

const log = {"setup": {"sites": [{"id": 0, "x": 0.0, "y": 0.0}, {"id": 1, "x": 1.0, "y": 0.0}, {"id": 2, "x": 2.0, "y": 0.0}, {"id": 3, "x": 2.0, "y": -1.0}, {"id": 4, "x": 2.0, "y": -2.0}, {"id": 5, "x": 1.0, "y": -2.0}, {"id": 6, "x": 0.0, "y": -2.0}, {"id": 7, "x": 0.0, "y": -1.0}], "rivers": [{"source": 0, "target": 1, "owned_by": 0}, {"source": 1, "target": 2, "owned_by": 0}, {"source": 0, "target": 7, "owned_by": 0}, {"source": 7, "target": 6, "owned_by": 0}, {"source": 6, "target": 5, "owned_by": 0}, {"source": 5, "target": 4, "owned_by": 0}, {"source": 4, "target": 3, "owned_by": 0}, {"source": 3, "target": 2, "owned_by": 0}, {"source": 1, "target": 7, "owned_by": 0}, {"source": 1, "target": 3, "owned_by": 0}, {"source": 7, "target": 5, "owned_by": 0}, {"source": 5, "target": 3, "owned_by": 0}], "mines": [1, 5]}, "punters": 1, "moves": [{"claim": {"punter": 0, "source": 0, "target": 1}}, {"claim": {"punter": 0, "source": 1, "target": 2}}, {"claim": {"punter": 0, "source": 1, "target": 7}}, {"claim": {"punter": 0, "source": 1, "target": 3}}, {"claim": {"punter": 0, "source": 6, "target": 5}}, {"claim": {"punter": 0, "source": 5, "target": 4}}, {"claim": {"punter": 0, "source": 7, "target": 5}}, {"claim": {"punter": 0, "source": 5, "target": 3}}, {"claim": {"punter": 0, "source": 0, "target": 7}}, {"claim": {"punter": 0, "source": 3, "target": 2}}, {"claim": {"punter": 0, "source": 7, "target": 6}}, {"claim": {"punter": 0, "source": 4, "target": 3}}]}

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
    
    for (let i = 0; i < graph.moves.length; i++) {
        const move = graph.moves[i];
        if (move.claim !== undefined) {
            const claim = move.claim;
            updateEdgeOwner(claim.punter, claim.source, claim.target);
        }
    }
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


/**
 * Communication
 */
function _connect() {
    disableButton('connect');
    const gamePort = $('#gamePort').val();
    const punterName = $('#punterName').val();
    renderGraph(log);
    return;
}

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
