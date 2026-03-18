#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute = 0.0;
int beatAvg = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("======================================");
  Serial.println("Iniciando MAX30102 para pulso cardiaco");
  Serial.println("======================================");

  Wire.begin(21, 22);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 no detectado. Revisa conexiones.");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("MAX30102 detectado correctamente.");
  Serial.println("Coloca el dedo sobre el sensor y mantenlo quieto.");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);
}

void loop() {
  long irValue = particleSensor.getIR();

  if (irValue < 5000) {
    Serial.print("IR: ");
    Serial.print(irValue);
    Serial.println(" | Sin dedo detectado");
    delay(300);
    return;
  }

  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60.0 / (delta / 1000.0);

    if (beatsPerMinute > 20 && beatsPerMinute < 255) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;
    }
  }

  Serial.print("IR: ");
  Serial.print(irValue);
  Serial.print(" | BPM: ");
  Serial.print(beatsPerMinute, 1);
  Serial.print(" | Promedio BPM: ");
  Serial.println(beatAvg);

  delay(20);
}
