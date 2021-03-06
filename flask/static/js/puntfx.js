/* GLOBALS */
let punterID = -1;
let numPunters = -1;
let initialised = false;

let queuedClaims = [];
let queuedPass = false;

let playerNames = {}

function getPlayerName(i) {
    if( i in playerNames ) {
        return playerNames[i];
    }else  {
        return "Player" + i;
    }
}

function setPlayerName(i, s) {
    playerNames[i] = s;
}


const relayPort = 9998;

/* Graph rendering */

const default_color = "#009"

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
           }
          );

    moves = graph.moves;
    num_punters = graph.punters;
    adjacent_graph = buildAdjacentGraph(graph);
    for (let i = 0; i < moves.length; i++) {
        const move = moves[i];
        if (move.claim !== undefined) {
            const claim = move.claim;
            updateEdgeOwner(claim.punter, claim.source, claim.target);
        } else if (move.splurge !== undefined) {
            const splurge = move.splurge;
            for (let j = 0; j+1 < splurge.route.length; ++j) {
                updateEdgeOwner(splurge.punter, splurge.route[j], splurge.route[j + 1]);
            }
        } else if (move.option !== undefined) {
            // TODO
        }
    }


    const punter_scores = moves[moves.length - 1].scores;

    let score_root = document.getElementById("final-score")
    while (score_root.firstChild) {
        score_root.removeChild(score_root.firstChild);
    }

    score_root.appendChild(createScoreTable(punter_scores));

    updateCurrentScores(0);
    createSlider();
    addActionLog();
}

function updateCurrentScores(turn) {
    let scores = new Array(num_punters);
    scores.fill(0);
    if (turn > 0) {
        scores = moves[turn - 1].scores;
    }
    let score_root = document.getElementById("current-score")
    while (score_root.firstChild) {
        score_root.removeChild(score_root.firstChild);
    }
    score_root.appendChild(createScoreTable(scores));
}

function createScoreTable(scores) {
    let table = document.createElement('table');
    table.setAttribute('class', 'settings-table');

    let headers = ["Player", "Score", "Basic", "Futures"];
    let tr = document.createElement('tr');

    headers.forEach(function(val) {
        let th = document.createElement('th');
        th.innerHTML = val;
        tr.appendChild(th);
    });

    table.appendChild(tr);

    let score_names = ["score", "basic_score", "futures_score"];

    for (let i = 0; i < num_punters; i++) {
        let tr = document.createElement('tr');
        let td = document.createElement('td');
        td.innerHTML = '<div style="background-color: ' + colours[i] + '">'
                     + getPlayerName(i)
                     + '</div>';


        tr.appendChild(td);

        score_names.forEach(function(val) {
            let td = document.createElement('td');
            td.innerHTML = scores[i][val] === undefined ? 0 : scores[i][val];
            tr.appendChild(td);
        });

        table.appendChild(tr);
    }
    return table;
}

