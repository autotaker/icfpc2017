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
    large_rating real,
    status varchar(16)
    );

CREATE TABLE IF NOT EXISTS game (
    key integer PRIMARY KEY AUTOINCREMENT,
    id varchar(16) UNIQUE,
    map_key integer,
    status varchar(16),
    created_at TIMESTAMP DEFAULT (DATETIME('now', 'localtime')));

CREATE TABLE IF NOT EXISTS game_match (
    game_key integer,
    ai_key integer,
    play_order integer,
    score integer,
    rank integer
    );

