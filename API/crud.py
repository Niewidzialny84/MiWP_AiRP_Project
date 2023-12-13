from sqlalchemy.orm import Session

from . import models, schemas

def get_all_records(db: Session):
    return db.query(models.Sensors).all()

def get_records(db: Session, key: str):
    return db.query(models.Sensors).filter(models.Sensors.device_key == key).all()

def add_record(db: Session, key: str, data: schemas.SensorData):
    record = models.Sensors(device_key=key)
    record.humidity = data.humidity
    record.temperatureC = data.temperatureC
    record.pressure = data.pressure
    record.ppmAlcohol = data.ppmAlcohol
    record.ppmCH4 = data.ppmCH4
    record.ppmCO = data.ppmCO
    record.ppmH2 = data.ppmH2
    record.ppmLPG = data.ppmLPG
    db.add(record)
    db.commit()
    db.refresh(record)
    return record