function addActionLog() {
    let log_table = document.createElement('table');
    log_table.setAttribute('class', 'table');
    let log_header = document.createElement('tr');
    let log_player_turn_header = document.createElement('th');
    log_player_turn_header.innerHTML = '#';
    log_header.appendChild(log_player_turn_header);
    let log_player_header = document.createElement('th');
    log_player_header.innerHTML = 'Player';
    log_header.appendChild(log_player_header);
    let log_action_header = document.createElement('th');
    log_action_header.innerHTML = 'Action';
    log_header.appendChild(log_action_header);
    let log_source_header = document.createElement('th');
    log_source_header.innerHTML = 'Source';
    log_header.appendChild(log_source_header);
    let log_target_header = document.createElement('th');
    log_target_header.innerHTML = 'Target';
    log_header.appendChild(log_target_header);
    log_table.appendChild(log_header);


    for (let i = 0; i < moves.length; i++) {
        let log_line = document.createElement('tr');

        let log_player_turn_body = document.createElement('td');
        log_player_turn_body.innerHTML = i + 1;
        log_line.appendChild(log_player_turn_body);
        if (moves[i].claim !== undefined) {
            const claim = moves[i].claim;
            let log_player_body = document.createElement('td')
            log_player_body.innerHTML = claim.punter;
            log_line.appendChild(log_player_body);
            let log_action_body = document.createElement('td');
            log_action_body.innerHTML = 'Claim'
            log_line.appendChild(log_action_body);
            let log_source_body = document.createElement('td');
            log_source_body.innerHTML = claim.source;
                log_line.appendChild(log_source_body);
            let log_target_body = document.createElement('td');
            log_target_body.innerHTML = claim.target;
            log_line.appendChild(log_target_body);
        } else if (moves[i].splurge !== undefined) {
            const splurge = moves[i].splurge;
            let log_player_body = document.createElement('td')
            log_player_body.innerHTML = splurge.punter;
            log_line.appendChild(log_player_body);
            let log_action_body = document.createElement('td');
            log_action_body.innerHTML = 'Splurge'
            log_line.appendChild(log_action_body);
            let log_path_body = document.createElement('td');
            log_path_body.innerHTML = '';
            for (let j = 0; j < splurge.route.length; j += 3) {
                var to = Math.min(j + 3, splurge.route.length);
                log_path_body.innerHTML += splurge.route.slice(j, to).join(',') + ',<br/>';
            }
            log_path_body.style = 'word-wrap:break-word;';
            log_line.appendChild(log_path_body);
        } else if (moves[i].option !== undefined) {
            const option = moves[i].option;
            let log_player_body = document.createElement('td')
            log_player_body.innerHTML = option.punter;
            log_line.appendChild(log_player_body);
            let log_action_body = document.createElement('td');
            log_action_body.innerHTML = 'Option'
            log_line.appendChild(log_action_body);
            let log_source_body = document.createElement('td');
            log_source_body.innerHTML = option.source;
                log_line.appendChild(log_source_body);
            let log_target_body = document.createElement('td');
            log_target_body.innerHTML = option.target;
            log_line.appendChild(log_target_body);
        } else {
            const pass = moves[i].pass;
            let log_player_body = document.createElement('td')
            log_player_body.innerHTML = pass.punter;
            log_line.appendChild(log_player_body);
            let log_action_body = document.createElement('td');
            log_action_body.innerHTML = 'Pass'
            log_line.appendChild(log_action_body);
        }
        log_table.appendChild(log_line);
    }
    action_log = document.getElementById('action-log');
    while (action_log.firstChild) {
        action_log.removeChild(action_log.firstChild);
    }
    action_log.appendChild(log_table);
}

function createSlider() {
    document.getElementById("turnInputId").setAttribute("max", moves.length);
    document.getElementById("turnInputId").setAttribute("onchange", "updateSlider(value)");
    document.getElementById("turnInputId").value = 0;
    document.getElementById("turnOutputId").value = 0;
    current_value = 0;
    for (let i = 0; i < moves.length; i++) {
        const move = moves[i];
        if (move.claim !== undefined) {
            const claim = move.claim;
            updateEdgeOwner(-1, claim.source, claim.target);
        } else if (move.splurge !== undefined) {
            const splurge = move.splurge;
            for (let j = 0; j+1 < splurge.route.length; ++j) {
                updateEdgeOwner(-1, splurge.route[j], splurge.route[j + 1]);
            }
        } else if (move.option !== undefined) {
          // TODO
        }
    }
}

function updateSlider(value) {
    let i_value = parseInt(value)
    for (let i = current_value; i < i_value; i++) {
        const move = moves[i];
        if (move.claim !== undefined) {
            const claim = move.claim;
            updateEdgeOwner(claim.punter, claim.source, claim.target);
        } else if (move.splurge !== undefined) {
            const splurge = move.splurge;
            for (let j = 0; j+1 < splurge.route.length; ++j) {
                updateEdgeOwner(splurge.punter, splurge.route[j], splurge.route[j + 1]);
            }
        } else if (move.option !== undefined) {
          // TODO
        }
    }

    for (let i = i_value; i < current_value; i++) {
        const move = moves[i];
        if (move.claim !== undefined) {
            const claim = move.claim;
            updateEdgeOwner(-1, claim.source, claim.target);
        } else if (move.splurge !== undefined) {
            const splurge = move.splurge;
            for (let j = 0; j+1 < splurge.route.length; ++j) {
                updateEdgeOwner(-1, splurge.route[j], splurge.route[j + 1]);
            }
        } else if (move.option !== undefined) {
          // TODO
        }
    }
    updateCurrentScores(i_value);
    current_value = i_value;
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
    if (source > target) {
        let tmp = source;
        source = target;
        target = tmp;
    }

    const es = cy.edges("[source=\"" + source + "\"][target=\"" + target + "\"]");
    if (es.length > 0) {
        const e = es[0];
        e.data()["owner"] = punter;
        if (punter >= 0) {
            e.style("line-color", getPunterColour(punter));
        } else {
            e.style("line-color", default_color);
        }
    } else {
        console.error("Trying to update nonexistent edge! (" + source + " -- " + target + ")");
    }
}

function upload(files) {
    const file = files[0]
    const reader = new FileReader();
    reader.onload = function(ev){
        renderGraph(JSON.parse(reader.result));
    }
    reader.readAsText(file);
}
