# API

## About

This is simple API written using python FastAPI. Packacked as docker container for faster deployment.

## Prerequirements

All required python packages are inside `requirements.txt`

## Running

To run simply execute in root folder of this project `uvicorn API.main:app --reload --host 0.0.0.0`. This command is for debug only.

While running folder with two files is required:

- config.json - contains database info for the api e.g.

```json
{
    "PSQL_USER":"postgres",
    "PSQL_PASSWORD":"postgres",
    "PSQL_ADDRESS":"127.0.0.1:5432/postgres"
}
```

- keys.txt - list of keys each one in new line e.g.

```txt
key1
key2
key3
```

## Running as container

To build a container use provided `Dockerfile` in main folder.

And for running use provided `docker-compose.yml` file.
