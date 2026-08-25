# Aviónica de Carga Útil - Equipo Astrolatas

Este repositorio contiene el código fuente de la computadora de vuelo desarrollada por el equipo **Astrolatas** para la categoría "Max Q" (Cohete de Combustible Sólido). 

El sistema está basado en un microcontrolador **ESP32-S3** y se encarga de la adquisición de datos ambientales e inerciales, la estimación del estado de vuelo y la transmisión de telemetría en tiempo real a la estación terrena mediante radiofrecuencia (LoRa).



## Hardware y Sensores

El sistema integra los siguientes componentes principales:

*   **Microcontrolador:** ESP32-S3.
*   **Comunicaciones:** Módulo LoRa SX1262 (Banda 915 MHz).
*   **Sensor Ambiental:** BME280 (Presión, Temperatura, Humedad).
*   **Sensor Inercial (IMU):** MPU6050 (Aceleración y Giroscopio).
*   **Geolocalización:** Módulo GNSS u-blox NEO-8MN.
*   **Alimentación:** Batería LiPo 1S (3.7V, 300mAh) regulada a 3.3V mediante convertidor Buck MP1584EN.

### Diagrama de Conexiones (Pinout Base)

| Componente | Pin ESP32-S3 | Bus / Función |
| :--- | :--- | :--- |
| **BME280 & MPU6050** | PINES POR DEFECTO | `SDA` e `I2C SCL` |
| **NEO-8MN (GPS)** | Pin 16 | `RX` (Conecta al TX del GPS) |
| **NEO-8MN (GPS)** | Pin 17 | `TX` (Conecta al RX del GPS) |
| **SX1262 (LoRa)** | Pin 10 | `NSS` / `CS` (SPI Chip Select) |
| **SX1262 (LoRa)** | Pin 9 | `DIO1` (Interrupción) |
| **SX1262 (LoRa)** | Pin 11 | `RESET` |
| **SX1262 (LoRa)** | Pin 12 | `BUSY` |
| **SX1262 (LoRa)** | PINES POR DEFECTO | `MOSI`, `MISO`, `SCK` |

---

## Dependencias (Librerías)

Para compilar este código en el IDE de Arduino, es necesario instalar las siguientes librerías desde el Gestor de Librerías:

1.  `Adafruit BME280 Library` (por Adafruit)
2.  `Adafruit MPU6050` (por Adafruit)
3.  `TinyGPSPlus` (por Mikal Hart)
4.  `RadioLib` (por Jan Gromes) - *Para el manejo del SX1262*

---

## Máquina de Estados de Vuelo

El código opera bajo un modelo de máquina de estados finitos que monitorea continuamente la dinámica del cohete para inferir su etapa de vuelo actual:

1.  **ESPERA:** El cohete está en la rampa. Se toman muestras para promediar la presión de referencia a nivel del suelo ($P_0$) y establecer la altitud relativa a 0 m. Se esperan fuerzas G > 20 para confirmar el despegue.
2.  **ASCENSO:** El motor está en combustión o el cohete va por inercia hacia el cielo. El MPU6050 busca el cruce por cero de la aceleración en el eje Z (estado de ingravidez / apogeo).
3.  **DESCENSO:** La carga de retraso ha accionado el despliegue del sistema de recuperación. El barómetro detecta una caída sostenida en la altitud.
4.  **ATERRIZAJE:** El sistema detecta que la variación de altitud es nula durante más de 30 segundos consecutivos. La misión concluye y la telemetría se enfoca en transmitir las coordenadas GPS para la recuperación.

---

## Trama de Telemetría

Los datos son procesados, empaquetados en formato CSV (texto delimitado) y transmitidos hacia la estación terrena con la siguiente estructura:

`ALT_INFO: Tiempo=X.XX | Pres=XXXXX.X | Temp=XX.X | Hum=XX.X | AcelZ=X.XX | Alt_Est=XXX.X | Estado=X | Lat=XX.XXXXXX | Lon=-XX.XXXXXX`

---

## 💻 Instrucciones de Uso

1.  Clona este repositorio o descarga el archivo `.ino`.
2.  Abre el archivo en Arduino IDE.
3.  Selecciona la placa **ESP32S3 Dev Module** en `Herramientas > Placa`.
4.  Asegúrate de que las conexiones físicas de los pines coincidan con los definidos en el encabezado del código.
5.  Compila y sube el código a la placa.
6.  Abre el Monitor Serie a **115200 baudios** para verificar la inicialización de los sensores en frío antes de activar la interfaz LoRa.
