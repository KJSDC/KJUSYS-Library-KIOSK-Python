#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 22
#define INDICATOR 13
#define BUZZER 12
#define STATUS 2

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);

  pinMode(INDICATOR, OUTPUT);
  pinMode(STATUS, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  SPI.begin();
  rfid.PCD_Init();
  digitalWrite(STATUS, HIGH);

  // Initialize LEDC peripheral for the buzzer
  ledcSetup(0, 5000, 8); // Channel 0, 5 kHz frequency, 8-bit resolution
  ledcAttachPin(BUZZER, 0); // Attach channel 0 to the buzzer pin

  // Set the key (default key)
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == ';') {
      writeData();
    } else if (command == '/') {
      printData();
    }
  }
}

void writeData() {
  String dataToWrite = "";
  while (true) {
    if (Serial.available() > 0) {
      char incomingChar = Serial.read();
      if (incomingChar == ':') {
        break;
      } else {
        dataToWrite += incomingChar;
      }
    }
  }

  dataToWrite.trim();
  dataToWrite += "#"; // Add delimiter
  byte blockAddr = 4; // Block address to write to
  MFRC522::StatusCode status;

  // Authenticate
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // Write data to the block
  byte buffer[18];
  dataToWrite.getBytes(buffer, 18);
  status = rfid.MIFARE_Write(blockAddr, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
  } else {
    Serial.println(F("Data was written successfully"));
  }

  // Halt PICC and stop encryption on PCD
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void printData() {
  while (true) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      byte blockAddr = 4; // Block address to read from
      byte buffer[18];
      byte size = sizeof(buffer);
      MFRC522::StatusCode status;

      // Authenticate
      status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        return;
      }

      // Read data from the block
      status = rfid.MIFARE_Read(blockAddr, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
      } else {
        String readData = "";
        for (byte i = 0; i < size; i++) {
          readData += (char)buffer[i];
        }
        readData.replace("#", ""); // Remove delimiter
        Serial.println(readData);
      }

      // Halt PICC and stop encryption on PCD
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }

    if (Serial.available() > 0) {
      char command = Serial.read();
      if (command == '\\') {
        break;
      }
    }
  }
}
