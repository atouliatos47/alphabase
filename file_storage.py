# file_storage.py
import os
import uuid
import shutil
from datetime import datetime
from fastapi import UploadFile, HTTPException
from typing import Dict, Any, Optional
from sqlalchemy.orm import Session

from models import FileDB

class FileStorage:
    """Manage file uploads, downloads, and metadata"""
    
    def __init__(self):
        self.storage_dir = "alphabase_storage"
        self.users_dir = os.path.join(self.storage_dir, "users")
        self.public_dir = os.path.join(self.storage_dir, "public")
        
        # Create storage directories
        os.makedirs(self.storage_dir, exist_ok=True)
        os.makedirs(self.users_dir, exist_ok=True)
        os.makedirs(self.public_dir, exist_ok=True)
    
    def generate_file_id(self) -> str:
        """Generate unique file ID"""
        return str(uuid.uuid4())
    
    def get_user_storage_path(self, username: str) -> str:
        """Get user's personal storage directory"""
        user_dir = os.path.join(self.users_dir, username)
        os.makedirs(user_dir, exist_ok=True)
        return user_dir
    
    async def save_upload_file(self, upload_file: UploadFile, username: str, is_public: bool = False) -> Dict[str, Any]:
        """Save uploaded file and return file info"""
        file_id = self.generate_file_id()
        original_filename = upload_file.filename
        file_extension = os.path.splitext(original_filename)[1]
        
        # Determine storage location
        if is_public:
            storage_dir = self.public_dir
        else:
            storage_dir = self.get_user_storage_path(username)
        
        # Create unique filename
        unique_filename = f"{file_id}{file_extension}"
        file_path = os.path.join(storage_dir, unique_filename)
        
        # Save file
        file_size = 0
        with open(file_path, "wb") as buffer:
            content = await upload_file.read()
            file_size = len(content)
            buffer.write(content)
        
        return {
            "file_id": file_id,
            "filename": unique_filename,
            "original_filename": original_filename,
            "file_path": file_path,
            "file_size": file_size,
            "mime_type": upload_file.content_type,
            "is_public": is_public
        }
    
    def get_file_path(self, file_id: str, db: Session) -> Optional[str]:
        """Get file path by file ID"""
        file_record = db.query(FileDB).filter(FileDB.id == file_id).first()
        if file_record:
            return file_record.file_path
        return None
    
    def delete_file(self, file_id: str, db: Session) -> bool:
        """Delete file from storage and database"""
        file_record = db.query(FileDB).filter(FileDB.id == file_id).first()
        if file_record:
            # Delete physical file
            if os.path.exists(file_record.file_path):
                os.remove(file_record.file_path)
            
            # Delete database record
            db.delete(file_record)
            db.commit()
            return True
        return False

# Create global instance
file_storage = FileStorage()