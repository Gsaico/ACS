/**
 * Secure nRF communication library
 *
 * Features: 
 *		128bit AES encryption
 *		30 byte message packets (2 bytes for CRC)
 *		CRC16 
 * 		(not implemented: automatic resend messaging)
 *
 * Library dependencies:
 *		SPI 	(core lib)
 *		RF24 	(http://maniacbug.github.io/RF24/)
 *		CRC16 	(http://github.com/ronnied/crc16/)
 *		AES 	(https://github.com/Baldanos/pa55ware/tree/master/libraries/AES)
 */
#include "Arduino.h"
#include "Secure_nRF.h"

/**
 * Class Constructor
 */
Secure_nRF::Secure_nRF(void):
	_nRF(RF24(HWPIN_SPI_CE, HWPIN_SPI_CS))
{
	// 32 Bytes per nRF packet
	_payloadSize = 32;
}

/**
 * Required to correctly initialise the nRF object
 */
void Secure_nRF::begin(void)
{
	_nRF.begin();
	_nRF.setRetries(4, 15);
	_nRF.setPayloadSize(32);
	_nRF.setDataRate(RF24_250KBPS);

	// This needs more thought
	_nRF.openReadingPipe(1, _destinationDeviceId);
	_nRF.startListening();   
}

/**
 * Local Key and IV
 */
void Secure_nRF::setLocalAESParams(AESParams l)
{
	_localAESParams = l;
}

/**
 * Destination Key and IV
 */
void Secure_nRF::setDestinationAESParams(AESParams d)
{
	_destinationAESParams = d;
}

/**
 * Set Local Device ID
 */
void Secure_nRF::setLocalDeviceId(uint64_t id)
{
	_localDeviceId = id;
}

/**
 * Set Destination Device ID
 */
void Secure_nRF::setDestinationDeviceId(uint64_t id)
{
	_destinationDeviceId = id;
}
 
/**
 * Send an Encrypted Message to the Destination Device
 */
void Secure_nRF::sendMessage(byte* payload)
{	
	// Initialise Variables
	int payloadSize = 32;
	int blocks = 2;
	int keyLength = 128;
	byte temp[32];
	for(int i=0; i<32; i++){
		temp[i] = 0x00;
	}

	// Debug
	//Serial.println("sendPacket");
	//Serial.write(payload, payloadSize);
	//Serial.println(" ");
	
	// CRC the payload
	_crc.appendCRC16(payload, payloadSize);
		
	// Set the Destination AES key
	_aes.set_key(_destinationAESParams.key, keyLength);
	
	// Randomise the IV
	//randomiseIV(_destinationAESParams.iv);
	
	// Block cipher the 2 x 16 byte blocks
	_aes.cbc_encrypt(payload, temp, blocks, _destinationAESParams.iv);
	
	// Prepare a Message Header //
	
	// Create a UUID for this Message Request
	unsigned long uuid = 0xF0F1F2F4; //random(4294967295);
	byte msgType = 0x01; // Request
	byte msgLen = 32; // 32 Bytes in payload
	
	// Send the Message Header
	_txHeader(uuid, msgType, msgLen, _destinationAESParams.iv);
		
	// Send the Encrypted Message
	_txPacket(temp);
	
	// Await Response...
}
/**
 * Transmit a Message Header
 */
void Secure_nRF::_txHeader(unsigned long uuid, byte msgType, byte msgLen, byte* iv)
{
	int payloadSize = 32;

	// Header byte array
	byte header[32];

	// Packet Type (0xFF)
	for(int i=0; i<8; i++){
		header[i] = 0xFF;
	}
	
	// UUID
	header[8] = uuid >> 24; 		// (MSB)
	header[9] = (uuid >> 16) & 0xFF;	//
	header[10] = (uuid >> 8) & 0xFF;	//
	header[11] = uuid & 0xFF;		// (LSB)
	
	// Message Type
	header[12] = msgType;
	
	// Message Length
	header[13] = msgLen;
	
	// AES IV	
	for(int i=14, j=0; i<30; i++, j++){
		header[i] = iv[j];
	}	
	
	// Clear the CRC bytes
	header[30] = 0x00;
	header[31] = 0x00;
	
	// CRC the array
	_crc.appendCRC16(header, payloadSize);
	
	// Send the Header
	_txPacket(header);	
}

