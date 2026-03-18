#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 8; // cantidad de muestras para promediar BPM
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute = 0.0;
int beatAvg = 0;

uint32_t lastPrint = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("====================================");
  Serial.println(" Chaleco inteligente - Validacion BPM");
  Serial.println(" MAX30102 + ESP32");
  Serial.println("====================================");

  Wire.begin(21, 22);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("ERROR: MAX30102 no encontrado. Verifica cableado.");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("MAX30102 detectado correctamente.");

  // Configuración recomendada para dedo/prueba de BPM
  particleSensor.setup(
    60,   // brillo LED: 0-255
    4,    // promedio de muestras: 1,2,4,8,16,32
    2,    // modo LED: 2 = Red + IR
    100,  // sample rate: 50,100,200,400,800,1000,1600,3200
    411,  // pulse width: 69,118,215,411
    4096  // adc range: 2048,4096,8192,16384
  );

  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0); // verde apagado

  Serial.println("Coloca el dedo sobre el sensor y mantenlo quieto.");
  Serial.println("Esperando lecturas...");
}

void loop() {
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  bool fingerDetected = irValue > 50000;

  if (fingerDetected) {
    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60.0 / (delta / 1000.0);

      if (beatsPerMinute > 35 && beatsPerMinute < 220) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        long sum = 0;
        for (byte i = 0; i < RATE_SIZE; i++) {
          sum += rates[i];
        }
        beatAvg = sum / RATE_SIZE;

        Serial.println("LATIDO detectado");
      }
    }
  } else {
    beatsPerMinute = 0;
    beatAvg = 0;
  }

  if (millis() - lastPrint >= 500) {
    lastPrint = millis();

    Serial.print("IR=");
    Serial.print(irValue);

    Serial.print(" | RED=");
    Serial.print(redValue);

    Serial.print(" | DEDO=");
    Serial.print(fingerDetected ? "SI" : "NO");

    Serial.print(" | BPM=");
    Serial.print(beatsPerMinute, 1);

    Serial.print(" | BPM_PROM=");
    Serial.println(beatAvg);
  }

  delay(10);
}
