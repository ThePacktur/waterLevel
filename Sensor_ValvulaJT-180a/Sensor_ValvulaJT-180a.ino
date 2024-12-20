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
#include "secrets.h"    // Archivo con las credenciales y configuración

// Pines de control de la bomba mecánica
const int IN1 = 9;  // Pin digital para encender la bomba
const int IN2 = 10; // Pin digital para apagar la bomba

// Pines del sensor ultrasónico (opcional si también se mide nivel del agua)
const int Trigger = 13;
const int Echo = 12;

// Configuración de niveles
const int DISTANCIA_VACIO = 24;  // Estanque vacío
const int DISTANCIA_LLENO = 15;  // Estanque lleno

// Variables para medir nivel del estanque
WiFiClient client;
HTTPClient http;

unsigned long tiempoUltimaMedicion = 0;
unsigned long intervaloMedicion = 15000; // Medir cada 15 segundos

// URL de ThingSpeak para leer y escribir
String readApiUrl = "http://api.thingspeak.com/channels/" + String(SECRET_CH_ID) +
                    "/fields/2.json?api_key=" + String(SECRET_READ_APIKEY) + "&results=1";

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
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado.");
  ThingSpeak.begin(client);
}

// Función para medir nivel del estanque (promedio)
long medirDistanciaPromedio() {
  long suma = 0;
  int mediciones = 5;

  for (int i = 0; i < mediciones; i++) {
    digitalWrite(Trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trigger, LOW);

    long duracion = pulseIn(Echo, HIGH);
    long distancia = duracion * 0.034 / 2;
    suma += distancia;
    delay(10);
  }
  return suma / mediciones;
}

// Leer estado de la bomba desde ThingSpeak (field2)
void leerEstadoBomba() {
  HTTPClient http;

  if (WiFi.status() == WL_CONNECTED) {
    http.begin(client, readApiUrl); //Usar el objeto WIFICLient y la URL
    int httpCode = http.GET(); //REaliza una peticion GET

    if (httpCode > 0) { //si la respuesta es exitosa
      String payload = http.getString(); //Obtiene la respuesta
      Serial.println("Respuesta de ThingSpeak: " + payload);

      // Parsear el JSON
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("Error al parsear JSON: ");
        Serial.println(error.c_str());
      } else {
        String fieldValue = doc["feeds"][0]["field2"] | "0"; // Leer el último valor de field2
        Serial.print("Valor recibido (field2): ");
        Serial.println(fieldValue);

        // Controlar la bomba manualmente
        if (fieldValue == "1") {
          Serial.println("Encendiendo la bomba...");
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
        } else if (fieldValue == "0") {
          Serial.println("Apagando la bomba...");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
        } else {
          Serial.println("Valor inválido. Apagando bomba por seguridad.");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
        }
      }
    } else {
      Serial.println("Error al obtener datos de ThingSpeak.");
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW); // Seguridad: Apagar la bomba si hay error
    }
    http.end();
  } else {
    Serial.println("Wi-Fi desconectado. Intentando reconectar...");
    WiFi.begin(SECRET_SSID, SECRET_PASS);
  }
}

// Escribir datos en ThingSpeak
void escribirDatosThingSpeak(int nivel, int estadoBomba) {
  ThingSpeak.setField(1, nivel);              // Escribir nivel del estanque en field1
  ThingSpeak.setField(2, estadoBomba);        // Escribir estado de la bomba en field2

  int status = ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_APIKEY);
  if (status == 200) {
    Serial.println("Datos enviados correctamente a ThingSpeak.");
  } else {
    Serial.print("Error al enviar datos. Código HTTP: ");
    Serial.println(status);
  }
}

void loop() {
  unsigned long tiempoActual = millis();

  // Leer estado de la bomba desde ThingSpeak
  leerEstadoBomba();

  // Cada intervalo, medir nivel del estanque y enviar datos
  if (tiempoActual - tiempoUltimaMedicion >= intervaloMedicion) {
    tiempoUltimaMedicion = tiempoActual;

    long distancia = medirDistanciaPromedio();
    int nivelPorcentaje = map(distancia, DISTANCIA_VACIO, DISTANCIA_LLENO, 0, 100);
    nivelPorcentaje = constrain(nivelPorcentaje, 0, 100);

    Serial.print("Nivel del estanque: ");
    Serial.print(nivelPorcentaje);
    Serial.println("%");

    // Obtener estado de la bomba actual
    int estadoBomba = (digitalRead(IN1) == HIGH) ? 1 : 0;

    // Enviar datos a ThingSpeak
    escribirDatosThingSpeak(nivelPorcentaje, estadoBomba);
  }

  delay(15000); // Esperar antes de la próxima iteración
}
