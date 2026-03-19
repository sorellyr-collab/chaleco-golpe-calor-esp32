#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <TinyGPSPlus.h>

MAX30105 particleSensor;
TinyGPSPlus gps;

// =========================
// MAX30102
// =========================
const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute = 0.0;
int beatAvg = 0;

// =========================
// GPS
// =========================
HardwareSerial gpsSerial(2);  // UART2
static const int GPS_RX_PIN = 16; // ESP32 recibe aquí -> GPS TX
static const int GPS_TX_PIN = 17; // ESP32 transmite aquí -> GPS RX
static const uint32_t GPS_BAUD = 9600;

// =========================
// Tiempos
// =========================
uint32_t lastPrint = 0;

void setupMax30102() {
  Wire.begin(21, 22);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("ERROR: MAX30102 no encontrado. Verifica cableado.");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("MAX30102 detectado correctamente.");

  particleSensor.setup(
    60,   // brillo LED
    4,    // promedio de muestras
    2,    // modo LED: RED + IR
    100,  // sample rate
    411,  // pulse width
    4096  // adc range
  );

  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);
}

void setupGPS() {
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS inicializado en UART2.");
}

void updateHeartRate(long irValue) {
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
}

void updateGPS() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

void printTelemetry(long irValue, long redValue) {
  bool fingerDetected = irValue > 50000;

  Serial.print("IR=");
  Serial.print(irValue);

  Serial.print(" | RED=");
  Serial.print(redValue);

  Serial.print(" | DEDO=");
  Serial.print(fingerDetected ? "SI" : "NO");

  Serial.print(" | BPM=");
  Serial.print(beatsPerMinute, 1);

  Serial.print(" | BPM_PROM=");
  Serial.print(beatAvg);

  Serial.print(" | GPS=");

  if (gps.location.isValid()) {
    Serial.print("OK");
    Serial.print(" | LAT=");
    Serial.print(gps.location.lat(), 6);
    Serial.print(" | LON=");
    Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print("SIN_FIX");
    Serial.print(" | LAT=--");
    Serial.print(" | LON=--");
  }

  Serial.print(" | SAT=");
  if (gps.satellites.isValid()) {
    Serial.print(gps.satellites.value());
  } else {
    Serial.print("--");
  }

  Serial.print(" | HDOP=");
  if (gps.hdop.isValid()) {
    Serial.print(gps.hdop.hdop());
  } else {
    Serial.print("--");
  }

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==============================================");
  Serial.println(" TermoGuard - BPM + GPS");
  Serial.println(" ESP32 + MAX30102 + NEO-6M");
  Serial.println("==============================================");

  setupMax30102();
  setupGPS();

  Serial.println("Coloca el dedo en el MAX30102.");
  Serial.println("Lleva el GPS cerca de ventana o exterior.");
}

void loop() {
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  updateHeartRate(irValue);
  updateGPS();

  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    printTelemetry(irValue, redValue);
  }

  delay(10);
}
