#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <TinyGPSPlus.h>
#include <RadioLib.h>

// Definición de pines para el ESP32-S3
#define RX_GPS 16
#define TX_GPS 17
#define LORA_CS 10
#define LORA_DIO1 9
#define LORA_RESET 11
#define LORA_BUSY 12

// Instancias de los sensores
Adafruit_BME280 bme;     // Sensor de Presión, Temperatura y Humedad
Adafruit_MPU6050 mpu;    // Acelerómetro y Giroscopio
TinyGPSPlus gps;         // Módulo GPS NEO-8MN
HardwareSerial gpsSerial(1); // UART para GPS

// Instancia del módulo LoRa SX1262
SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

// Estados de la misión
enum EstadoMision {
  ESPERA,
  ASCENSO,
  DESCENSO,
  ATERRIZAJE
};

EstadoMision estadoActual = ESPERA;

// Variables globales para la telemetría
float presion_ref = 0.0;
float altura_max = 0.0;
unsigned long tiempoUltimoMovimiento = 0;
float ultimaAltitud = 0.0;
unsigned long tiempoInicio = 0;

void setup() {
  Serial.begin(115200);
  
  // Inicialización de GPS
  gpsSerial.begin(9600, SERIAL_8N1, RX_GPS, TX_GPS);

  // Inicialización de I2C para BME280 y MPU6050
  Wire.begin();

  if (!bme.begin(0x76)) {
    Serial.println("Error: No se encuentra el sensor BME280");
  } else {
    // Tomar presión base antes del lanzamiento
    presion_ref = bme.readPressure() / 100.0F; 
  }

  if (!mpu.begin()) {
    Serial.println("Error: No se encuentra el sensor MPU6050");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G); // Rango ampliado para soportar el despegue
  }

  // Inicialización del LoRa SX1262 a 915 MHz
  if (lora.begin(915.0, 125.0, 7, 5, 0x12, 22) != RADIOLIB_ERR_NONE) {
    Serial.println("Error: Inicialización LoRa SX1262 fallida");
  }

  tiempoInicio = millis();
  Serial.println("Sistema inicializado. Estado: ESPERA");
}

void loop() {
  // 1. Lectura de Sensores
  sensors_event_t a, g, temp_mpu;
  mpu.getEvent(&a, &g, &temp_mpu);
  
  float presion = bme.readPressure() / 100.0F;
  float temperatura = bme.readTemperature();
  float humedad = bme.readHumidity();
  float altitud = bme.readAltitude(presion_ref);
  
  // Actualizar altura máxima
  if (altitud > altura_max) altura_max = altitud;

  // Lectura GPS
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // 2. Máquina de Estados (Lógica de Control)
  unsigned long tiempoActual = millis();
  float aceleracionTotal = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));

  switch (estadoActual) {
    case ESPERA:
      // Detección de lanzamiento: Cambio abrupto de aceleración
      if (aceleracionTotal > 20.0) { // Umbral de aceleración (ajustar según pruebas)
        estadoActual = ASCENSO;
        Serial.println("Lanzamiento detectado. Estado: ASCENSO");
      }
      break;

    case ASCENSO:
      // Detección de apogeo / liberación de carga de retraso
      // La aceleración en Z se aproxima a cero en el apogeo/caída libre
      if (a.acceleration.z < 2.0 && a.acceleration.z > -2.0) { 
        estadoActual = DESCENSO;
        Serial.println("Apogeo / Desacople detectado. Estado: DESCENSO");
      }
      break;

    case DESCENSO:
      // Detección de aterrizaje: Sin cambio de posición/altitud durante 30 segundos
      if (abs(altitud - ultimaAltitud) > 0.5) {
        tiempoUltimoMovimiento = tiempoActual; // Sigue en movimiento
        ultimaAltitud = altitud;
      }
      if (tiempoActual - tiempoUltimoMovimiento > 30000) { // 30 segundos
        estadoActual = ATERRIZAJE;
        Serial.println("Aterrizaje confirmado. Estado: ATERRIZAJE");
      }
      break;

    case ATERRIZAJE:
      // La misión concluyó, solo se transmite ubicación para recuperación
      break;
  }

  // 3. Formateo y Transmisión de Telemetría (Trama CSV)
  String trama = "ALT_INFO: Tiempo=" + String((tiempoActual - tiempoInicio) / 1000.0) +
                 " | Pres=" + String(presion) + 
                 " | Temp=" + String(temperatura) + 
                 " | Hum=" + String(humedad) + 
                 " | AcelZ=" + String(a.acceleration.z) + 
                 " | Alt_Est=" + String(altitud) + 
                 " | Estado=" + String(estadoActual) + 
                 " | Lat=" + String(gps.location.lat(), 6) + 
                 " | Lon=" + String(gps.location.lng(), 6);

  // Enviar datos vía LoRa SX1262
  lora.transmit(trama);
  
  // Imprimir por consola para depuración
  Serial.println(trama);

  // Frecuencia de muestreo (Ajustar según necesidad de transmisión)
  delay(100); 
}