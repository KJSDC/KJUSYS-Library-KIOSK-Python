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

  ledcSetup(0, 5000, 8);
  ledcAttachPin(BUZZER, 0);

  // Set the default key
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
  while (true) {
    if (Serial.available() > 0) {
      String dataToWrite = Serial.readStringUntil('\n');
      dataToWrite.trim();

      if (dataToWrite.endsWith(":")) {
        dataToWrite.remove(dataToWrite.length() - 1);
        break;
      }

      dataToWrite += "#"; // Add delimiter
      byte blockAddr = 4; // Block address to write to
      MFRC522::StatusCode status;

      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        // Authenticate
        status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid));
        if (status != MFRC522::STATUS_OK) {
          Serial.print(F("PCD_Authenticate() failed: "));
          Serial.println(rfid.GetStatusCodeName(status));
          return;
        }

        // Write data to the block
        byte buffer[18] = {0};
        dataToWrite.getBytes(buffer, 18);
        status = rfid.MIFARE_Write(blockAddr, buffer, 16);
        if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Write() failed: "));
          Serial.println(rfid.GetStatusCodeName(status));
        } else {
          // Buzzer and LED indicators
          digitalWrite(INDICATOR, HIGH);
          // Start the tone using LEDC
          ledcWriteTone(0, 698); // Channel 0, frequency 698 Hz
          delay(50);
          digitalWrite(INDICATOR, LOW);
          // Stop the tone
          ledcWriteTone(0, 0);

          Serial.println(F("Data was written successfully"));
        }

        // Halt PICC and stop encryption on PCD
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
    }
  }
}

void printData() {
  while (true) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      byte blockAddr = 4; // Block address to read from
      byte buffer[18] = {0};
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
        for (byte i = 0; i < 16; i++) {
          if (buffer[i] >= 32 && buffer[i] <= 126) { // Only add printable characters
            readData += (char)buffer[i];
          }
        }
        readData.replace("#", ""); // Remove delimiter

        // Buzzer and LED indicators
        digitalWrite(INDICATOR, HIGH);
        // Start the tone using LEDC
        ledcWriteTone(0, 698); // Channel 0, frequency 698 Hz
        delay(50);
        digitalWrite(INDICATOR, LOW);
        // Stop the tone
        ledcWriteTone(0, 0);

        Serial.println(readData);
      }

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
