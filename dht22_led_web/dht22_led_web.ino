#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <ElegantOTA.h>  // Include ElegantOTA library
#include "credentials.h" 

// Pins
const int ledPin = D2;
const int dhtPin = D4;

// DHT sensor setup
#define DHTTYPE DHT22
DHT dht(dhtPin, DHTTYPE);

// Server instance
AsyncWebServer server(80);

// Global variables
bool timerActive = false;
unsigned long timerStart = 0;
String ledState = "OFF";
float temperature = 0.0;
float humidity = 0.0;

// Update DHT sensor values
void readDHT() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  }
}

void setup() {
  // Initialize serial
  Serial.begin(115200);

  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Initialize DHT sensor
  dht.begin();

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize ElegantOTA
  ElegantOTA.begin(&server);  // Enable OTA updates via the server

  // Configure server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE HTML><html><head>";
    html += "<meta charset=\"UTF-8\">"; // Ensure the page uses UTF-8 encoding
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }";
    html += "button { padding: 15px 30px; font-size: 18px; margin: 10px; cursor: pointer; }";
    html += "button:hover { background-color: #ddd; }";
    html += ".status { font-size: 20px; margin-top: 20px; color: #333; }";
    html += "</style></head><body>";
    html += "<h1>ESP8266 LED & DHT22 Control</h1>";
    html += "<p><a href=\"/LED=ON\"><button style=\"background-color: #4CAF50; color: white;\">Turn LED ON</button></a></p>";
    html += "<p><a href=\"/LED=OFF\"><button style=\"background-color: #f44336; color: white;\">Turn LED OFF</button></a></p>";
    html += "<p><a href=\"/LED=ON30\"><button style=\"background-color: #2196F3; color: white;\">Turn LED ON for 30 Seconds</button></a></p>";
    html += "<p class=\"status\">LED Status: <strong>" + ledState + "</strong></p>";
    html += "<h2>Temperature & Humidity</h2>";
    html += "<p>Temperature: " + String(temperature) + "&deg;C</p>"; // Use &deg; for degree symbol
    html += "<p>Humidity: " + String(humidity) + "%</p>";
    html += "<script>setTimeout(() => { location.reload(); }, 5000);</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/LED=ON", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(ledPin, HIGH);
    ledState = "ON";
    timerActive = false;
    request->redirect("/");
  });

  server.on("/LED=OFF", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(ledPin, LOW);
    ledState = "OFF";
    timerActive = false;
    request->redirect("/");
  });

  server.on("/LED=ON30", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(ledPin, HIGH);
    ledState = "ON (30 seconds)";
    timerActive = true;
    timerStart = millis();
    request->redirect("/");
  });

  // Start server
  server.begin();
}

void loop() {
  // Handle 30-second timer
  if (timerActive && millis() - timerStart >= 30000) {
    digitalWrite(ledPin, LOW);
    ledState = "OFF";
    timerActive = false;
    Serial.println("Timer completed, LED turned off.");
  }

  // Update DHT values every 5 seconds
  static unsigned long lastDHTRead = 0;
  if (millis() - lastDHTRead >= 5000) {
    readDHT();
    lastDHTRead = millis();
  }

  // Handle OTA requests
  ElegantOTA.loop();  // Process OTA requests
}
