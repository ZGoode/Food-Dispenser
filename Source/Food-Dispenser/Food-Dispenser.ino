#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "FS.h"

#include "HTML.h"

#define VERSION "1.0"
#define HOSTNAME "Food-Dispensor"
#define CONFIG "/conf.txt"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define INVERT_DISPLAY false

const int WEBSERVER_PORT = 80;
const boolean WEBSERVER_ENABLED = true;
char* www_username = "admin";
char* www_password = "password";
int screenMode = 0;
boolean displayIP = false;

long previousMillis = 0;
long previousMillisIP = 0;
long interval = 5000;
long IPInterval = 10000;

boolean ENABLE_OTA = true;
String OTA_Password = "";

ESP8266WebServer server(WEBSERVER_PORT);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

const int externalLight = LED_BUILTIN;

const int IPAddressButton = D7;

void setup(void) {
  Serial.begin(115200);
  SPIFFS.begin();
  delay(10);

  Serial.println();
  pinMode(externalLight, OUTPUT);

  pinMode(IPAddressButton, INPUT);

  readSettings();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  if (INVERT_DISPLAY) {
    display.invertDisplay(true); // connections at top of OLED display
  }

  display.clearDisplay();
  display.display();
  delay(100);
  display.clearDisplay();

  parseHomePage();
  parseConfigurePage();
  parseControlPage();

  WiFiManager wifiManager;

  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  if (!wifiManager.autoConnect((const char *)hostname.c_str())) {// new addition
    delay(3000);
    WiFi.disconnect(true);
    ESP.reset();
    delay(5000);
  }

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  if (OTA_Password != "") {
    ArduinoOTA.setPassword(((const char *)OTA_Password.c_str()));
  }
  ArduinoOTA.begin();

  if (WEBSERVER_ENABLED) {
    Serial.println("WEBSERVER ENABLED");
    server.on("/", HTTP_GET, handleRoot);
    server.on("/Home", HTTP_GET, handleRoot);
    server.on("/systemreset", handleSystemReset);
    server.on("/forgetwifi", handleWifiReset);
    server.on("/Configure", handleConfigure);
    server.on("/updateConfig", handleUpdateConfigure);
    server.on("/FactoryReset", handleSystemReset);
    server.on("/WifiReset", handleWifiReset);
    server.onNotFound(handleRoot);
    server.begin();
    Serial.println("Server started");
    String webAddress = "http://" + WiFi.localIP().toString() + ":" + String(WEBSERVER_PORT) + "/";
    Serial.println("Use this URL : " + webAddress);
  } else {
    Serial.println("WEBSERVER DISABLED");
  }
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  if (digitalRead(IPAddressButton) == HIGH) { //IP Address Mode
    String tempString = "";

    previousMillisIP = currentMillis;

    displayIP = true;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 4);
    display.println(F("You May Connect to IP"));
    display.setCursor(0, 13);
    tempString = WiFi.localIP().toString();
    display.println(tempString);
    display.setCursor(0, 22);
    tempString = "Port: ";
    tempString = tempString + String(WEBSERVER_PORT);
    display.println(tempString);
    display.display();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisIP > IPInterval) {
    displayIP = false;
  }

}

void handleSystemReset() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  Serial.println("Reset System Configuration");
  if (SPIFFS.remove(CONFIG)) {
    handleRoot();
    ESP.restart();
  }
}

void handleWifiReset() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  handleRoot();
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}

void writeSettings() {
  // Save decoded message to SPIFFS file for playback on power up.
  File f = SPIFFS.open(CONFIG, "w");
  if (!f) {
    Serial.println("File open failed!");
  } else {
    Serial.println("Saving settings now...");
    f.println("www_username=" + String(www_username));
    f.println("www_password=" + String(www_password));
    f.println("otapassword=" + String(OTA_Password));
    f.println("timezone=" + String(timeZone));
    f.println("24hour=" + String(hour24));
  }
  f.close();
  readSettings();
}

void handleUpdateConfigure() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }

  String temp = server.arg("userid");
  temp.toCharArray(www_username, sizeof(temp));
  temp = server.arg("stationpassword");
  temp.toCharArray(www_password, sizeof(temp));
  OTA_Password = server.arg("otapassword");
  timeZone = server.arg("timezone").toInt();
  hour24 = server.hasArg("24hour");

  writeSettings();
  handleConfigureNoPassword();
}

void readSettings() {
  if (SPIFFS.exists(CONFIG) == false) {
    Serial.println("Settings File does not yet exists.");
    writeSettings();
    return;
  }
  File fr = SPIFFS.open(CONFIG, "r");
  String line;
  while (fr.available()) {
    line = fr.readStringUntil('\n');

    if (line.indexOf("www_username=") >= 0) {
      String temp = line.substring(line.lastIndexOf("www_username=") + 13);
      temp.trim();
      temp.toCharArray(www_username, sizeof(temp));
      Serial.println("www_username=" + String(www_username));
    }
    if (line.indexOf("www_password=") >= 0) {
      String temp = line.substring(line.lastIndexOf("www_password=") + 13);
      temp.trim();
      temp.toCharArray(www_password, sizeof(temp));
      Serial.println("www_password=" + String(www_password));
    }
    if (line.indexOf("timezone=") >= 0) {
      timeZone = line.substring(line.lastIndexOf("timezone=") + 9).toInt();
      Serial.println("timezone=" + String(timeZone));
    }
    if (line.indexOf("24hour=") >= 0) {
      hour24 = line.substring(line.lastIndexOf("24hour=") + 7).toInt();
      Serial.println("IS_METRIC=" + String(hour24));
    }
    if (line.indexOf("otapassword=") >= 0) {
      hour24 = line.substring(line.lastIndexOf("otapassword=") + 12).toInt();
      Serial.println("otapassword=" + String(OTA_Password));
    }
  }
  fr.close();
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URL in the request
}

void handleRoot() {
  String form = parseHomePage();
  server.send(200, "text/html", form);
}

void handleConfigure() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }
  String form = parseConfigurePage();
  form.replace("%USERID%", www_username);
  form.replace("%STATIONPASSWORD%", www_password);
  form.replace("%OTAPASSWORD%", OTA_Password);
  form.replace("%TIMEZONE%", String(timeZone));

  String checked = "";
  if (hour24) {
    checked = "checked='checked'";
  }
  form.replace("%24HOUR%", checked);

  server.send(200, "text/html", form);
}

void handleConfigureNoPassword() {
  String form = parseConfigurePage();
  form.replace("%USERID%", www_username);
  form.replace("%STATIONPASSWORD%", www_password);
  form.replace("%OTAPASSWORD%", OTA_Password);
  form.replace("%TIMEZONE%", String(timeZone));

  String checked = "";
  if (hour24) {
    checked = "checked='checked'";
  }
  form.replace("%24HOUR%", checked);

  server.send(200, "text/html", form);
}
