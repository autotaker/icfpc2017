if [ ! -e env ] ;  then
    virtualenv -p python3 env
fi
source env/bin/activate
pip install -r requirements.txt
sqlite3 db.sqlite3 < schema.sql
python register_maps.py
export FLASK_APP=main.py
export FLASK_DEBUG=1
python main.py
#flask run
