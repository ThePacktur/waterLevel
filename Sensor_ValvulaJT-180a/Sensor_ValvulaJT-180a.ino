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
char ssid[] = "Packtur";          // Nombre de tu red WiFi
char pass[] = "Julieta2022.";     // Contraseña de tu red WiFi
WiFiClient client;

// Configuración de ThingSpeak
unsigned long myChannelNumber = SECRET_CH_ID;       // ID del canal en ThingSpeak
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;    // API Key de escritura

// Pines del sensor ultrasónico
const int Trigger = 13;  // Pin TRIG del sensor
const int Echo = 12;     // Pin ECHO del sensor

// Pin de control de la bomba
const int bombaPin = 5;  // Pin digital para controlar la bomba (usa un transistor o relé)

// Niveles de agua
const int DISTANCIA_VACIO = 24;  // Distancia considerada como estanque vacío (cm)
const int DISTANCIA_LLENO = 14;  // Distancia considerada como estanque lleno (cm)

void setup() {
  Serial.begin(115200);           // Inicializa la comunicación serial
  pinMode(Trigger, OUTPUT);       // Configura Trigger como salida
  pinMode(Echo, INPUT);           // Configura Echo como entrada
  pinMode(bombaPin, OUTPUT);      // Configura el pin de la bomba como salida
  digitalWrite(Trigger, LOW);     // Inicializa Trigger en LOW
  digitalWrite(bombaPin, LOW);    // Apaga la bomba al inicio

  WiFi.mode(WIFI_STA);            // Configura el ESP8266 como estación
  ThingSpeak.begin(client);       // Inicializa ThingSpeak
}

void loop() {
  // Conexión a WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Intentando conectar a WiFi: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConexión WiFi establecida.");
  }

  // Medición de distancia con el sensor HC-SR04
  long duration;  // Duración del pulso
  long distance;  // Distancia calculada en cm

  // Enviar pulso al Trigger
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger, LOW);

  // Leer el tiempo del pulso en el Echo
  duration = pulseIn(Echo, HIGH);

  // Calcular distancia (cm)
  distance = duration * 0.034 / 2;

  Serial.print("Distancia medida: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Determinar nivel del estanque y controlar la bomba
  if (distance <= DISTANCIA_VACIO) {
    Serial.println("Estado: Estanque vacío. Encendiendo la bomba.");
    digitalWrite(bombaPin, HIGH); // Encender la bomba
  } else if (distance >= DISTANCIA_LLENO) {
    Serial.println("Estado: Estanque lleno. Apagando la bomba.");
    digitalWrite(bombaPin, LOW);  // Apagar la bomba
  } else {
    Serial.println("Estado: Nivel intermedio. Manteniendo la bomba apagada.");
    digitalWrite(bombaPin, LOW);  // Mantener la bomba apagada
  }

  // Enviar datos a ThingSpeak
  int status = ThingSpeak.writeField(myChannelNumber, 1, distance, myWriteAPIKey);
  if (status == 200) {
    Serial.println("Actualización exitosa en ThingSpeak.");
  } else {
    Serial.println("Error al actualizar el canal. Código HTTP: " + String(status));
  }

  delay(1000);  // Espera 1 segundo para enviar el siguiente dato
}
