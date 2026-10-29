#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "secrets.h"

#define PIN_LED 23
#define OLED_ADDR 0x3C

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

float lux = 0;
int umbralOn = 50;
int umbralOff = 150;
bool lampara = false;
bool primeraLectura = true;
unsigned long ultimaConsulta = 0;

void mostrar(const String &l2, const String &l3) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("LUZ INTELIGENTE");
  display.setCursor(0, 20);
  display.println(l2);
  display.setCursor(0, 40);
  display.println(l3);
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Wire.begin(21, 22);
    if (!display.begin(OLED_ADDR, true)) {
    Serial.println("No se encontro la OLED");
}
  mostrar("Conectando WiFi...", "");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  mostrar("Conectando a", "Firebase...");
}

void loop() {
  if (millis() - ultimaConsulta < 1000) return;
  ultimaConsulta = millis();

  if (!Firebase.ready()) return;

  if (Firebase.RTDB.getJSON(&fbdo, "/iluminacion")) {
    FirebaseJson &json = fbdo.jsonObject();
    FirebaseJsonData d;

    json.get(d, "lux");
    if (d.success) lux = d.floatValue;
    json.get(d, "umbral_encendido");
    if (d.success) umbralOn = d.intValue;
    json.get(d, "umbral_apagado");
    if (d.success) umbralOff = d.intValue;

    // Histéresis: entre los dos umbrales se mantiene el estado anterior
    bool nuevo = lampara;
    if (lux < umbralOn) nuevo = true;
    else if (lux > umbralOff) nuevo = false;

    if (nuevo != lampara || primeraLectura) {
      lampara = nuevo;
      primeraLectura = false;
      digitalWrite(PIN_LED, lampara ? HIGH : LOW);
      Firebase.RTDB.setBool(&fbdo, "/iluminacion/lampara", lampara);
    }

    mostrar("Luminosidad: " + String((int)lux) + " lux",
            String("Lampara: ") + (lampara ? "ENCENDIDA" : "APAGADA"));
    Serial.printf("lux=%.0f lampara=%d\n", lux, lampara);
  } else {
    Serial.println(fbdo.errorReason());
  }
}