from fastapi import FastAPI, HTTPException, Depends, status, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
from pydantic import BaseModel, EmailStr
from passlib.context import CryptContext
from sqlalchemy import create_engine, Column, String, Text, DateTime
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, Session
import json
from datetime import datetime, timedelta
import jwt
import threading
import paho.mqtt.client as mqtt
import time
import asyncio

# Initialize FastAPI
app = FastAPI(title="AlphaBase", version="3.1.0")

# Enable CORS for PWA apps
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Security Configuration
SECRET_KEY = "alphabase-secret-key-change-in-production"
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 30

pwd_context = CryptContext(schemes=["argon2"], deprecated="auto")
security = HTTPBearer()

# Database Setup
SQLALCHEMY_DATABASE_URL = "sqlite:///./alphabase.db"
engine = create_engine(SQLALCHEMY_DATABASE_URL, connect_args={"check_same_thread": False})
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)
Base = declarative_base()

# Database Models
class UserDB(Base):
    __tablename__ = "users"
    username = Column(String, primary_key=True, index=True)
    email = Column(String, unique=True, index=True)
    password = Column(String)
    created_at = Column(DateTime, default=datetime.utcnow)

class DataDB(Base):
    __tablename__ = "data"
    id = Column(String, primary_key=True)  # collection:key format
    collection = Column(String, index=True)
    key = Column(String)
    value = Column(Text)  # JSON stored as text
    owner = Column(String)
    created_at = Column(DateTime, default=datetime.utcnow)

# Create tables
Base.metadata.create_all(bind=engine)

# WebSocket Connection Manager
class ConnectionManager:
    def __init__(self):
        self.active_connections: list[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)
        print(f"✅ WebSocket connected. Total: {len(self.active_connections)}")

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
            print(f"❌ WebSocket disconnected. Remaining: {len(self.active_connections)}")

    async def broadcast(self, message: str):
        if not self.active_connections:
            return
            
        print(f"📢 Broadcasting to {len(self.active_connections)} clients: {message}")
        disconnected = []
        for connection in self.active_connections:
            try:
                await connection.send_text(message)
            except Exception as e:
                print(f"❌ Failed to send to client: {e}")
                disconnected.append(connection)
        
        # Clean up disconnected clients
        for connection in disconnected:
            self.active_connections.remove(connection)

manager = ConnectionManager()

# MQTT Manager
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
            
            # Broadcast real-time update via WebSocket
            asyncio.run_coroutine_threadsafe(
                manager.broadcast(json.dumps({
                    "action": "update",
                    "collection": collection,
                    "key": key,
                    "source": "mqtt"
                })),
                asyncio.get_event_loop()
            )
            
            print(f"✅ MQTT data stored: {collection}/{key}")
            
        except Exception as e:
            print(f"❌ Database error: {e}")
            db.rollback()
        finally:
            db.close()
    
    def start(self):
        def run_mqtt():
            try:
                self.client.connect("localhost", 1883, 60)
                print("🚀 MQTT client starting...")
                self.client.loop_forever()
            except Exception as e:
                print(f"❌ MQTT connection failed: {e}")
                print("💡 Make sure Mosquitto is running: mosquitto")
        
        # Start MQTT in background thread
        mqtt_thread = threading.Thread(target=run_mqtt)
        mqtt_thread.daemon = True
        mqtt_thread.start()

# Initialize managers
mqtt_manager = MQTTManager()

# Dependency to get DB session
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

# Pydantic Models
class DataItem(BaseModel):
    collection: str
    key: str
    value: dict

class UserRegister(BaseModel):
    username: str
    email: EmailStr
    password: str

class UserLogin(BaseModel):
    username: str
    password: str

class Token(BaseModel):
    access_token: str
    token_type: str

# Helper Functions
def hash_password(password: str) -> str:
    return pwd_context.hash(password)

def verify_password(plain_password: str, hashed_password: str) -> bool:
    return pwd_context.verify(plain_password, hashed_password)

def create_access_token(data: dict):
    to_encode = data.copy()
    expire = datetime.utcnow() + timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    to_encode.update({"exp": expire})
    encoded_jwt = jwt.encode(to_encode, SECRET_KEY, algorithm=ALGORITHM)
    return encoded_jwt

def verify_token(credentials: HTTPAuthorizationCredentials = Depends(security)):
    try:
        token = credentials.credentials
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        username: str = payload.get("sub")
        if username is None:
            raise HTTPException(status_code=401, detail="Invalid authentication credentials")
        return username
    except jwt.ExpiredSignatureError:
        raise HTTPException(status_code=401, detail="Token has expired")
    except jwt.JWTError:
        raise HTTPException(status_code=401, detail="Could not validate credentials")

# Public Endpoints
@app.get("/")
async def root():
    return {
        "message": "Welcome to AlphaBase v3.1.0!",
        "status": "running",
        "features": ["Authentication", "Persistent Storage", "Real-time WebSockets", "MQTT Integration"],
        "storage": "SQLite Database",
        "timestamp": datetime.now().isoformat()
    }

