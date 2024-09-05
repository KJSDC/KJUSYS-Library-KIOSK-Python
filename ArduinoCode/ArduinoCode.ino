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
#define INDICATOR 3
#define BUZZER 8

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

void setup() {
  Serial.begin(9600);                                             // Initialize serial communication at 9600 baud rate

  // Initialize PN5180 ISO15693 module
  nfc.begin();
  nfc.reset();
  nfc.setupRF();
  Serial.println("Book Scanner Ready...");

  // Initialize MFRC522
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("ID Card Scanner Ready...");

  // Set the default key for MFRC522
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop()
{
  ReadCommand();  // Check for commands from the serial input
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


void readRFID_PN5180() {
  uint8_t uid[8];
  ISO15693ErrorCode rc = nfc.getInventory(uid);
  /*if (ISO15693_EC_OK != rc) {
    Serial.print("Error reading inventory: ");
    Serial.println(nfc.strerror(rc));  
    delay(1000);
    return;
  }*/

  if (!cardDetected) {
    uint8_t blockSize, numBlocks;
    rc = nfc.getSystemInfo(uid, &blockSize, &numBlocks);
    if (ISO15693_EC_OK != rc) {
      delay(1000);
      return;
    }

    cardData = "";
    uint8_t readBuffer[blockSize];
    bool delimiterFound = false;

    for (int no = 0; no < numBlocks; no++) {
      rc = nfc.readSingleBlock(uid, no, readBuffer, blockSize);
      if (ISO15693_EC_OK != rc) {
        delay(1000);
        return;
      }

      for (int i = 0; i < blockSize; i++) {
        if (readBuffer[i] == '#') {
          delimiterFound = true;
          break;
        }
        cardData += (char)readBuffer[i];
      }

      if (delimiterFound) {
        break;
      }
    }

    if (delimiterFound) {
      cardData.trim();  // Remove any trailing spaces
      cardData.remove(cardData.indexOf('#'));  // Remove delimiter
    } else {
      Serial.println(F("Delimiter '#' not found in any block."));
    }
      Serial.println(cardData);  // Print card data
      BData=cardData;

    cardDetected = true;
  } else {
    delay(1000);  // Wait before scanning again
    uint8_t newUid[8];
    ISO15693ErrorCode rc = nfc.getInventory(newUid);
    if (ISO15693_EC_OK != rc || memcmp(newUid, uid, 8) != 0) {
      cardDetected = false;  // Card removed
    }
  }
}

void readRFID_MFRC522() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
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
        IDData=readData;
      }

      Serial.println(readData);  // Print the filtered data

      // Buzzer and LED indicators
      digitalWrite(INDICATOR, HIGH);
      digitalWrite(BUZZER, HIGH);
      delay(50);
      digitalWrite(INDICATOR, LOW);
      digitalWrite(BUZZER, LOW);
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
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

    Serial.print(F("Enter data to store: "));
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
      }

      // Copy the current UID to the lastProcessedUID
      memcpy(lastProcessedUID, uid, sizeof(uid));

    } else {
      Serial.println(F("Operation stopped. Awaiting new command."));
      break;  // Exit loop when stop command is detected
    }
  }
}