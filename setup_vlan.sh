#!/bin/bash

if [[ $1 == "--help" || $1 == "-h" ]]
then
    echo "Usage: ./setup_vlan.sh <host_name> <vlan_tag> <IP-address>"
else
    HOST=$1
    TAG=$2
    IP=$3

    modprobe 8021q

    vconfig add $HOST-eth0 $TAG

    ifconfig $HOST-eth0.$TAG up

    ifconfig $HOST-eth0.$TAG $IP
fi
