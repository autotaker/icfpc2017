if [ ! -e env ] ;  then
    virtualenv env
fi
source env/bin/activate
pip install -r requirements.txt
python register_maps.py
sqlite3 db.sqlite3 < schema.sql
export FLASK_APP=main.py
export FLASK_DEBUG=1
python main.py
#flask run
