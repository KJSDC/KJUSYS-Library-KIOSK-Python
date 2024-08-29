#include <PN5180.h>
#include <PN5180ISO15693.h>

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_AVR_NANO)

#define PN5180_NSS  10
#define PN5180_BUSY 8
#define PN5180_RST  9

#elif defined(ARDUINO_ARCH_ESP32)

#define PN5180_NSS  16
#define PN5180_BUSY 5
#define PN5180_RST  17

#else
#error Please define your pinout here
#endif

PN5180ISO15693 nfc(PN5180_NSS, PN5180_BUSY, PN5180_RST);

void setup() {
  Serial.begin(115200);
  nfc.begin();
  nfc.reset();
  nfc.setupRF();
}

void loop() {
  static bool cardPresent = false;
  uint8_t uid[8];
  ISO15693ErrorCode rc = nfc.getInventory(uid);

  if (ISO15693_EC_OK == rc) {
    if (!cardPresent) {
      cardPresent = true;

      uint8_t blockSize, numBlocks;
      rc = nfc.getSystemInfo(uid, &blockSize, &numBlocks);
      if (ISO15693_EC_OK != rc) {
        Serial.print(F("Error in getSystemInfo: "));
        Serial.println(nfc.strerror(rc));
        return;
      }

      String result = "";
      bool stopReading = false;

      uint8_t readBuffer[blockSize];
      for (int no = 0; no < numBlocks && !stopReading; no++) {
        rc = nfc.readSingleBlock(uid, no, readBuffer, blockSize);
        if (ISO15693_EC_OK == rc) {
          for (int i = 0; i < blockSize; i++) {
            if (readBuffer[i] == '#' || readBuffer[i] == '$') {
              stopReading = true;
              break;
            }
            result += (char)readBuffer[i];
          }
        } else {
          Serial.print(F("Error in readSingleBlock #"));
          Serial.print(no);
          Serial.print(F(": "));
          Serial.println(nfc.strerror(rc));
        }
      }

      if (result.length() > 0) {
        Serial.println(result);
      }
    }
  } else {
    if (cardPresent) {
      cardPresent = false;
    }
  }

  delay(100);
}
