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

// Credenciales WiFi
const char* ssid = "Packtur";
const char* password = "Julieta2022.";

// Pines de control de la bomba
const int IN1 = 15;
const int IN2 = 2;

// Pines del sensor ultrasónico
const int Trigger = 13;
const int Echo = 12;

// Configuración de niveles
const int DISTANCIA_VACIO = 24;  // Estanque vacío en cm
const int DISTANCIA_LLENO = 15;  // Estanque lleno en cm

// API de ThingSpeak
const char* writeApiUrl = "http://api.thingspeak.com/update";  // URL para enviar datos
const char* readApiEndpoint = "http://api.thingspeak.com/channels/2794877/fields/1/last.json?api_key=UBC8TC9O5HMP0J21";
const char* apiKey = "6OOFLKLG096IP2Y8"; 

void setup() {
  Serial.begin(115200);
  
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);

  // Bomba apagada por defecto
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  Serial.println("Bomba apagada al iniciar.");

  // Conexión a Wi-Fi
  Serial.println("Conectando al Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado.");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    // Medir nivel del estanque usando el sensor ultrasónico
    digitalWrite(Trigger, LOW);
    delayMicroseconds(2);
    digitalWrite(Trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trigger, LOW);

    long duration = pulseIn(Echo, HIGH);
    float distance = (duration / 2.0) * 0.0343; // Convertir duración en cm

    // Calcular porcentaje del nivel del estanque
    // A mayor distancia, menor nivel de agua (porcentaje bajo)
    float porcentaje = map(distance, DISTANCIA_LLENO, DISTANCIA_VACIO, 0, 100); 
    porcentaje = constrain(porcentaje, 0, 100);  // Asegurarse de que el porcentaje esté entre 0 y 100

    // Mostrar datos en el monitor serie
    Serial.print("Distancia medida: ");
    Serial.print(distance);
    Serial.println(" cm");
    Serial.print("Porcentaje del Estanque: ");
    Serial.print(porcentaje);
    Serial.println("%");

    // Enviar datos del porcentaje a ThingSpeak (field2)
    String url = String(writeApiUrl) + "?api_key=" + apiKey + "&field2=" + String(porcentaje, 2);
    http.begin(client, url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Porcentaje enviado correctamente a ThingSpeak.");
    } else {
      Serial.println("Error al enviar los datos del porcentaje.");
    }
    http.end();

    // Leer estado de la bomba desde ThingSpeak (field1)
    http.begin(client, readApiEndpoint);
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
        String fieldValue = doc["field1"] | "0";  // Leer field1 (bomba)
        Serial.print("Valor recibido (field1): ");
        Serial.println(fieldValue);

        // **Controlar la bomba manualmente**
        if (fieldValue == "1") {
          Serial.println("Encendiendo la bomba. Agua fluyendo...");
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          delay(1000);  // Ajustar duración del flujo de agua según necesidad
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
      // Seguridad: Apagar la bomba si hay error
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
    http.end();
  } else {
    Serial.println("Error: Wi-Fi desconectado. Intentando reconectar...");
    WiFi.begin(ssid, password);  // Intentar reconexión Wi-Fi
  }

  delay(15000);  // Espera 15 segundos antes de la siguiente iteración
}
