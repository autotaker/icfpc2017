CREATE TABLE IF NOT EXISTS battle (id varchar(16), 
                     status varchar(16),
                     created_at TIMESTAMP DEFAULT (DATETIME('now','localtime'))
                    ); 

CREATE TABLE IF NOT EXISTS map (
    key integer PRIMARY KEY AUTOINCREMENT,
    id varchar(16) UNIQUE,
    size integer,
    tag varchar(16)
    );

CREATE TABLE IF NOT EXISTS AI (
    key integer PRIMARY KEY AUTOINCREMENT,
    commit_id varchar(50),
    name varchar(16),
    small_rating real,
    med_rating real,
    large_rating real);

CREATE TABLE IF NOT EXISTS game (
    key integer PRIMARY KEY AUTOINCREMENT,
    id varchar(16) UNIQUE,
    map_id integer,
    status varchar(16),
    created_at TIMESTAMP DEFAULT (DATETIME('now', 'localtime')));

CREATE TABLE IF NOT EXISTS game_match (
    game_id integer,
    ai_id integer,
    play_order integer);

CREATE TABLE IF NOT EXISTS game_result (
    game_id integer,
    ai_id integer,
    score integer,
    rank integer);

    
