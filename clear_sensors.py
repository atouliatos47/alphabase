# clear_sensors.py - Clear all sensor data
from models import SessionLocal, DataDB

db = SessionLocal()

# Delete all sensor data
deleted = db.query(DataDB).filter(DataDB.collection == "sensors").delete()
db.commit()
db.close()

print(f"✅ Deleted {deleted} sensor records!")