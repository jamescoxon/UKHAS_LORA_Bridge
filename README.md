# UKHAS_LORA_Bridge

This allows a bridge between a local 869.5Mhz UKHASnet network and a LoRa UKHASnet link (used to link local networks together. Currently being used to link 3 UKHASnet networks over a distance of 12km. 

There are multiple components all linked by a redis database, different components include  a python script that manages the LoRa bridge and a modified UKHASnet SDR decoder. The redis database acts as a queue system for example packets from the UKHASnet side will expire after 120 seconds if not broadcast if there is a duty cycle limit, chat and beacon packets are prioritised.

## Running
Example: `python3 lora.py -c -p 10 -i AB1 -r`

* -c = this sets the script to upload received strings to the UKHAS.net website
* -p = this allows you to adjust the power output of the radio, measured in dBm can be between 2 and 20
* -i = this is the gateway idenfier.
* -b = turns off broadcasting of packets via the LoRa radio, default is on.
* -r = turns on repeating rx'd packets, default is off
* -m = allows you to change the mode, default is (now) 5, see below for more options.
* -d = duty cycle, as an integer, default is 10, not required, for example `-d 10`

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

## Duty Cycle
UKHASnet and the WV LoRa network transmit using licence exempt rules (IR2030), UKHASnet is on 869.500MHz and LoRa on 434.400MHz. On 434.400MHz there is a 10% duty cycle which we have calculated as 6 packets per minute (each packet takes roughly 1 second to transmit). The bridge code now keeps track of how many packets are sent in a minute and will delay further transmission if has reached 6 in the last minute. This means that bridged sensor packets may well be discarded (as they are only stored for 120 seconds) but chat messages will be retained and broadcast at the next available slot.

## Future

* make the system more robust

## Installation on Ubuntu/Debian/Raspbian/DietPi

```
sudo apt install git python3-pip make gcc g++ libcurl4-openssl-dev screen redis git python3-setuptools
git clone https://github.com/jamescoxon/UKHAS_LORA_Bridge.git
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