/**
 * Transmit a packet to the destination device id
 */
void Secure_nRF::_txPacket(byte* packet)
{
	int payloadSize = 32;

	_nRF.openWritingPipe(_destinationDeviceId);
	_nRF.stopListening();
	
	bool ok = false;
	do{
		// Write packet to the radio module
		ok = _nRF.write(packet, payloadSize); 
		//if(!ok){
			//delay(40);
		//	delay(1);
		//}
		// Reset nRF before next transmit
		_nRF.startListening();  
		//delay(20);
		//delay(2);
		_nRF.stopListening();
	} while(!ok);
}

/**
 * Receive a packet for this local device id
 */
bool Secure_nRF::_rxPacket(byte* packet)
{
  int PAYLOAD_SIZE = 32;
  
  if (_nRF.available())
  {
    bool done = false;    
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      done = _nRF.read(packet, PAYLOAD_SIZE);
      //delay(2);      
    }
    
	// Reset nRF
	_nRF.stopListening();
	_nRF.startListening();
    
    // Packet received
    return true;
  }
  
  // No packet waiting...
  return false;
}

/**
 * Receive a Message from a remote Device
 *
 * returns false if no message or error
 */
bool Secure_nRF::receiveMessage(byte* payload)
{
  Serial.println("receiveMessage:");
  
  _nRF.openReadingPipe(1, _destinationDeviceId);
  _nRF.startListening();   
  
  int LED = 9;
  byte rxBuff[32];
    
  if(_rxPacket(rxBuff)){

    Serial.println("packet received, validating header packet type...");
    
    // We always assume the we are waiting for a new message... 
    if(_isHeader(rxBuff) && _decodeHeader(rxBuff)){
     
      // wait for message packet
      while(1){

          Serial.println("awaiting message packet...");

          if(_rxPacket(rxBuff)){
            
            _decodeMessage(rxBuff, payload);
          }
          break;          
      } 
    }
	
	return true;
	
  } else {
     Serial.println("No packet.");
 
    digitalWrite(LED, HIGH);  // Short blink to say NO message
    Serial.print(".");    
    delay(20);
    digitalWrite(LED, LOW);
    delay(20);
	
	return false;
  }
	
}

bool Secure_nRF::_isHeader(byte* header)
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

bool Secure_nRF::_decodeHeader(byte* header)
{
  int PAYLOAD_SIZE = 32;
  
  Serial.println("decodeHeader");
  Serial.write(header, 32);
  Serial.println(" ");
  
  // Clear the display
  //lcd.clear();
  //lcd.print("Header :");
  
  // Display header on LCD
  //for(int i=0;i<PAYLOAD_SIZE;i++) {
  //  lcd.write(header[i]);
  //}
  
  // Check Packet Type
  if(!_isHeader(header)){
    
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
    _localAESParams.iv[j] =  header[i];
  } 
  // debug
  for(int i=0; i<16; i++){
    Serial.print(_localAESParams.iv[i], HEX);
  }
  Serial.println(" "); 
 
  // Verify the crc checksum
  bool crcCheck = _crc.verifyCRC16(header, PAYLOAD_SIZE);
  
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

void Secure_nRF::_decodeMessage(byte* msg, byte* output)
{
  int PAYLOAD_SIZE = 32;
  
  Serial.println("decodeMessage");
  Serial.write(msg, HEX);
  Serial.println(" ");
    
  // Decrypt the Message
  int blocks = 2;
  byte succ = _aes.cbc_decrypt(msg, output, blocks, _localAESParams.iv);
    
  // Verify the crc checksum
  bool crcCheck = _crc.verifyCRC16(output, PAYLOAD_SIZE);
    
  if(crcCheck == true) {
    
    // Display Decoded msg on LCD
    //lcd.print("Decoded:");
  
    //for(int i=0;i<PAYLOAD_SIZE;i++) {
    //  lcd.write(output[i]);
    //}
    Serial.write(output, PAYLOAD_SIZE);
    Serial.println(" ");
    
  } else {      
    //lcd.print("CRC err:");
    Serial.println("CRC error in Message Packet");
  }
    
  Serial.println("============================="); 
}

void Secure_nRF::randomiseIV(byte* iv)
{
	int ivSize = 16;
	for(int i=0; i<ivSize; i++){
		iv[i] = random(0, 256);
	}
}