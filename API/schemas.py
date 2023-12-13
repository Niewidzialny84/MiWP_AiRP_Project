from typing import Optional
from pydantic import BaseModel

class SensorData(BaseModel):
    humidity: float
    temperatureC: float
    pressure: float
    ppmCO: float
    ppmAlcohol: float
    ppmCH4: float
    ppmH2: float
    ppmLPG: float
    id: Optional[int] = None
    device_key: Optional[str] = None

    class Config:
        orm_mode = True