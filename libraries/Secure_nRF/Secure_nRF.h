/**
 * Secure nRF communication library
 *
 * Features:
 *		128bit AES encryption
 *		30 byte message packets (2 bytes for CRC)
 *		CRC16 with automatic resend messaging
 *
 * Library dependencies:
 *		SPI 	(core lib)
 *		RF24 	(http://maniacbug.github.io/RF24/)
 *		CRC16 	(http://github.com/ronnied/crc16/)
 *		AES 	(https://github.com/Baldanos/pa55ware/tree/master/libraries/AES)
 */
#ifndef _Secure_nRF_h_
#define _Secure_nRF_h_

#include "Arduino.h"
#include <AES.h>
#include <CRC16.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define HWPIN_SPI_CE 2
#define HWPIN_SPI_CS 10

struct Packet {
  byte payload[32];
};

struct AESParams {
	byte key[16];
	byte iv[16];
};

class Secure_nRF
{
	public:
		Secure_nRF(void);
		void begin(void);
		
		void setLocalAESParams(AESParams l);				
		void setDestinationAESParams(AESParams d);

		void setLocalDeviceId(uint64_t id);		
		void setDestinationDeviceId(uint64_t id);	

		void sendMessage(byte* payload);
		bool receiveMessage(byte* payload);		

	protected:
		uint64_t _localDeviceId;
		uint64_t _destinationDeviceId;
		void randomiseIV(byte* iv);

	private:
		AESParams _localAESParams;
		AESParams _destinationAESParams;
		int _payloadSize;
		AES _aes;
		CRC _crc;
		RF24 _nRF;
		
		void _txHeader(unsigned long uuid, byte msgType, byte msgLen, byte* iv);
		void _txPacket(byte* packet);
};

#endif