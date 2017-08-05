virtualenv env
source env/bin/activate
pip install -r requirements.txt
sqlite3 db.sqlite3 < schema.sql
export FLASK_APP=main.py
export FLASK_DEBUG=1
flask run
