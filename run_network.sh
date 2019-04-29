#!/usr/bin/env bash

if [[ $1 = "--help" ]]; then
    echo './run_network.sh "topo_file.py"'

else
    IP=172.30.7.201
    PORT=6653
    TOPOLOGY=$1

    sudo mn --custom $TOPOLOGY \
            --topo mytopo \
            --switch ovsk,protocols=OpenFlow13 \
            --controller remote,ip=$IP,port=$PORT
fi
