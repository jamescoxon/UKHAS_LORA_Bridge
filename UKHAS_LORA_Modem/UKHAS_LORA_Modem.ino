/*
 * Built off the RadioLib example and merged with previous UKHASnet code to create a serial/LoRa modem
 * https://github.com/jamescoxon/
 
   RadioLib SX127x Receive with Interrupts Example

   This example listens for LoRa transmissions and tries to
   receive them. Once a packet is received, an interrupt is
   triggered. To successfully receive data, the following
   settings have to be the same on both transmitter
   and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word

   Other modules from SX127x/RFM9x family can also be used.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>
#include <string.h>

#define VERSION 19

// Singleton instance of the radio driver
#define NSS 8
#define DIO0 3
#define RESET 4
#define DIO1 6
SX1278 radio = new Module(NSS, DIO0, RESET, DIO1);

String a;
uint8_t n, i, j, k, packet_len;
uint8_t last_packet[255];
char temp[16];

//****CONFIG******
char id[] = "LORA01";

bool ukhas = true; 
bool output_string = true;

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  receivedFlag = true;
}

void setup() 
{
  Serial.begin(9600);
  while (!Serial) ; // Wait for Serial port to be available

    // initialize SX1278 with default settings
  Serial.print(F("> [SX1278] Initializing ... "));
  int state = radio.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // Set Frequency
  radio.setFrequency(434.4);
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(11);
  radio.setCodingRate(5);
  

  // Setup interrupt
  radio.setDio0Action(setFlag);
  state = radio.startReceive();
  if (state == ERR_NONE) {
    Serial.println(F("> success!"));
  } else {
    Serial.print(F("> failed, code "));
    Serial.println(state);
    while (true);
  }
  
}

void loop()
{
  //Serial.println("Check for new Serial");
  if (Serial.available() > 0) {

    while(Serial.available()) {

      a += Serial.readString();// read the incoming data as string

      a.trim();
      //Serial.println(a); 
    }
    
    if (a.charAt(0) == '$') {

      //COMMANDS
      // F - Freqency (float)
      // P - Power Output
      // I - ID 
      // W - Sync Word
      // L - Preamble Length
      // C - Custom settings (SF, CR, Bandwidth as a float)
      // M - Mode (currently on 5)
      // D - Debug
      // R - CRC Toggle On/Off
      // K - Low Data Rate On/Off
      // U - UKHAS Packet format On/Off
      // V - Version
      // S - Output as String or Binary
      
      a.remove(0, 1);
      Serial.print("> Found Command: ");
      Serial.println(a);
      
      if (a.charAt(0) == 'F') {
        Serial.println("> Changing Freq");
        a.remove(0,1);
        float frequency = a.toFloat();
        radio.setFrequency(frequency);
        Serial.println("> Frequency Set");
      }
      else if (a.charAt(0) == 'P') {
        Serial.println("> Changing Tx Power");
        a.remove(0,1);
        int power = a.toInt();
        if ((power >= 2) && (power <= 20)) {
          radio.setOutputPower(power);
          Serial.println("> Power Set");
        }
        else {
          Serial.println("> Incorrect Power Setting 2-20dbm");
        }
      }
      else if (a.charAt(0) == 'I'){
        Serial.print("> ");
        Serial.println(id);
        a.remove(0,1);
        a.trim();
        int str_len = a.length() + 1;
        a.toCharArray(id, str_len);
        Serial.print("> ");
        Serial.println(id);
        Serial.println("> ID Set");
      }
      
      else if (a.charAt(0) == 'W'){
        a.remove(0,1);

        if (a.charAt(0) == '0')
        {
            radio.setSyncWord(0x12);
            Serial.println("> 0x12 Sync Word Set");
        }
        else
        {
            int syncword = a.toInt();
            radio.setSyncWord(syncword);
            Serial.print("> Sync Word Set: "); Serial.println(syncword);
        }

        Serial.println("> Sync Word Set");
      }
            
      else if (a.charAt(0) == 'L') {
        Serial.println("> Preamble Length");
        a.remove(0,1);
        int preamble_int = a.toInt();
        radio.setPreambleLength(preamble_int);
        Serial.println("> Preamble Length Set");
      }
      
      else if (a.charAt(0) == 'C') {
        Serial.println("> Custom Mode");
        a.remove(0,1);
        int comma_index = a.indexOf(',');
        String sf = a.substring(0, comma_index);
        Serial.print("> SF: "); Serial.println(sf);
        int sf_int = sf.toInt();
        radio.setSpreadingFactor(sf_int);
        a.remove(0,comma_index + 1);

        comma_index = a.indexOf(',');
        String cr = a.substring(0, comma_index);
        Serial.print("> CR: "); Serial.println(cr);
        int cr_int = cr.toInt();
        radio.setCodingRate(cr_int);
        a.remove(0,comma_index + 1);

        comma_index = a.indexOf(',');
        String bandwidth = a.substring(0, comma_index);
        Serial.print("> Bandwidth: "); Serial.println(bandwidth);
        float bandwidth_float = bandwidth.toFloat();
        radio.setBandwidth(bandwidth_float);
        a.remove(0,comma_index + 1);
        
      }
      else if (a.charAt(0) == 'M') {
        Serial.println("> Changing Mode");
        a.remove(0,1);
        
        if (a.charAt(0) == '1')
        {
            //rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);
            Serial.println("> Mode Set");
        }
        else if (a.charAt(0) == '2')
        {
            //rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); 
            Serial.println("> Mode Set");
        }
        else if (a.charAt(0) == '3')
        {
            //rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512); 
            Serial.println("> Mode Set");
        }
        else if (a.charAt(0) == '4')
        {
            //rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);
            Serial.println("> Mode Set"); 
        }
        else if (a.charAt(0) == '5')
        {
            //rf95.setModemConfig(RH_RF95::Bw125Cr45Sf2048); 
            radio.setBandwidth(125.0);
            radio.setSpreadingFactor(11);
            radio.setCodingRate(5);
            output_string = true;
            radio.forceLDRO(true);  
            Serial.println("> Mode Set - 125.0,11,5,str_output:true,LDRO:true");
        }
        else if (a.charAt(0) == '6')
        {
            //rf95.setModemConfig(RH_RF95::Bw125Cr45Sf2048); 
            radio.setBandwidth(250.0);
            radio.setSpreadingFactor(10);
            radio.setCodingRate(5);
            output_string = false;
            radio.forceLDRO(true);  
            Serial.println("> Mode Set (Norbi) - 250.0,10,5,str_output:false,LDRO:true");
        }
      }
       else if (a.charAt(0) == 'D') {
        Serial.println("> Debug");
      for (int i = 0; i < 255; i++) {
        sprintf(temp, "%.2X",last_packet[i]);
        Serial.print(temp); Serial.print(" ");
      }
      Serial.println(); 
       }
       
       else if (a.charAt(0) == 'R') {
        a.remove(0,1);
        
        if (a.charAt(0) == '0')
        {
        Serial.println("> CRC Off");
        radio.setCRC(false);
        //rf95.setPayloadCRC(false);
        }
        else
        {
        Serial.println("> CRC On");
        radio.setCRC(true);
        //rf95.setPayloadCRC(true);        
        }
       }

       else if (a.charAt(0) == 'S') {
        a.remove(0,1);
        
        if (a.charAt(0) == '0')
        {
            Serial.println("> Binary Output");
            output_string = false;
        }
        else
        {
            Serial.println("> String Output");
            output_string = true;
             
        }
       }
       
       else if (a.charAt(0) == 'K') {
        a.remove(0,1);
        
        if (a.charAt(0) == '0')
        {
        Serial.println("> Low Data Rate Off");
        radio.forceLDRO(false);
        }
        else
        {
        Serial.println("> Low Data Rate On");
        radio.forceLDRO(true);       
        }
       }
       
        else if (a.charAt(0) == 'U') {
        a.remove(0,1);
        
        if (a.charAt(0) == '0')
        {
        Serial.println("> UKHAS Packet Format Off");
        ukhas = false;
        }
        else
        {
        Serial.println("> UKHAS Packet Format On");
        ukhas = true;        
        }
       }

       else if (a.charAt(0) == 'V') {
        Serial.print("> Version ");
        Serial.println(VERSION);
       }
       
      else if (a.charAt(0) == 'A'){
        Serial.println("> Changing State");
        a.remove(0,1);
        if (a.charAt(0) == 'R'){
          //rf95.setModeRx();
          Serial.println("> Rx Set");
        }
        else if (a.charAt(0) == 'T'){
          //rf95.setModeTx();
          Serial.println("> Tx Set");
        }
        else if (a.charAt(0) == 'I'){
          //rf95.setModeIdle();
          Serial.println("> Idle Set");
        }
      }

      a = "";
     // put module back to listen mode
    radio.startReceive();

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
    }
  }

  // Detect multiple lines

  if (ukhas == true) {
  if (a.indexOf("]") != -1){
          
      String packet = a.substring(0, a.indexOf("]") + 1);
      a.remove(0, a.indexOf("]") + 1);
      Serial.print("> Sending: ");
      Serial.println(packet);
          
      int str_len = packet.length() + 1; 
      uint8_t char_array[str_len];
      packet.getBytes(char_array, str_len);

      //rf95.waitCAD();
      //rf95.send(char_array, sizeof(char_array));
      int state = radio.transmit(packet);
  
      //rf95.waitPacketSent();
      Serial.println("> Sent");
      transmittedFlag = true;
        
  }
  else {
      a = "";
  }
  }
  else if (a.length() > 3 && a.indexOf("$") != 0){
      String packet = a;
      a = "";
      Serial.print("> Sending: ");
      Serial.println(packet);
      int state = radio.transmit(packet);
  
      //rf95.waitPacketSent();
      Serial.println("> Sent");
      transmittedFlag = true;
  }
  
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;
    
    
    if(transmittedFlag) {
      transmittedFlag = false;
      //Serial.println("> Already sent");
    }
    else {
    // you can read received data as an Arduino String

    int state;
    String str;
    
    if (output_string == true) {
      state = radio.readData(str);
    }
    else {
      uint8_t data_array[255];
      state = radio.readData(data_array, 255);
      memcpy(last_packet, data_array, 255);
      
      //Serial.print('Binary String Found:');
      for (int i = 0; i < 255; i++) {
        sprintf(temp, "%.2X",data_array[i]);
        Serial.print(temp); Serial.print(" ");
      }
      Serial.println(); 
      
    }

    if (state == ERR_NONE) {
      // print data of the packet
      //Serial.print(F("> [SX1278] Data:\t\t"));
      Serial.print(str);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print("|");
      Serial.println(radio.getRSSI());

    } else if (state == ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("> [SX1278] CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("> [SX1278] Failed, code "));
      Serial.println(state);

    }
    }
    // put module back to listen mode
    radio.startReceive();

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
}
