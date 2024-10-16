#include <PN5180.h>                                                // Basic functions to control the PN5180 NFC module.
#include <PN5180ISO15693.h>                                        // Support for the ISO15693 protocol
#include <SPI.h>                                                   // SPI library for MFRC522
#include <MFRC522.h>                                              // Library for MFRC522 RFID reader

// Define the pins used for interfacing with the PN5180 module
#define PN5180_NSS  10
#define PN5180_BUSY 9
#define PN5180_RST  7

// Define pins for MFRC522
#define SS_PIN 5
#define RST_PIN 6

#define Buzzer 8


// Create instances of PN5180 and MFRC522 objects
PN5180ISO15693 nfc(PN5180_NSS, PN5180_BUSY, PN5180_RST);
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Global variables
String combinedData = "";
String command = "";
unsigned long startTime; // Variable to store the start time
const unsigned long timeout = 3000; 

bool cardDetected = false;
String cardData = "";
String IDData = "";
String BData = "";

uint8_t lastUID[8] = {0};
bool tagInRange = false;
uint8_t lastProcessedUID[8] = {0};
uint8_t lastReadUID[8] = {0};
bool tagOutOfRange = true;

String previousCardData = "";

void setup() {
  Serial.begin(9600);                                             // Initialize serial communication at 9600 baud rate

  // Initialize PN5180 ISO15693 module
  nfc.begin();
  nfc.reset();
  nfc.setupRF();
 // Serial.println("Book Scanner Ready...");

  // Initialize MFRC522
  SPI.begin();
  rfid.PCD_Init();
  pinMode(Buzzer,OUTPUT);
  //Serial.println("ID Card Scanner Ready...");
  digitalWrite(8,HIGH);
  delay(100);
  digitalWrite(8,LOW);

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
      
}

void loop()
{
  ReadCommand();  // Check for commands from the serial input
  //readRFID_MFRC522();
}



void ReadCommand() {
  // Check if there's a command available from the serial input
  if (Serial.available() > 0) {
    command = Serial.readStringUntil('\n');  // Read the command
    command.trim();  // Remove any extra whitespace
    if (command == "W") {
      Serial.println("Writing to PN5180...");
      writeRFID_PN5180();  // Call function to write data to PN5180 RFID tag
    } else if (command == "B") {
      startTime = millis();
      //Serial.println("Place Book...");
      while(BData=="")
      {
        if (millis() - startTime > timeout)
         {
            Serial.println("No Response...");
            break;
         }
        readRFID_PN5180();
      }
      BData="";
        // Call function to read data from PN5180
    } else if (command == "I") {
      startTime = millis();
      //Serial.println("Tap ID...");
      while(IDData=="")
      {
         if (millis() - startTime > timeout)
         {
            Serial.println("No Response...");
            break;
         }
        readRFID_MFRC522();
      }
      IDData="";
        // Call function to read data from MFRC522
    } else {
      Serial.println("Unknown command.");  // For any other inputs
    }
  }
}


void readRFID_PN5180() 
{
  // Static variables to store card information
  static uint8_t uid[8];               // Store the UID of the detected card
  static bool cardDetected = false;    // Track whether a card is currently detected

  // Try to detect a new card
  uint8_t newUid[8];
  ISO15693ErrorCode rc = nfc.getInventory(newUid);

  if (rc == ISO15693_EC_OK) 
  {
    // If a card is detected, always process it
    cardDetected = true;  // Set the flag to indicate card has been processed
    memcpy(uid, newUid, 8);  // Update the current card UID

    // Get card system info
    uint8_t blockSize, numBlocks;
    rc = nfc.getSystemInfo(uid, &blockSize, &numBlocks);
    if (ISO15693_EC_OK != rc) 
    {
      return;  // If getting system info fails, just return and retry
    }

    // Read the card data
    String cardData = "";
    uint8_t readBuffer[blockSize];
    bool delimiterFound = false;

    for (int no = 0; no < numBlocks; no++) 
    {
      rc = nfc.readSingleBlock(uid, no, readBuffer, blockSize);
      if (ISO15693_EC_OK != rc) 
      {
        return;  // If reading block fails, exit and retry
      }

      // Process the read data block by block
      for (int i = 0; i < blockSize; i++) 
      {
        if (readBuffer[i] == '#') 
        {
          delimiterFound = true;
          break;  // Stop reading if delimiter is found
        }
        cardData += (char)readBuffer[i];
      }

      if (delimiterFound) 
      {
        break;
      }
    }

    if (delimiterFound) 
    {
      cardData.trim();  // Clean up any trailing spaces
      cardData.remove(cardData.indexOf('#'));  // Remove the delimiter

      // Process the card data
      Serial.println(cardData);  // Print the card data
      digitalWrite(Buzzer, HIGH);  // Activate buzzer
      delay(250);  // Short delay for buzzer feedback
      digitalWrite(Buzzer, LOW);  // Deactivate buzzer
      delay(100); 
      digitalWrite(Buzzer, HIGH);  // Activate buzzer
      delay(250);  // Short delay for buzzer feedback
      digitalWrite(Buzzer, LOW);  // Deactivate buzzer
       delimiterFound = false;
       delay(2500);
    } 
    else 
    {
      Serial.println(F("Delimiter '#' not found in any block."));
    }
  } 
 

  // Small delay for stability and CPU load management
  delay(50);  // A small delay to avoid overwhelming the microcontroller
}










