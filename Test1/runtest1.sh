#!/bin/sh
trap 'pkill rcsapp-server' SIGINT
pwd
./rcsapp-server &
sleep 1
./rcsapp-client
pkill rcsapp-server
pkill rcsapp-client