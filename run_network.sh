#!/usr/bin/env bash

if [[ $1 = "--help" ]]; then
    echo './run_network.sh TOPOLOGY_FILE CONTROLLER_IP CONTROLLER_PORT'

else
    TOPOLOGY=$1
    IP=$2
    PORT=$3

    sudo mn --custom $TOPOLOGY \
            --topo mytopo \
            --switch ovsk,protocols=OpenFlow13 \
            --controller remote,ip=$IP,port=$PORT
fi
