/**
 * AES encryption with nRF24L0l nodes
 * 
 * Testing rolling IV
 *
 * MESSAGE HEADER (unencrypted)
 * [ Packet Type (8 bytes) ; Packet UUID (4 bytes) ; Message Type (1 byte) ; Message Length (1 byte) ; AES IV (16 bytes) ; CRC (2 bytes) ]
 *
 * MESSAGE PAYLOAD (encrypted)
 * [ Message (32 bytes) ]
 *
 * Packet Type: 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF (Header)
 * Packet UUID: unsigned long (MSB -> LSB)
 * Message Type: 0x01 = Request, 0x02 = Response, 0x03 Resend Request, 0x04 Resend Response
 * Message Length: 0 -> 255 = 0 -> 8 blocks w/ crc included in each payload block
 * AES IV: 16 byte IV
 * CRC: 2 byte CRC
 *
 */ 
#include <SPI.h>
#include "nRF24L01.h"  
#include "RF24.h"
#include "AES.h"
#include "CRC16.h"
#include <LiquidCrystal.h>

// Global objects
AES aes;
CRC crc;

/**
 * LCD Setup
 */
// include the library code:
int LCD_RS = 15;
int LCD_E = 14;
int LCD_D4 = 18;
int LCD_D5 = 19;
int LCD_D6 = 17;
int LCD_D7 = 16;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Key is secret but known to both sender and receiver
byte key[] = 
{
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// IV is unknown prior to receiving message header
byte iv[16];

// Set up nRF24L01 radio on SPI bus plus pins 2 & 10 
RF24 radio(2, 10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0E2LL };   

#define PAYLOAD_SIZE 32
byte txBuff[PAYLOAD_SIZE];
byte rxBuff[PAYLOAD_SIZE];
byte outBuff[PAYLOAD_SIZE];

boolean messageMode = true; // true = header, false = payload

#define LED 9

void setup()
{
  radio.begin();
  radio.setRetries(4,15);  // Set the delay between retries(per 250uS) & # of retries
  radio.setPayloadSize(PAYLOAD_SIZE);  // A smaller payload size improves reliability, set it for what you need
  //radio.setPAPower();
  radio.setDataRate(RF24_250KBPS);  // Set the Speed RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.openReadingPipe(1, pipes[1]);  //Listen on Radio Pipe(channel) 1
  radio.startListening(); 

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);  //Blink to say we are alive
  delay(25);
  digitalWrite(LED, LOW);

  Serial.begin(9600); //For Debugging & bluetooth 
  Serial.println("Receiving! Awaiting a message header.");
  
  // Setup LCD
  lcd.begin(40, 2);
  
  // Print a message to the LCD.
  lcd.print("Awaiting a message header via nRF...  ");
  
  // Turn on the display:
  lcd.display();
       
  aes.set_key (key, 128);
}

void loop()
{
  receiveMessage();
  
  delay(5);
}

void receiveMessage()
{
  Serial.println("receiveMessage:");
  
  bool packetAvailable = receivePacket();
  
  if(packetAvailable != false){

    Serial.println("packet received, validating header packet type...");
    
    // We always assume the we are waiting for a new message... 
    if(isHeader(rxBuff) && decodeHeader(rxBuff)){
     
      // wait for message packet
      while(1){

          Serial.println("awaiting message packet...");
        
          packetAvailable = receivePacket();
          
          if(packetAvailable != false){
            
            decodeMessage(rxBuff);
          }
          break;          
      } 
    }       
  } else {
     Serial.println("No packet.");
     shortBlink(); 
  }
}

bool isHeader(byte* header)
{
  // Check Packet Type
  for(int i=0; i<8; i++){
    if(header[i] != 0xFF){
      Serial.println("NOT A HEADER PACKET!!!");
      Serial.write(header, HEX);
      Serial.println(" ");      
      
      return false;
    } 
  }
  
  Serial.println("Header packet type validated!");
  
  return true;
}

bool receivePacket()
{
  if (radio.available())
  {
    bool done = false;    
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      done = radio.read(rxBuff, PAYLOAD_SIZE);
      //delay(2);      
    }
    
    resetNRF();
    
    // Packet received
    return true;
  }
  
  // No packet waiting...
  return false;
}

void resetNRF()
{
  radio.stopListening();
  radio.startListening();
}

void longBlink()
{
    digitalWrite(LED, HIGH);  // Long blink to say we got a message
    delay(250);
    digitalWrite(LED, LOW);
    delay(250);   
}
void shortBlink()
{
    digitalWrite(LED, HIGH);  // Short blink to say NO message
    Serial.print(".");    
    delay(20);
    digitalWrite(LED, LOW);
    delay(20);
}
  
bool decodeHeader(byte* header)
{
  Serial.println("decodeHeader");
  Serial.write(header, 32);
  Serial.println(" ");
  
  // Clear the display
  lcd.clear();
  lcd.print("Header :");
  
  // Display header on LCD
  for(int i=0;i<PAYLOAD_SIZE;i++) {
    lcd.write(header[i]);
  }
  
  // Check Packet Type
  if(!isHeader(header)){
    
      return false;
  }
    
  // Decode Packet UUID
  unsigned long uuid = ((unsigned long)(header[8]) << 24) + ((unsigned long)(header[9]) << 16) + ((unsigned long)(header[10]) << 8) + header[11];
  Serial.println("UUID: ");
  Serial.println(uuid);
  
  // Decode Message Type
  byte msgType = header[12];
  Serial.println("msgType: ");
  Serial.println(msgType, HEX);
  
  // Decode Message Length
  byte msgLen = header[13];
  Serial.println("msgLen: ");
  Serial.println(msgLen);
  
  // Decode AES IV
  for(int i=14, j=0; i<30; i++, j++){
    iv[j] =  header[i];
  } 
  // debug
  for(int i=0; i<16; i++){
    Serial.print(iv[i], HEX);
  }
  Serial.println(" "); 
 
  // Verify the crc checksum
  bool crcCheck = crc.verifyCRC16(header, PAYLOAD_SIZE);
  
  if(crcCheck == true) {
    Serial.println("CRC check ok.");
    //lcd.print("Decoded:");
    
    return true;
    
  } else {      
    Serial.println("CRC failed!");
    //lcd.print("CRC err:");
    
    return false;
  } 
  
}

void decodeMessage(byte* msg)
{
  Serial.println("decodeMessage");
  Serial.write(msg, HEX);
  Serial.println(" ");
    
  // Decrypt the Message
  int blocks = 2;
  byte succ = aes.cbc_decrypt(msg, outBuff, blocks, iv);
    
  // Verify the crc checksum
  bool crcCheck = crc.verifyCRC16(outBuff, PAYLOAD_SIZE);
    
  if(crcCheck == true) {
    
    // Display Decoded msg on LCD
    lcd.print("Decoded:");
  
    for(int i=0;i<PAYLOAD_SIZE;i++) {
      lcd.write(outBuff[i]);
    }
    Serial.write(outBuff, PAYLOAD_SIZE);
    Serial.println(" ");
    
  } else {      
    lcd.print("CRC err:");
    Serial.println("CRC error in Message Packet");
  }
    
  Serial.println("=============================");
 
}