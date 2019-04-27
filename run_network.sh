#!/usr/bin/env bash

IP=172.30.7.201
PORT=6653
TOPOLOGY=$1
# TOPOLOGY=topology.py

sudo mn --custom $TOPOLOGY \
        --topo mytopo \
        --switch ovsk,protocols=OpenFlow13 \
        --controller remote,ip=$IP,port=$PORT
