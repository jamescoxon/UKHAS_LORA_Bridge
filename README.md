# UKHAS_LORA_Bridge

This allows a bridge between a local 869.5Mhz UKHASnet network and a LoRa UKHASnet link (used to link local networks together. Currently being used to link 2 UKHASnet networks over a distance of 5.8km. 

There are 2 components, a python script that manages the LoRa bridge and a modified UKHASnet SDR decoder.

## Installation

## Running
Example: python3 lora.py -c -p 10 -i AB1

* -c = this sets the script to upload received strings to the UKHAS.net website
* -p = this allows you to adjust the power output of the radio, measured in dBm can be between 2 and 20
* -i = this is the gateway idenfier.
* -b = turns off broadcasting of packets via the LoRa radio, default is on.

## Future

* add packet repeating
* add ability to chat over the network

```
apt install git python3-pip make gcc g++ libcurl4-openssl-dev screen redis
git clone https://wvsensornet.xyz/git/jamescoxon/UKHAS_LORA_Bridge,get
cd UKHAS_LORA_Bridge
git submodule init
git submodule update
cd UKHASnet-decoder/hiredis/
make
make install 
ldconfig
cd ..
make
cd ..
pip3 install serial requests redis
python3 lora.py -c -p 10 -i AB2
```
