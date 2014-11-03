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
 * Receive a Message from a remote Device
 *
 * returns false if no message or error
 */
bool Secure_nRF::receiveMessage(byte* payload)
{
	
}

void Secure_nRF::randomiseIV(byte* iv)
{
	int ivSize = 16;
	for(int i=0; i<ivSize; i++){
		iv[i] = random(0, 256);
	}
}