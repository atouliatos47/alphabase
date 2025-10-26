# models.py
from sqlalchemy import create_engine, Column, String, Text, DateTime, Integer
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
from datetime import datetime

# Database Setup
SQLALCHEMY_DATABASE_URL = "sqlite:///./alphabase.db"
engine = create_engine(SQLALCHEMY_DATABASE_URL, connect_args={"check_same_thread": False})
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)
Base = declarative_base()

class UserDB(Base):
    __tablename__ = "users"
    username = Column(String, primary_key=True, index=True)
    email = Column(String, unique=True, index=True)
    password = Column(String)
    created_at = Column(DateTime, default=datetime.utcnow)

class DataDB(Base):
    __tablename__ = "data"
    id = Column(String, primary_key=True)
    collection = Column(String, index=True)
    key = Column(String)
    value = Column(Text)
    owner = Column(String)
    created_at = Column(DateTime, default=datetime.utcnow)

class FileDB(Base):
    __tablename__ = "files"
    id = Column(String, primary_key=True)
    filename = Column(String)
    original_filename = Column(String)
    file_path = Column(String)
    file_size = Column(Integer)
    mime_type = Column(String)
    owner = Column(String)
    is_public = Column(String, default="false")
    created_at = Column(DateTime, default=datetime.utcnow)