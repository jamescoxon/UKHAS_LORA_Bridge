# UKHAS_LORA_Bridge

This allows a bridge between a local 869.5Mhz UKHASnet network and a LoRa UKHASnet link (used to link local networks together. Currently being used to link 2 UKHASnet networks over a distance of 5.8km. 

There are multiple components all linked by a redis database, different components include  a python script that manages the LoRa bridge and a modified UKHASnet SDR decoder.

## Running
Example: `python3 lora.py -c -p 10 -i AB1 -r`

* -c = this sets the script to upload received strings to the UKHAS.net website
* -p = this allows you to adjust the power output of the radio, measured in dBm can be between 2 and 20
* -i = this is the gateway idenfier.
* -b = turns off broadcasting of packets via the LoRa radio, default is on.
* -r = turns on repeating rx'd packets, default is off
* -m = allows you to change the mode, default is (now) 5, see below for more options.

## Modes
* 1 Bw125Cr45Sf128   Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range.
* 2 Bw500Cr45Sf128  Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range.
* 3 Bw31_25Cr48Sf512  Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range.
* 4 Bw125Cr48Sf4096   Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, low data rate, CRC on. Slow+long range.
* 5 Bw125Cr45Sf2048   Bw = 125 kHz, Cr = 4/5, Sf = 2048chips/symbol, CRC on. Slow+long range. 

## Chat (chat.py)
If you have the lora.py script running (either as a service or in a seperate screen) you can then use `chat.py` to send custom messages. The script creates a UKHASnet packet and puts it in the redis database for the main lora.py to send. The script prioritises the packet over other packets.

## Beacon (beacon.py)
This is a simple script that broadcasts every 60 seconds a short packet. Currently it uses the CPU temperature on the Pi but could be used for anything.

## Future

* make the system more robust

## Installation on Ubuntu/Debian/Raspbian/DietPi

```
sudo apt install git python3-pip make gcc g++ libcurl4-openssl-dev screen redis git python3-setuptools
git clone https://wvsensornet.xyz/git/jamescoxon/UKHAS_LORA_Bridge.git
cd UKHAS_LORA_Bridge
git submodule init
git submodule update
cd UKHASnet-decoder/hiredis/
make
sudo make install 
sudo ldconfig
cd ..
make
cd ..
pip3 install pyserial requests redis
```

## Installation as a systemd service

```
cp lora.service /etc/systemd/system/
systemctl enable lora.service
systemctl start lora.service
```

To check it it is working use the command:
`systemctl status lora.service`

