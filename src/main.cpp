#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

AsyncWebServer server(80);
Preferences preferences;
const gpio_num_t bootButtonPin = GPIO_NUM_0;

void setupWiFiSoftAP() {
    WiFi.softAP("ESP32-Config");
    Serial.println("Started as SoftAP. Connect to ESP32 Wi-Fi configuration page at http://192.168.4.1");
}

void setupWiFiClient() {
    preferences.begin("wifi-config", false);
    String savedSSID = preferences.getString("ssid", "");
    String savedPass = preferences.getString("pass", "");
    preferences.end();

    if (savedSSID.length() > 0 && savedPass.length() > 0) {
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.println("Connecting to Wi-Fi...");
        }

        Serial.print("Connected to Wi-Fi, IP: ");
        Serial.println(WiFi.localIP());
    } else {
        setupWiFiSoftAP();
    }
}

void handleRoot(AsyncWebServerRequest *request) {
    String html = "<html><body>";
    html += "<h1>ESP32 Wi-Fi Configuration</h1>";
    html += "<form method='post' action='/save'>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "Pass: <input type='text' name='pass'><br>";
    html += "<input type='submit' value='Save and Reboot'>";
    html += "</form></body></html>";

    request->send(200, "text/html", html);
}

void saveWiFiCredentials(const String &ssid, const String &pass) {
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    preferences.end();
}

void handleSave(AsyncWebServerRequest *request) {

    if (request->method() != HTTP_POST) {
        request->send(405, "text/plain", "Method Not Allowed");
        return;
    }

    String ssid = request->arg("ssid");
    String pass = request->arg("pass");

    saveWiFiCredentials(ssid, pass);
    request->send(200, "text/html", "Configuration saved. The ESP32 will now reboot.");
    delay(2000);
    ESP.restart();
}

void clearNVSData() {
    Serial.println("Boot button pressed. Clearing NVS memory...");
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    Serial.println("NVS memory cleared.");
}

void bootButtonISR(void *arg) {
    clearNVSData();
    ESP.restart();
}

void configureBootButton() {
    gpio_set_direction(bootButtonPin, GPIO_MODE_INPUT);
    gpio_set_intr_type(bootButtonPin, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(bootButtonPin, bootButtonISR, NULL);
}

void setup() {
    Serial.begin(115200);

    configureBootButton();
    setupWiFiClient();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
}

void loop() {

}
