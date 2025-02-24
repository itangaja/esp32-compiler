#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>

const char* ssid = "Iroschool";       // Ganti dengan SSID WiFi Anda
const char* password = "D1866VBV";    // Ganti dengan password WiFi Anda

IPAddress local_IP(192, 168, 200, 80);  // IP tetap untuk ESP32
IPAddress gateway(192, 168, 200, 1);     // Gateway (biasanya IP router)
IPAddress subnet(255, 255, 255, 0);  

AsyncWebServer server(80);

const char* uploadPage = R"rawliteral(
  <html>
  <head>
      <script>
          function checkStatus() {
              fetch('/status')
              .then(response => response.text())
              .then(data => {
                  document.getElementById('status').innerHTML = data;
              });
          }
          setInterval(checkStatus, 2000);
      </script>
  </head>
  <body>
      <form method='POST' action='/update' enctype='multipart/form-data'>
          <input type='file' name='update'>
          <input type='submit' value='Upload Firmware'>
      </form>
      <p id="status">Menunggu update...</p>
  </body>
  </html>
)rawliteral";


void setup() {
    Serial.begin(921600);

    // Konfigurasi IP statis sebelum koneksi ke WiFi
    if (!WiFi.config(local_IP, gateway, subnet)) {
        Serial.println("Gagal mengatur IP statis!");
    }

    WiFi.begin(ssid, password);
    Serial.print("Menghubungkan ke WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nTerhubung ke WiFi!");
    Serial.print("IP Address ESP32: ");
    Serial.println(WiFi.localIP());

    pinMode(2, OUTPUT); // LED_BUILTIN di ESP32

    // Halaman upload
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", uploadPage);
    });

    // Upload firmware
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", Update.hasError() ? "Gagal" : "Sukses! Restarting...");
        delay(1000);
        ESP.restart();
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index) {
            Serial.printf("Upload Start: %s\n", filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        }
        if (!Update.hasError()) {
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }
        }
        if (final) {
            if (Update.end(true)) {
                Serial.printf("Update Success: %u bytes\n", index + len);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.begin();
}

void loop() {
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(2, LOW);
    delay(500);
}
