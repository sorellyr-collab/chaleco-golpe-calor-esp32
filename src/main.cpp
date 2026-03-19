#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>

#define I2C_SDA 21
#define I2C_SCL 22

MAX30105 particleSensor;

const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute = 0.0;
int beatAvg = 0;

bool maxConnected = false;

bool initMax30102() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("ERROR: MAX30102 no detectado");
    return false;
  }

  byte ledBrightness = 0x3F;
  byte sampleAverage = 4;
  byte ledMode = 2;      // Red + IR
  int sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  particleSensor.setPulseAmplitudeRed(0x2A);
  particleSensor.setPulseAmplitudeIR(0x2A);
  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println("MAX30102 detectado correctamente");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Chaleco inteligente: MAX30102 ===");

  maxConnected = initMax30102();
}

void loop() {
  if (!maxConnected) {
    Serial.println("ERROR: sensor MAX30102 no disponible");
    delay(1000);
    return;
  }

  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  bool fingerPresent = irValue > 50000;

  if (!fingerPresent) {
    beatsPerMinute = 0.0;
    beatAvg = 0;

    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(" | RED=");
    Serial.print(redValue);
    Serial.println(" | SIN_DEDO | BPM=0.0 | BPM_PROM=0");

    delay(250);
    return;
  }

  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    if (delta > 0) {
      beatsPerMinute = 60.0 / (delta / 1000.0);

      if (beatsPerMinute > 40 && beatsPerMinute < 220) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte i = 0; i < RATE_SIZE; i++) {
          beatAvg += rates[i];
        }
        beatAvg /= RATE_SIZE;
      }
    }
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(" | RED=");
  Serial.print(redValue);
  Serial.print(" | DEDO=SI");
  Serial.print(" | BPM=");
  Serial.print(beatsPerMinute, 1);
  Serial.print(" | BPM_PROM=");
  Serial.println(beatAvg);

  delay(50);
}
