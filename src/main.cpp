#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <TinyGPSPlus.h>

#define I2C_SDA 21
#define I2C_SCL 22

#define GPS_RX 16   // ESP32 recibe desde TX del NEO-6M
#define GPS_TX 17   // ESP32 transmite hacia RX del NEO-6M

MAX30105 particleSensor;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute = 0.0;
int beatAvg = 0;

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

void setupMax30102() {
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("ERROR: MAX30102 no detectado");
    while (true) {
      delay(1000);
    }
  }

  particleSensor.setup(
    0x1F,  // powerLevel
    4,     // sampleAverage
    2,     // ledMode: Red + IR
    400,   // sampleRate
    411,   // pulseWidth
    4096   // adcRange
  );

  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println("MAX30102 detectado correctamente");
}

void setupGps() {
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS iniciado en UART1");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Chaleco inteligente: MAX30102 + GPS ===");

  setupMax30102();
  setupGps();
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

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
  Serial.print(" | BPM=");
  Serial.print(beatsPerMinute, 1);
  Serial.print(" | BPM_PROM=");
  Serial.print(beatAvg);
  Serial.print(" | ");

  printGpsData();

  delay(1000);
}
