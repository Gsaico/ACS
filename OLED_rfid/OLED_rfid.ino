/*
 * Dump block 0 of a MIFARE RFID card using a RFID-RC522 reader
 * Uses MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT. 
 ----------------------------------------------------------------------------- 
 * Pin layout should be as follows:
 * Signal     Pin              Pin               Pin
 *            Arduino Uno      Arduino Mega      MFRC522 board
 * ------------------------------------------------------------
 * Reset      9                5                 RST
 * SPI SS     10               53                SDA
 * SPI MOSI   11               52                MOSI
 * SPI MISO   12               51                MISO
 * SPI SCK    13               50                SCK
 *
 * Hardware required:
 * Arduino
 * PCD (Proximity Coupling Device): NXP MFRC522 Contactless Reader IC
 * PICC (Proximity Integrated Circuit Card): A card or tag using the ISO 14443A interface, eg Mifare or NTAG203.
 * The reader can be found on eBay for around 5 dollars. Search for "mf-rc522" on ebay.com. 
 */

unsigned char storedID[2][4] = { { 0xA3, 0xB1, 0xD0, 0xF7 } , { 0x03, 0xCC, 0xC9, 0xDE  } };
char* names[] = { "Skip", "David"};

#include <SPI.h>
#include <MFRC522.h>
#include "U8glib.h"

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 


#define SS_PIN 10    //Arduino Uno
#define RST_PIN 8
MFRC522 mfrc522(SS_PIN, RST_PIN);        // Create MFRC522 instance.

void draw_Home(void) {
  u8g.firstPage();  
  do {
    // graphic commands to redraw the complete screen should be placed here  
    u8g.setFont(u8g_font_unifont);
    u8g.drawStr( 24, 10, "Gold Coast");
    u8g.drawStr( 28, 28, "TECHSPACE");

    u8g.drawStr( 10, 62, "Printer Online");
  } 
  while( u8g.nextPage() ); 

}

void setup() {
  Serial.begin(9600);        // Initialize serial communications with the PC
  SPI.begin();                // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card
  Serial.println("Print block 0 of a MIFARE PICC ");

  u8g.setColorIndex(1);         // Set mode to B/W screen  
  draw_Home();

}

void loop() {

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())    return;

  unsigned char id[4];
  Serial.print("Card UID:");    //Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    id[i] = mfrc522.uid.uidByte[i];
  } 
  Serial.println();

  int stored = -1;
  for(int i = 0; i < 2; i++){  //2 = Number of Stored ID's
    if(id[0] == storedID[i][0]){
      if(id[1] == storedID[i][1]){
        if(id[2] == storedID[i][2]){
          if(id[3] == storedID[i][3]){
            stored = i;
          }
        }
      }
    }
  }
  if(stored >= 0) {
    Serial.println("Matched");
    Serial.println(names[stored]);
    u8g.firstPage();  
    do {
      u8g.setFont(u8g_font_unifont);    //u8g.setFont(u8g_font_osb21);
      u8g.drawStr( 24, 10, "Welcome");
      u8g.drawStr( 28, 28, names[stored]);
    } 
    while( u8g.nextPage() );                    
  } 
  else {
    Serial.println("No Match");        
    u8g.firstPage();  
    do {
      u8g.drawStr( 24, 10, "NO MATCH");
      u8g.drawStr( 28, 28, "FOUND");
      u8g.setCursorPos(10, 62);
      for (byte i = 0; i < 4; i++) {
       Serial.print(id[i], HEX);
       }            
    } 
    while( u8g.nextPage() );  
    delay(2000);
    draw_Home();
  }



  Serial.print(" PICC type: ");   // Dump PICC type
  byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  byte buffer[18];  
  byte block  = 0;
  byte status;
  //Serial.println("Authenticating using key A...");
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("PCD_Authenticate() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Read block
  byte byteCount = sizeof(buffer);
  status = mfrc522.MIFARE_Read(block, buffer, &byteCount);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("MIFARE_Read() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  else  // Dump data
  for (byte index = 0; index < 16; index++) {
    Serial.print(buffer[index] < 0x10 ? " 0" : " ");
    Serial.print(buffer[index], HEX);
    if ((index % 4) == 3) Serial.print(" ");
  }
  Serial.println(" ");
  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD

}




