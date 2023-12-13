from fastapi import FastAPI, HTTPException, Security, status
from fastapi.security import APIKeyHeader

from fastapi import Depends, FastAPI, HTTPException
from sqlalchemy.orm import Session

from . import crud, models, schemas
from .database import SessionLocal, engine

import os
print(os.listdir())

models.Base.metadata.create_all(bind=engine)

def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

api_keys = open('./configs/keys.txt').read().splitlines()

app = FastAPI()
api_key_header = APIKeyHeader(name="X-API-Key")

def get_api_key(api_key_header: str = Security(api_key_header)) -> str:
    if api_key_header in api_keys:
        return api_key_header
    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Invalid or missing API Key",
    )

@app.post("/sensors", status_code=201)
async def create_record(data: schemas.SensorData, api_key: str = Security(get_api_key), db: Session = Depends(get_db)):
    return crud.add_record(db=db, key=api_key, data=data)

@app.get("/sensors", response_model=list[schemas.SensorData] ,status_code=200)
async def get_records(api_key: str = Security(get_api_key), db: Session = Depends(get_db)):
    records = crud.get_records(db=db, key=api_key)
    return records