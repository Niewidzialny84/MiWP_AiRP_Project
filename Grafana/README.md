# Running Grafana

This is simple solution for running grafana in docker container on any device. Simply copy and paste `docker-compose.yml` file and run using `docker compose up -d`.

Note: Creating network for proper communication between container could be nessesary, then uncoment the network values and before running container execute `docker network create -d bridge external-test-network` - where `external-test-network` could be replaced with any other network name.
