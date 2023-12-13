from sqlalchemy import Column, Integer, String, Float, DateTime
from sqlalchemy.sql import expression
from sqlalchemy.ext.compiler import compiles
from sqlalchemy.types import DateTime

class utcnow(expression.FunctionElement):
    type = DateTime()
    inherit_cache = True

@compiles(utcnow, 'postgresql')
def pg_utcnow(element, compiler, **kw):
    return "TIMEZONE('utc', CURRENT_TIMESTAMP)"

from .database import Base

class Sensors(Base):
    __tablename__ = "sensors_values"

    id = Column(Integer, primary_key=True, index=True)
    device_key = Column(String)
    humidity = Column(Float)
    temperatureC = Column(Float)
    pressure = Column(Float)
    ppmCO = Column(Float)
    ppmAlcohol = Column(Float)
    ppmCH4 = Column(Float)
    ppmH2 = Column(Float)
    ppmLPG = Column(Float)
    date_time = Column(DateTime, server_default=utcnow())