@app.post("/auth/register", response_model=Token)
async def register(user: UserRegister, db: Session = Depends(get_db)):
    """Register a new user"""
    # Check if username exists
    if db.query(UserDB).filter(UserDB.username == user.username).first():
        raise HTTPException(status_code=400, detail="Username already exists")
    
    # Check if email exists
    if db.query(UserDB).filter(UserDB.email == user.email).first():
        raise HTTPException(status_code=400, detail="Email already registered")
    
    # Create new user
    new_user = UserDB(
        username=user.username,
        email=user.email,
        password=hash_password(user.password),
        created_at=datetime.utcnow()
    )
    db.add(new_user)
    db.commit()
    
    # Create access token
    access_token = create_access_token(data={"sub": user.username})
    
    return {
        "access_token": access_token,
        "token_type": "bearer"
    }

@app.post("/auth/login", response_model=Token)
async def login(user: UserLogin, db: Session = Depends(get_db)):
    """Login and get access token"""
    db_user = db.query(UserDB).filter(UserDB.username == user.username).first()
    
    if not db_user:
        raise HTTPException(status_code=401, detail="Invalid username or password")
    
    if not verify_password(user.password, db_user.password):
        raise HTTPException(status_code=401, detail="Invalid username or password")
    
    # Create access token
    access_token = create_access_token(data={"sub": user.username})
    
    return {
        "access_token": access_token,
        "token_type": "bearer"
    }

# WebSocket Endpoint
@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    print(f"✅ WebSocket connected. Total connections: {len(manager.active_connections)}")
    
    try:
        while True:
            # Keep connection alive
            data = await websocket.receive_text()
            print(f"📨 WebSocket message received: {data}")
    except WebSocketDisconnect:
        manager.disconnect(websocket)
        print(f"❌ WebSocket disconnected. Remaining: {len(manager.active_connections)}")

# Protected Endpoints
@app.get("/auth/me")
async def get_current_user(username: str = Depends(verify_token), db: Session = Depends(get_db)):
    """Get current user info"""
    user = db.query(UserDB).filter(UserDB.username == username).first()
    
    if not user:
        raise HTTPException(status_code=404, detail="User not found")
    
    return {
        "username": user.username,
        "email": user.email,
        "created_at": user.created_at.isoformat()
    }

@app.post("/data/set")
async def set_data(item: DataItem, username: str = Depends(verify_token), db: Session = Depends(get_db)):
    """Store data in a collection (requires authentication)"""
    data_id = f"{item.collection}:{item.key}"
    
    # Check if data already exists
    existing_data = db.query(DataDB).filter(DataDB.id == data_id).first()
    
    if existing_data:
        # Update existing data
        existing_data.value = json.dumps(item.value)
        existing_data.owner = username
    else:
        # Create new data
        new_data = DataDB(
            id=data_id,
            collection=item.collection,
            key=item.key,
            value=json.dumps(item.value),
            owner=username,
            created_at=datetime.utcnow()
        )
        db.add(new_data)
    
    db.commit()
    
    # Broadcast update via WebSocket
    await manager.broadcast(json.dumps({
        "action": "update",
        "collection": item.collection,
        "key": item.key
    }))
    
    return {
        "success": True,
        "collection": item.collection,
        "key": item.key,
        "message": "Data stored successfully"
    }

@app.get("/data/get/{collection}/{key}")
async def get_data(collection: str, key: str, username: str = Depends(verify_token), db: Session = Depends(get_db)):
    """Retrieve data from a collection (requires authentication)"""
    data_id = f"{collection}:{key}"
    data = db.query(DataDB).filter(DataDB.id == data_id).first()
    
    if not data:
        raise HTTPException(status_code=404, detail="Data not found")
    
    return {
        "success": True,
        "collection": collection,
        "key": key,
        "data": json.loads(data.value),
        "owner": data.owner
    }

@app.get("/data/list/{collection}")
async def list_collection(collection: str, username: str = Depends(verify_token), db: Session = Depends(get_db)):
    """List all items in a collection (requires authentication)"""
    data_items = db.query(DataDB).filter(DataDB.collection == collection).all()
    
    items = {item.key: json.loads(item.value) for item in data_items}
    
    return {
        "success": True,
        "collection": collection,
        "count": len(items),
        "items": items
    }

@app.delete("/data/delete/{collection}/{key}")
async def delete_data(collection: str, key: str, username: str = Depends(verify_token), db: Session = Depends(get_db)):
    """Delete data from a collection (requires authentication)"""
    data_id = f"{collection}:{key}"
    data = db.query(DataDB).filter(DataDB.id == data_id).first()
    
    if not data:
        raise HTTPException(status_code=404, detail="Data not found")
    
    # Check if user owns this data
    if data.owner != username:
        raise HTTPException(status_code=403, detail="Not authorized to delete this data")
    
    db.delete(data)
    db.commit()
    
    # Broadcast update via WebSocket
    await manager.broadcast(json.dumps({
        "action": "delete",
        "collection": collection,
        "key": key
    }))
    
    return {
        "success": True,
        "message": "Data deleted successfully"
    }

# New endpoint to see MQTT status
@app.get("/system/status")
async def system_status():
    """Get system status including MQTT connections"""
    return {
        "websocket_clients": len(manager.active_connections),
        "mqtt_connected": mqtt_manager.client.is_connected(),
        "timestamp": datetime.now().isoformat()
    }

if __name__ == "__main__":
    # Install required package if not already installed
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        print("❌ paho-mqtt not installed. Install it with:")
        print("   pip install paho-mqtt")
        exit(1)
    
    # Start MQTT integration
    print("🚀 Starting AlphaBase with MQTT Integration...")
    mqtt_manager.start()
    
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)