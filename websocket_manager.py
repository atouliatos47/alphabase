# websocket_manager.py
from fastapi import WebSocket, WebSocketDisconnect
import json
from typing import List

class ConnectionManager:
    def __init__(self):
        self.active_connections: List[WebSocket] = []

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

    async def websocket_endpoint(self, websocket: WebSocket):
        await self.connect(websocket)
        print(f"✅ WebSocket connected. Total connections: {len(self.active_connections)}")
        
        try:
            while True:
                # Keep connection alive
                data = await websocket.receive_text()
                print(f"📨 WebSocket message received: {data}")
        except WebSocketDisconnect:
            self.disconnect(websocket)
            print(f"❌ WebSocket disconnected. Remaining: {len(self.active_connections)}")

# Create global instance
manager = ConnectionManager()