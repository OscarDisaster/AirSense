#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_BME680.h>
#include <ESPSupabaseOscar.h>
#include <ESP_SupabaseOscar_Realtime.h>
#include "credentials.h" // Archivo con tus credenciales

// Objetos globales
Adafruit_BME680 bme;
Supabase db;
SupabaseRealtime realtime;

// Constantes
const uint32_t SEND_DATA_INTERVAL = 2000; // Intervalo para enviar datos en milisegundos
const int LED_PIN = 2; // Pin del LED (puedes cambiarlo según tu configuración)
const int CONNECTED_BLINK_INTERVAL = 1000; // Intervalo de parpadeo lento (conectado)
const int DISCONNECTED_BLINK_INTERVAL = 200; // Intervalo de parpadeo rápido (desconectado)

// Variables globales
unsigned long lastSendTime = 0; // Última vez que se enviaron datos
unsigned long lastBlinkTime = 0; // Última vez que el LED parpadeó
bool ledState = LOW; // Estado actual del LED

// Función para inicializar el sensor BME680
void initializeSensor() {
    Serial.println("[BME680] Iniciando sensor...");
    if (!bme.begin()) {
        Serial.println("[BME680] ¡Sensor no encontrado!");
        while (true) {} // Detener el programa si el sensor no está disponible
    }

    Serial.println("[BME680] Sensor inicializado correctamente");
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);
}

// Callback para manejar mensajes de Realtime
void realtimeCallback(String message) {
    Serial.println("[Realtime] Mensaje recibido:");
    Serial.println(message);
}

// Función para hacer parpadear el LED
void blinkLED(int interval) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= interval) {
        lastBlinkTime = currentTime;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
    }
}

// Función para configurar la conexión WiFi
void connectWiFi() {
    Serial.println("Conectando a WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        blinkLED(DISCONNECTED_BLINK_INTERVAL); // Parpadeo rápido mientras se conecta
        delay(500);
        Serial.print("...");
    }

    Serial.println("\nConectado a WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// Función para enviar datos a Supabase
void sendDataToSupabase() {
    if (!bme.performReading()) {
        Serial.println("[BME680] Error al leer los datos del sensor.");
        return;
    }

    StaticJsonDocument<256> doc; // Aumentar el tamaño del documento JSON
    doc["device_id"] = DEVICE_ID;
    doc["temperature"] = bme.temperature;
    doc["pressure"] = bme.pressure / 100.0;
    doc["humidity"] = bme.humidity;
    doc["gas"] = bme.gas_resistance / 1000.0;
    doc["ssid"] = WiFi.SSID(); // Agregar el SSID actual
    doc["refresh_time"] = SEND_DATA_INTERVAL / 1000; // Agregar el tiempo de refresco en segundos

    String jsonString;
    serializeJson(doc, jsonString);

    Serial.println("[HTTP] Enviando datos a Supabase...");
    Serial.println(jsonString);

    int responseCode = db.insert("sensor_readings", jsonString, false);

    // Mejor manejo de errores en la respuesta HTTP
    if (responseCode == -100) {
        Serial.println("[HTTP] Error: No se pudo conectar al servidor Supabase.");
    } else if (responseCode < 0) {
        Serial.println("[HTTP] Error desconocido: Código " + String(responseCode));
    } else if (responseCode >= 200 && responseCode < 300) {
        Serial.println("[HTTP] Datos enviados correctamente. Código de respuesta: " + String(responseCode));
    } else {
        Serial.println("[HTTP] Error inesperado: Código " + String(responseCode));
    }

    db.urlQuery_reset();
}

// Configuración inicial
void setup() {
    Serial.begin(9600);
    Wire.begin(21, 22); // SDA, SCL
    pinMode(LED_PIN, OUTPUT); // Configurar el pin del LED como salida

    // Inicializar hardware y conexiones
    initializeSensor();
    connectWiFi();

    // Configurar conexión a Supabase REST y Realtime
    WiFiClientSecure &client = db.getClient();
    client.setInsecure(); // Desactivar SSL

    db.begin(SUPABASE_URL, SUPABASE_API_KEY);
    realtime.begin(SUPABASE_URL, SUPABASE_API_KEY, realtimeCallback);

    // Agregar un listener para los cambios en la tabla "sensor_readings"
    realtime.addChangesListener("sensor_readings", "INSERT", "public", "");

    // Iniciar la conexión de Realtime
    realtime.listen();

    // Primera inserción de prueba
    Serial.println("[HTTP] Probando inserción HTTP...");
    sendDataToSupabase();
    Serial.println("[HTTP] Prueba completada");
}

// Bucle principal
void loop() {
    // Mantener viva la conexión Realtime
    realtime.loop();

    // Enviar datos periódicamente
    if (millis() - lastSendTime >= SEND_DATA_INTERVAL) {
        lastSendTime = millis();
        sendDataToSupabase();
    }

    // Parpadeo del LED según el estado de conexión WiFi
    if (WiFi.status() == WL_CONNECTED) {
        blinkLED(CONNECTED_BLINK_INTERVAL); // Parpadeo lento cuando está conectado
    } else {
        blinkLED(DISCONNECTED_BLINK_INTERVAL); // Parpadeo rápido cuando está desconectado
    }
}