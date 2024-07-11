#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
#define INDICATOR 3
#define BUZZER 8

char card[22];

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {

    Serial.begin(9600);

    pinMode(INDICATOR, OUTPUT);
    pinMode(BUZZER, OUTPUT);

    SPI.begin();
    rfid.PCD_Init();
}

void loop()
{
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {

    // Clear the card array
    memset(card, 0, sizeof(card));

    // Process the UID
    for (byte i = 0; i < rfid.uid.size; i++)
    {
      // Convert each byte to hex and store in the card array
      sprintf(&card[i * 3], "%02X ", rfid.uid.uidByte[i]);
    }

    card[rfid.uid.size * 3 - 1] = '\0';

    // Print the card array for verification
    Serial.println(card);

    // Buzzer and LED indicators
    digitalWrite(INDICATOR, HIGH);
    // Start the tone using LEDC
    digitalWrite(BUZZER, HIGH);
    delay(50);
    digitalWrite(BUZZER, LOW);
    digitalWrite(INDICATOR, LOW);

    // Halt PICC and stop encryption on PCD
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}
