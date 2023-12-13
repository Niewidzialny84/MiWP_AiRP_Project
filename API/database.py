from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
import json

file = open('./configs/config.json')
config = json.load(file)

SQLALCHEMY_DATABASE_URL = "postgresql://%s:%s@%s" % (config['PSQL_USER'],config['PSQL_PASSWORD'], config['PSQL_ADDRESS'])

engine = create_engine(
    SQLALCHEMY_DATABASE_URL
)

SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

Base = declarative_base()