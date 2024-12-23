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
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "ThingSpeak.h" // Siempre incluir después de otros encabezados

// Configuración Wi-Fi
const char* ssid = "Packtur";
const char* password = "Julieta2022.";

// Pines de control de la bomba mecánica
const int IN1 = 2;  // Pin digital para encender la bomba
const int IN2 = 15; // Pin digital para apagar la bomba

// Pines del sensor ultrasónico
const int Trigger = 13;
const int Echo = 12;

// Configuración de niveles
const int DISTANCIA_VACIO = 24;  // Estanque vacío en cm
const int DISTANCIA_LLENO = 15;  // Estanque lleno en cm

// Configuración ThingSpeak
const char* writeApiUrl = "http://api.thingspeak.com/update";
const char* readApiUrl = "https://api.thingspeak.com/channels/2782709/fields/1.json?api_key=Y110M1MLLT8Q74CR";
const char* apiKey = "EW65PRKCOEEZ50GI";

// Variables para medir nivel del estanque
WiFiClient client;             // Cliente para HTTP
WiFiClientSecure secureClient; // Cliente para HTTPS
HTTPClient http;


unsigned long tiempoUltimaMedicion = 0;
unsigned long intervaloMedicion = 15000; // Medir cada 15 segundos

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  // Conexión Wi-Fi
  Serial.println("Conectando al Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado.");

  // Configuración del cliente seguro
  secureClient.setInsecure(); // Desactiva la verificación de certificados (no recomendado en producción)
}


void loop() {
  unsigned long tiempoActual = millis();
  if (WiFi.status() == WL_CONNECTED) {
    // Medir nivel del estanque usando el sensor ultrasónico
    digitalWrite(Trigger, LOW);
    delayMicroseconds(2);
    digitalWrite(Trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trigger, LOW);

    long duration = pulseIn(Echo, HIGH);
    float distance = (duration / 2.0) * 0.0343; // Convertir duración en cm

    // Calcular porcentaje del nivel del estanque
    float porcentaje = map(distance, DISTANCIA_VACIO, DISTANCIA_LLENO, 0, 100);
    porcentaje = constrain(porcentaje, 0, 100);

    // Mostrar datos en el monitor serie
    Serial.print("Distancia medida: ");
    Serial.print(distance);
    Serial.println(" cm");
    Serial.print("Porcentaje del Estanque: ");
    Serial.print(porcentaje);
    Serial.println("%");

    // Enviar datos del porcentaje a ThingSpeak
    String url = String(writeApiUrl) + "?api_key=" + apiKey + "&field1=" + String(porcentaje, 2);
    http.begin(client, url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Porcentaje enviado correctamente a ThingSpeak.");
    } else {
      Serial.println("Error al enviar los datos del porcentaje.");
    }
    http.end();

    // Leer estado de la bomba desde ThingSpeak (field2)
    http.begin(client, readApiUrl);
    int responseCode = http.GET();

    if (responseCode > 0) {
      String payload = http.getString();
      Serial.println("Respuesta de ThingSpeak: " + payload);

      // Parsear el JSON
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("Error al parsear JSON: ");
        Serial.println(error.c_str());
      } else {
        String fieldValue = doc["field2"] | "0";  // Leer field2
        Serial.print("Valor recibido (field2): ");
        Serial.println(fieldValue);

        // Controlar la bomba manualmente
        if (fieldValue == "1") {
          Serial.println("Encendiendo la bomba. Agua fluyendo...");
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          delay(1000);
        } else if (fieldValue == "0") {
          Serial.println("Apagando la bomba.");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
        } else {
          Serial.println("Valor inválido, apagando la bomba por seguridad.");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
        }
      }
    } else {
      Serial.println("Error al obtener datos de ThingSpeak.");
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
    http.end();
  } else {
    Serial.println("Error: Wi-Fi desconectado. Intentando reconectar...");
    WiFi.begin(ssid, password);
  }

  delay(15000); // Espera 15 segundos antes de la siguiente iteración
}
