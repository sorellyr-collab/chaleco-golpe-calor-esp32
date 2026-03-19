#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <TinyGPSPlus.h>

#define I2C_SDA 21
#define I2C_SCL 22

#define GPS_RX 16
#define GPS_TX 17

MAX30105 particleSensor;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

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

  // Configuración más estable para detección de latido
  byte ledBrightness = 0x3F;   // 0x00 a 0xFF
  byte sampleAverage = 4;
  byte ledMode = 2;            // Red + IR
  int sampleRate = 100;        // 100 Hz
  int pulseWidth = 411;        // 411 us
  int adcRange = 4096;         // 4096 nA

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

void setupGps() {
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS iniciado en UART1");
}

void printGpsData() {
  if (gps.location.isValid()) {
    Serial.print("LAT: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(" | LON: ");
    Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print("LAT/LON: esperando fix");
  }

  Serial.print(" | SAT: ");
  if (gps.satellites.isValid()) {
    Serial.print(gps.satellites.value());
  } else {
    Serial.print("N/A");
  }

  Serial.print(" | ALT(m): ");
  if (gps.altitude.isValid()) {
    Serial.print(gps.altitude.meters());
  } else {
    Serial.print("N/A");
  }

  Serial.print(" | SPD(km/h): ");
  if (gps.speed.isValid()) {
    Serial.print(gps.speed.kmph());
  } else {
    Serial.print("N/A");
  }

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Chaleco inteligente: MAX30102 + GPS ===");

  maxConnected = initMax30102();
  setupGps();
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (!maxConnected) {
    Serial.print("IR=0 | RED=0 | BPM=0.0 | BPM_PROM=0 | ");
    printGpsData();
    delay(1000);
    return;
  }

  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  // Detección simple de dedo presente
  bool fingerPresent = irValue > 50000;

  if (!fingerPresent) {
    beatsPerMinute = 0.0;
    beatAvg = 0;

    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(" | RED=");
    Serial.print(redValue);
    Serial.print(" | SIN_DEDO | BPM=0.0 | BPM_PROM=0 | ");
    printGpsData();

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
  Serial.print(beatAvg);
  Serial.print(" | ");

  printGpsData();

  delay(50);
}
