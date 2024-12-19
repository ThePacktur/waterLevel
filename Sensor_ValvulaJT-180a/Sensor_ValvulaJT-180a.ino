/*
  WriteSingleField
  
  Description: Writes a value to a channel on ThingSpeak every 20 seconds.
  
  Hardware: ESP8266 based boards
  
  !!! IMPORTANT - Modify the secrets.h file for this project with your network connection and ThingSpeak channel details. !!!
  
  Note:
  - Requires ESP8266WiFi library and ESP8622 board add-on. See https://github.com/esp8266/Arduino for details.
  - Select the target hardware from the Tools->Board menu
  - This example is written for a network using WPA encryption. For WEP or WPA, change the WiFi.begin() call accordingly.
  
  ThingSpeak ( https://www.thingspeak.com ) is an analytic IoT platform service that allows you to aggregate, visualize, and 
  analyze live data streams in the cloud. Visit https://www.thingspeak.com to sign up for a free account and create a channel.  
  
  Documentation for the ThingSpeak Communication Library for Arduino is in the README.md folder where the library was installed.
  See https://www.mathworks.com/help/thingspeak/index.html for the full ThingSpeak documentation.
  
  For licensing information, see the accompanying license file.
  
  Copyright 2020, The MathWorks, Inc.
*/
#include <ESP8266WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h" // Siempre incluir después de otros encabezados


// Configuración de red WiFi
WiFiClient client;

// Pines del sensor ultrasónico
const int Trigger = 13;  // Pin TRIG del sensor
const int Echo = 12;     // Pin ECHO del sensor

// Pin de control de la bomba
const int bombaPin = 5;  // Pin digital para controlar la bomba (usa un transistor o relé)

// Niveles de agua
const int DISTANCIA_VACIO = 24;  // Distancia considerada como estanque vacío (cm)
const int DISTANCIA_LLENO = 20;  // Distancia considerada como estanque lleno (cm)

// Channel details
unsigned long channelID = SECRET_CH_ID;
unsigned int fieldsensor = 1;
unsigned int fieldBomba = 2;

// Estado inicial de la bomba
bool bombaEncendida = false; // Variable para rastrear si la bomba está encendida

void setup() {
  Serial.begin(115200);           // Inicializa la comunicación serial
  pinMode(Trigger, OUTPUT);       // Configura Trigger como salida
  pinMode(Echo, INPUT);           // Configura Echo como entrada
  pinMode(bombaPin, OUTPUT);      // Configura el pin de la bomba como salida
  digitalWrite(Trigger, LOW);     // Inicializa Trigger en LOW
  digitalWrite(bombaPin, LOW);    // Apaga la bomba al inicio

  WiFi.mode(WIFI_STA);            // Configura el ESP8266 como estación
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConexión WiFi establecida.");
  ThingSpeak.begin(client);       // Inicializa ThingSpeak
}

long medirDistanciaPromedio() {
  long suma = 0;
  int mediciones = 5; // Número de mediciones para promediar

  for (int i = 0; i < mediciones; i++) {
    digitalWrite(Trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trigger, LOW);

    long duration = pulseIn(Echo, HIGH);
    long distance = duration * 0.034 / 2;

    suma += distance;
    delay(10); // Pequeña pausa entre mediciones
  }

  return suma / mediciones; // Retorna la distancia promedio
}

void loop() {
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Desconectado de WiFi. Reconectando...");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(SECRET_SSID, SECRET_PASS);
      delay(5000);
    }
    Serial.println("Reconectado a WiFi.");
  }

  // Leer valores desde ThingSpeak
  long valor_Sensor = ThingSpeak.readLongField(channelID, fieldsensor, SECRET_READ_APIKEY);
  long valor_Bomba = ThingSpeak.readLongField(channelID, fieldBomba, SECRET_READ_APIKEY);

  Serial.print("Valor Sensor: ");
  Serial.println(valor_Sensor);
  Serial.print("Valor Bomba: ");
  Serial.println(valor_Bomba);

  // Escribir valores en los pines
  digitalWrite(Trigger, valor_Sensor ? HIGH : LOW);
  digitalWrite(bombaPin, valor_Bomba ? HIGH : LOW);

  // Medir distancia
  long distancia = medirDistanciaPromedio();
  Serial.print("Distancia medida: ");
  Serial.print(distancia);
  Serial.println(" cm");

  // Controlar el estado de la bomba basado en la distancia
  if (distancia >= DISTANCIA_VACIO) {
    bombaEncendida = true;
  } else if (distancia <= DISTANCIA_LLENO) {
    bombaEncendida = false;
  }

  // Actualizar los valores en ThingSpeak
  ThingSpeak.setField(fieldsensor, distancia);
  ThingSpeak.setField(fieldBomba, bombaEncendida ? 1 : 0);

  int status = ThingSpeak.writeFields(channelID, SECRET_WRITE_APIKEY);
  if (status == 200) {
    Serial.println("Actualización exitosa en ThingSpeak.");
  } else {
    Serial.print("Error al enviar datos. Código HTTP: ");
    Serial.println(status);
  }

  delay(15000);  // Espera 15 segundos para enviar el siguiente dato
}
