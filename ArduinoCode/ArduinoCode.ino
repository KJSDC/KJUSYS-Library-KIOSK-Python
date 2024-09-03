#include <PN5180.h>                                                // Basic functions to control the PN5180 NFC module.
#include <PN5180ISO15693.h>                                        // Support for the ISO15693 protocol

//##############################################################################
// Define the pins used for interfacing with the PN5180 module
//##############################################################################

#define PN5180_NSS  10
#define PN5180_BUSY 9
#define PN5180_RST  7


PN5180ISO15693 nfc(PN5180_NSS, PN5180_BUSY, PN5180_RST);

String combinedData = "";


String command = "";

uint8_t lastUID[8] = {0};
bool tagInRange = false;
uint8_t lastProcessedUID[8] = {0};
uint8_t lastReadUID[8] = {0};
bool tagOutOfRange = true;

//###################################################################################################################################################
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////       Setup starts here.....     //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//###################################################################################################################################################

void setup() {
  Serial.begin(115200);
  nfc.begin();                                                             // Initialize PN5180ISO15693
  nfc.reset();                                                             // Reset PN5180ISO15693
  nfc.setupRF();                                                           // Enable RF field
}

void loop() {
  if (Serial.available() > 0) {
    command = Serial.readStringUntil('\n');
    command.trim();  
  }

  if (command.equalsIgnoreCase("-")) {
    readRFID();
    
    if (command.equalsIgnoreCase("_")){
      Serial.println(F("Operation stopped. Awaiting new command."));
      command = "";  
    }
  } else if (command.equalsIgnoreCase(";")) {
    writeRFID();
    if (command.equalsIgnoreCase("_")){
      Serial.println(F("Operation stopped. Awaiting new command."));
      command = "";  
    }
  } else if (command.equalsIgnoreCase("_")) {
    Serial.println(F("Operation stopped. Awaiting new command."));
    command = "";  
  }
}


void readRFID() {
  uint8_t uid[8];
  ISO15693ErrorCode rc = nfc.getInventory(uid);
  if (ISO15693_EC_OK != rc) {
    // If no tag is found
    tagOutOfRange = true;
    return;
  }

  // Check if the current UID matches the last read UID
  bool isSameUID = true;
  for (int i = 0; i < 8; i++) {
    if (uid[i] != lastReadUID[i]) {
      isSameUID = false;
      break;
    }
  }

  if (isSameUID && !tagOutOfRange) {
    return;
  }

  // If a new UID is detected
  uint8_t blockSize, numBlocks;                                                             
     rc = nfc.getSystemInfo(uid, &blockSize, &numBlocks);
  if (ISO15693_EC_OK != rc) {
    Serial.print(F("Error in getSystemInfo: "));
    Serial.println(nfc.strerror(rc));
    return;
  }

  uint8_t readBuffer[blockSize];
  combinedData = "";  

  for (int no = 0; no < numBlocks; no++) {
    rc = nfc.readSingleBlock(uid, no, readBuffer, blockSize);
    if (ISO15693_EC_OK == rc) {
      for (int i = 0; i < blockSize; i++) {
        if (readBuffer[i] == 0x23) {                            // 0x23 is the hexadecimal value for '#'
          no = numBlocks;  
          break;                               
        } else {
          combinedData += (char)readBuffer[i];    
        }
      }
    } else {
      Serial.print(F("Error in readSingleBlock #"));
      Serial.print(no);
      Serial.print(F(": "));
      Serial.println(nfc.strerror(rc));
      break;
    }
  }
  Serial.println(combinedData);

  // Copy the current UID to the lastReadUID
  memcpy(lastReadUID, uid, sizeof(uid));
  
  tagOutOfRange = false;                           // Reset tag Out Of Range since the tag is in range and successfully read
}


void writeRFID() {
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
      // Serial.println(F("Same UID detected, waiting for the tag to go out of range."));
      // delay(2000);  // Add a delay to prevent spamming the serial output
      continue;  // Skip the rest of the loop and check again
    }

    // If a new UID is detected, proceed with the write operation
    // Serial.print(F("Inventory successful, UID="));
    // for (int i = 0; i < 8; i++) {
    //   Serial.print(uid[7 - i], HEX);  
    //   if (i < 7) Serial.print(":");
    // }
    // Serial.println();

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

    if(inputData != "_"){
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

      // ##########################################################################
      // ########  can be replace with buzzer   ###################################
                    Serial.println(F(" successfully stored...."));
      // ##########################################################################
      }

      // for (int blockIndex = 0; blockIndex < numBlocksRequired; ++blockIndex) {
      //   uint8_t readBuffer[blockSize];
      //   rc = nfc.readSingleBlock(uid, blockIndex, readBuffer, blockSize);
      //   if (ISO15693_EC_OK == rc) {
      //     Serial.print(F("Data in block #"));
      //     Serial.print(blockIndex);
      //     Serial.print(F(": "));
      //     for (int i = 0; i < blockSize; i++) {
      //       Serial.print((char)readBuffer[i]);
      //     }
      //     Serial.println();
      //   } else {
      //     Serial.print(F("Error in readSingleBlock "));
      //     Serial.print(blockIndex);
      //     Serial.print(F(": "));
      //     Serial.println(nfc.strerror(rc));
      //   }
      // }

      // Copy the current UID to the lastProcessedUID
      memcpy(lastProcessedUID, uid, sizeof(uid));

      //Serial.println(F("UID cleared after successful operation."));
    }
    else{
      Serial.println(F("Operation stopped. Awaiting new command."));
      command = ""; 
      break;
    }

    delay(2000);
  }
}