void readRFID_MFRC522() {
  // Variable to keep track of whether a card is detected
  static bool cardDetected = false;
  
  // If a card is detected
  if (rfid.PICC_IsNewCardPresent()) {
    if (rfid.PICC_ReadCardSerial()) {
      if (!cardDetected) {
        // Card just appeared for the first time, perform authentication and reading
        byte blockAddr = 4;  // Block address to read from
        byte buffer[18] = {0};
        byte size = sizeof(buffer);
        MFRC522::StatusCode status;

        // Authenticate
        status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
        if (status != MFRC522::STATUS_OK) {
          Serial.print(F("PCD_Authenticate() failed: "));
          Serial.println(rfid.GetStatusCodeName(status));
          rfid.PICC_HaltA();
          rfid.PCD_StopCrypto1();
          return;
        }

        // Read data from the block
        status = rfid.MIFARE_Read(blockAddr, buffer, &size);
        if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Read() failed: "));
          Serial.println(rfid.GetStatusCodeName(status));
        } else {
          // Print data from the block
          String readData = "";
          for (byte i = 0; i < 16; i++) {
            if (buffer[i] == '#' || buffer[i] == '$') {
              break; // Stop reading when '#' or '$' is encountered
            }
            if (buffer[i] >= 32 && buffer[i] <= 126) { // Only add printable characters
              readData += (char)buffer[i];
            }
          }
          IDData = readData;
          Serial.println(readData);  // Print the filtered data
          digitalWrite(Buzzer, HIGH); // Activate buzzer for feedback
          delay(50);  // Short delay for buzzer sound
          digitalWrite(Buzzer, LOW);  // Turn off buzzer
        }

        // Mark that a card is detected and authentication is successful
        cardDetected = true;
      } // End of card detection
    }
  } else if (cardDetected) {
    // If no card is detected and we previously detected one, halt and reset
    //Serial.println(F("Card removed!"));
    rfid.PICC_HaltA();  // Halt the card communication
    rfid.PCD_StopCrypto1();  // Stop crypto and reset
    cardDetected = false;  // Reset the card detected flag
    //Serial.println(F("Ready for new card."));
  }
}
void writebookID() {
  while (true) {
    uint8_t uid[8];
    ISO15693ErrorCode rc = nfc.getInventory(uid);

    // Check if the current UID matches the last processed UID
    bool isSameUID = true;
    for (int i = 0; i < 8; i++) {
      if (uid[i] != lastProcessedUID[i]) {
        isSameUID = false;
        break;
      }
    }

    if (isSameUID) {
      continue;  // Skip the rest of the loop and check again
    }

    uint8_t blockSize, numBlocks;
    rc = nfc.getSystemInfo(uid, &blockSize, &numBlocks);
    if (ISO15693_EC_OK != rc) {
      Serial.print(F("Error in getSystemInfo: "));
      Serial.println(nfc.strerror(rc));
      return;
    }

    // Beep the buzzer to signal the start of the process
    digitalWrite(Buzzer,HIGH);
    delay(50);
    digitalWrite(Buzzer,LOW);
    delay(40);
    digitalWrite(Buzzer,HIGH);
    delay(50);
    digitalWrite(Buzzer,LOW);

    // Wait for the user to input data
    while (Serial.available() == 0) {}

    String inputData = Serial.readStringUntil('\n');
    inputData.trim();  // Remove any leading or trailing spaces

    // Check if the inputData is not empty and add the "#" delimiter
    if (inputData != "_") {
      // Add a '#' delimiter to the inputData if it doesn't already have one
      if (!inputData.endsWith("#")) {
        inputData += "#";  // Add delimiter to the end
      }

      int dataLength = inputData.length();
      int numBlocksRequired = (dataLength + blockSize - 1) / blockSize;  // Calculate how many blocks are needed

      if (numBlocksRequired > numBlocks) {
        Serial.println(F("Error: Not enough blocks available to store data."));
        return;
      }

      // Write the data to the RFID card in blocks
      for (int blockIndex = 0; blockIndex < numBlocksRequired; ++blockIndex) {
        uint8_t writeBuffer[blockSize];
        int startIndex = blockIndex * blockSize;
        int endIndex = min(startIndex + blockSize, dataLength);
        int length = endIndex - startIndex;

        // Copy data into the writeBuffer, padding with zeros if necessary
        for (int i = 0; i < blockSize; ++i) {
          writeBuffer[i] = (i < length) ? inputData[startIndex + i] : 0x00;  // Pad with zeros if input is shorter than block size
        }

        rc = nfc.writeSingleBlock(uid, blockIndex, writeBuffer, blockSize);
        if (ISO15693_EC_OK != rc) {
          Serial.print(F("Error in writeSingleBlock "));
          Serial.print(blockIndex);
          Serial.print(F(": "));
          Serial.println(nfc.strerror(rc));
          return;
        }

        Serial.println(F("Successfully stored block..."));
        digitalWrite(Buzzer,HIGH);
        delay(1000);
        digitalWrite(Buzzer,LOW);
      }

      // Copy the current UID to the lastProcessedUID to avoid reprocessing
      memcpy(lastProcessedUID, uid, sizeof(uid));

    } else {
      Serial.println(F("Operation stopped. Awaiting new command."));
      break;  // Exit loop when stop command is detected
    }
  }
}


