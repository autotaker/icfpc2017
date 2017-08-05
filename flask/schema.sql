CREATE TABLE IF NOT EXISTS battle (id varchar(16), 
                     status varchar(16),
                     created_at TIMESTAMP DEFAULT (DATETIME('now','localtime'))
                    ); 

