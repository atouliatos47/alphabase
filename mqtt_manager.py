# mqtt_manager.py
import paho.mqtt.client as mqtt
import json
import time
import threading
import asyncio
from datetime import datetime
from sqlalchemy.orm import Session

from models import DataDB, SessionLocal
from websocket_manager import manager

class MQTTManager:
    def __init__(self):
        self.client = mqtt.Client()
        self.setup_callbacks()
        
    def setup_callbacks(self):
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"✅ MQTT Connected to broker")
            # Subscribe to ESP32 topics
            client.subscribe("alphabase/sensors/#")
            client.subscribe("alphabase/status/#")
            client.subscribe("alphabase/commands/#")
            print("📡 Subscribed to ESP32 MQTT topics:")
            print("   - alphabase/sensors/#")
            print("   - alphabase/status/#")
            print("   - alphabase/commands/#")
        else:
            print(f"❌ MQTT Connection failed with code: {rc}")
        
    def on_message(self, client, userdata, msg):
        try:
            print(f"📨 MQTT -> AlphaBase: {msg.topic}")
            
            payload = json.loads(msg.payload.decode())
            print(f"   Data: {payload}")
            
            # Store directly in AlphaBase database
            self.store_mqtt_data(msg.topic, payload)
            
        except json.JSONDecodeError as e:
            print(f"❌ Failed to parse JSON: {e}")
        except Exception as e:
            print(f"❌ MQTT processing error: {e}")
    
    def store_mqtt_data(self, topic, payload):
        db = SessionLocal()
        try:
            if "sensors" in topic:
                collection = "sensors"
                device_id = payload.get("device_id", "unknown")
                key = f"{device_id}_{int(time.time())}"
                
            elif "status" in topic:
                collection = "devices" 
                device_id = payload.get("device_id", "unknown")
                key = device_id
                
            else:
                # For commands or other topics, just log them
                print(f"💡 MQTT Command/Other: {topic} - {payload}")
                return
                
            # Create data ID
            data_id = f"{collection}:{key}"
            
            # Check if exists
            existing = db.query(DataDB).filter(DataDB.id == data_id).first()
            
            if existing:
                existing.value = json.dumps(payload)
                existing.owner = "mqtt_bridge"
            else:
                new_data = DataDB(
                    id=data_id,
                    collection=collection,
                    key=key,
                    value=json.dumps(payload),
                    owner="mqtt_bridge",  # Special owner for MQTT data
                    created_at=datetime.utcnow()
                )
                db.add(new_data)
            
            db.commit()
            
            # Broadcast real-time update via WebSocket (FIXED)
            try:
                loop = asyncio.get_event_loop()
                if loop.is_running():
                    asyncio.run_coroutine_threadsafe(
                        manager.broadcast(json.dumps({
                            "action": "update",
                            "collection": collection,
                            "key": key,
                            "source": "mqtt"
                        })),
                        loop
                    )
            except Exception as e:
                print(f"⚠️  Could not broadcast via WebSocket: {e}")
            
            print(f"✅ MQTT data stored: {collection}/{key}")
            
        except Exception as e:
            print(f"❌ Database error: {e}")
            db.rollback()
        finally:
            db.close()
    
    def start(self):
        def run_mqtt():
            try:
                # FIXED: Use your PC's IP instead of localhost
                self.client.connect("192.168.0.52", 1883, 60)
                print("🚀 MQTT client starting...")
                self.client.loop_forever()
            except Exception as e:
                print(f"❌ MQTT connection failed: {e}")
                print("💡 Make sure Mosquitto is running: mosquitto -c mosquitto.conf -v")
        
        # Start MQTT in background thread
        mqtt_thread = threading.Thread(target=run_mqtt)
        mqtt_thread.daemon = True
        mqtt_thread.start()

# Create global instance
mqtt_manager = MQTTManager()