void writeRFID_PN5180() {
  while (true) {
    uint8_t uid[8];
    ISO15693ErrorCode rc = nfc.getInventory(uid);

    // Check if the current UID matches the last processed UID
    bool isSameUID = true;
    for (int i = 0; i < 8; i++) {
      if (uid[i] != lastProcessedUID[i]) {
        isSameUID = false;
        break;
      }
    }

    if (isSameUID) {
      continue;  // Skip the rest of the loop and check again
    }

    uint8_t blockSize, numBlocks;
    rc = nfc.getSystemInfo(uid, &blockSize, &numBlocks);
    if (ISO15693_EC_OK != rc) {
      Serial.print(F("Error in getSystemInfo: "));
      Serial.println(nfc.strerror(rc));
      return;
    }

    //Serial.print(F("Enter data to store: "));
    digitalWrite(Buzzer,HIGH);
    delay(50);
    digitalWrite(Buzzer,LOW);
    delay(40);
    digitalWrite(Buzzer,HIGH);
    delay(50);
    digitalWrite(Buzzer,LOW);
    while (Serial.available() == 0) {}

    String inputData = Serial.readStringUntil('\n');
    inputData.trim(); 

    if (inputData != "_") {
      inputData += "#";  // Add delimiter to the end
      int dataLength = inputData.length();
      int numBlocksRequired = (dataLength + blockSize - 1) / blockSize;

      if (numBlocksRequired > numBlocks) {
        Serial.println(F("Error: Not enough blocks available to store data."));
        return;
      }

      for (int blockIndex = 0; blockIndex < numBlocksRequired; ++blockIndex) {
        uint8_t writeBuffer[blockSize];
        int startIndex = blockIndex * blockSize;
        int endIndex = min(startIndex + blockSize, dataLength);
        int length = endIndex - startIndex;

        for (int i = 0; i < blockSize; ++i) {
          writeBuffer[i] = (i < length) ? inputData[startIndex + i] : 0x00;  // Pad with zeros if input is shorter than block size
        }

        rc = nfc.writeSingleBlock(uid, blockIndex, writeBuffer, blockSize);
        if (ISO15693_EC_OK != rc) {
          Serial.print(F("Error in writeSingleBlock "));
          Serial.print(blockIndex);
          Serial.print(F(": "));
          Serial.println(nfc.strerror(rc));
          return;
        }

        Serial.println(F("Successfully stored...."));
        digitalWrite(Buzzer,HIGH);
        delay(1000);
        digitalWrite(Buzzer,LOW);
      }

      // Copy the current UID to the lastProcessedUID
      memcpy(lastProcessedUID, uid, sizeof(uid));

    } else {
      Serial.println(F("Operation stopped. Awaiting new command."));
      break;  // Exit loop when stop command is detected
    }
  }
}