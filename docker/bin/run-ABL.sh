#!/bin/bash

cd /opt/media/ABLMediaServer
sudo chmod +x ABLMediaServer 
export LD_LIBRARY_PATH=/opt/media/ABLMediaServer:$LD_LIBRARY_PATH
./MediaServer -d -m 3
