#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include "credentials.h"

// Configuración WiFi
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;


// Crear objeto BME680
Adafruit_BME680 bme;

void setupWiFi() {
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("Dirección IP: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(9600);
    Wire.begin(D2, D1); // SDA, SCL

    // Inicializar WiFi
    setupWiFi();

    // Inicializar el sensor
    if (!bme.begin()) {
        Serial.println("No se encontró el sensor BME680, verifica conexiones!");
        while (1);
    }

    // Configurar el sensor
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C durante 150 ms
}

void loop() {
    if (bme.performReading()) {
        // Verificar conexión WiFi
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("--------------------");
            Serial.print("Temperatura = ");
            Serial.print(bme.temperature);
            Serial.println(" *C");

            Serial.print("Presión = ");
            Serial.print(bme.pressure / 100.0);
            Serial.println(" hPa");

            Serial.print("Humedad = ");
            Serial.print(bme.humidity);
            Serial.println(" %");

            Serial.print("Gas = ");
            Serial.print(bme.gas_resistance / 1000.0);
            Serial.println(" KOhms");
        }
    }
    delay(2000);
}