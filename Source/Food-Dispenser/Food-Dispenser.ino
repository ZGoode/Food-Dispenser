#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Servo.h>
#include <Wire.h>
#include <AFArray.h>
#include <AFArrayType.h>
#include "FS.h"
#include "SH1106Wire.h"
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"

#include "HTML.h"

#define VERSION "1.0"
#define HOSTNAME "Food-Dispensor"
#define CONFIG "/conf.txt"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

void handleSystemReset();
void handleWifiReset();
int8_t getWifiQuality();
void writeSettings();
void handleUpdateConfigure();
void handleUpdateControl();
void readSettings();
void handleNotFound();
void handleRoot();
void handleConfigure();
void handleConfigureNoPassword();
void handleControl();
void handleControlNoPassword();
String reportFeedingTimes();
void feedPosie(double cupsOfFood);
int getTimeZone(int modes);

const int WEBSERVER_PORT = 80;
const boolean WEBSERVER_ENABLED = true;
char* www_username = "admin";
char* www_password = "password";
int timeZone = -7;
int screenMode = 0;
boolean displayIP = false;

int lengthOfFeedingArray = 0;
AFArray<int> feedingHours;
AFArray<int> feedingMinutes;
AFArray<boolean> AMPM;
AFArray<boolean> monday;
AFArray<boolean> tuesday;
AFArray<boolean> wednesday;
AFArray<boolean> thursday;
AFArray<boolean> friday;
AFArray<boolean> saturday;
AFArray<boolean> sunday;
AFArray<double> cups;

long previousMillis = 0;
long previousMillisIP = 0;
long interval = 5000;
long IPInterval = 10000;
long previousMillisFeedingTime = 0;
long FeedingTimeInterval = 60000;
long previousMillisServo = 0;
long servoInterval = 0;

boolean augerOn = false;

long millisPerCup = 250; //fill this variable

boolean ENABLE_OTA = true;
String OTA_Password = "";

ESP8266WebServer server(WEBSERVER_PORT);

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "ntp1.net.berkeley.edu");

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c; // I2C Address of your Display (usually 0x3c or 0x3d)
const int SDA_PIN = D2;
const int SCL_PIN = D5;
const boolean INVERT_DISPLAY = true; // true = pins at top | false = pins at the bottom
//#define DISPLAY_SH1106       // Uncomment this line to use the SH1106 display -- SSD1306 is used by default and is most common

// Initialize the oled display for I2C_DISPLAY_ADDRESS
// SDA_PIN and SCL_PIN
#if defined(DISPLAY_SH1106)
SH1106Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SCL_PIN);
#else
SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SCL_PIN); // this is the default
#endif

OLEDDisplayUi   ui( &display );

// Set the number of Frames supported
const int numberOfFrames = 3;
FrameCallback frames[numberOfFrames];
FrameCallback clockFrame[2];

Servo myservo;

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

  //initialize display
  display.init();
  if (INVERT_DISPLAY) {
    display.flipScreenVertically(); // connections at top of OLED display
  }

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 10, "Connect to WiFi Network");
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 24, HOSTNAME);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 38, "And log into your home");
  display.drawString(64, 48, "WiFi network to setup");
  display.display();

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
    server.on("/Control", handleControl);
    server.on("/updateControl", handleUpdateControl);
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
  timeClient.update();
  server.handleClient();
  ArduinoOTA.handle();

  unsigned long currentMillis = millis();

  if (digitalRead(IPAddressButton) == HIGH) { //IP Address Mode
    String tempString = "";

    previousMillisIP = currentMillis;

    displayIP = true;

    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 10, "Web Interface On");
    display.drawString(64, 20, "You May Connect to IP");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 30, WiFi.localIP().toString());
    display.drawString(64, 46, "Port: " + String(WEBSERVER_PORT));
    display.display();
  }


  if (currentMillis - previousMillisIP > IPInterval) {
    displayIP = false;
    display.clear();
    display.display();
  }

  if (currentMillis - previousMillisFeedingTime > FeedingTimeInterval) {
    for (int i; i < lengthOfFeedingArray; i++) {
      int temp = feedingHours[i];

      if (!AMPM[i]) {
        temp += 12;
      }

      if (temp == getTimeZone(0)) {
        if (feedingMinutes[i] == timeClient.getMinutes()) {
          if ((getTimeZone(1) == 0) && sunday[i]) {
            myservo.attach(D4);
            myservo.write(180);
            previousMillisServo = currentMillis;
            servoInterval = millisPerCup * cups[i];
            augerOn = true;
          } else if ((getTimeZone(1) == 1) && monday[i]) {
            myservo.attach(D4);
            myservo.write(180);
            previousMillisServo = currentMillis;
            servoInterval = millisPerCup * cups[i];
            augerOn = true;
          } else if ((getTimeZone(1) == 2) && tuesday[i]) {
            myservo.attach(D4);
            myservo.write(180);
            previousMillisServo = currentMillis;
            servoInterval = millisPerCup * cups[i];
            augerOn = true;
          } else if ((getTimeZone(1) == 3) && wednesday[i]) {
            myservo.attach(D4);
            myservo.write(180);
            previousMillisServo = currentMillis;
            servoInterval = millisPerCup * cups[i];
            augerOn = true;
          } else if ((getTimeZone(1) == 4) && thursday[i]) {
            myservo.attach(D4);
            myservo.write(180);
            previousMillisServo = currentMillis;
            servoInterval = millisPerCup * cups[i];
            augerOn = true;
          } else if ((getTimeZone(1) == 5) && friday[i]) {
            myservo.attach(D4);
            myservo.write(180);
            previousMillisServo = currentMillis;
            servoInterval = millisPerCup * cups[i];
            augerOn = true;
          } else if ((getTimeZone(1) == 6) && saturday[i]) {
            myservo.attach(D4);
            myservo.write(180);
            previousMillisServo = currentMillis;
            servoInterval = millisPerCup * cups[i];
            augerOn = true;
          }
        }
      }
    }
  }

  if ((currentMillis - previousMillisServo > servoInterval) && augerOn) {
    myservo.detach();
    augerOn = false;
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
    //add support for afarray
    //add support for length of afarray
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

  writeSettings();
  handleConfigureNoPassword();
}

void handleUpdateControl() {  //fix this for the control page
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }

  int placeholder = server.arg("timeselection").toInt();

  if (server.arg("hours").toInt() == 0) {
    lengthOfFeedingArray--;
    if (placeholder == 0) {
      feedingHours = feedingHours.slice(placeholder + 1, lengthOfFeedingArray);
      feedingMinutes = feedingMinutes.slice(placeholder + 1, lengthOfFeedingArray);
      AMPM = AMPM.slice(placeholder + 1, lengthOfFeedingArray);
      monday = monday.slice(placeholder + 1, lengthOfFeedingArray);
      tuesday = tuesday.slice(placeholder + 1, lengthOfFeedingArray);
      wednesday = wednesday.slice(placeholder + 1, lengthOfFeedingArray);
      thursday = thursday.slice(placeholder + 1, lengthOfFeedingArray);
      friday = friday.slice(placeholder + 1, lengthOfFeedingArray);
      saturday = saturday.slice(placeholder + 1, lengthOfFeedingArray);
      sunday = sunday.slice(placeholder + 1, lengthOfFeedingArray);
      cups = cups.slice(placeholder + 1, lengthOfFeedingArray);
    } else if (placeholder == lengthOfFeedingArray) {
      feedingHours = feedingHours.slice(0, placeholder - 1);
      feedingMinutes = feedingMinutes.slice(0, placeholder - 1);
      AMPM = AMPM.slice(0, placeholder - 1);
      monday = monday.slice(0, placeholder - 1);
      tuesday = tuesday.slice(0, placeholder - 1);
      wednesday = wednesday.slice(0, placeholder - 1);
      thursday = thursday.slice(0, placeholder - 1);
      friday = friday.slice(0, placeholder - 1);
      saturday = saturday.slice(0, placeholder - 1);
      sunday = sunday.slice(0, placeholder - 1);
      cups = cups.slice(0, placeholder - 1);
    } else {
      feedingHours = feedingHours.slice(0, placeholder - 1) + feedingHours.slice(placeholder + 1, lengthOfFeedingArray);
      feedingMinutes = feedingMinutes.slice(0, placeholder - 1) + feedingMinutes.slice(placeholder + 1, lengthOfFeedingArray);
      AMPM = AMPM.slice(0, placeholder - 1) + AMPM.slice(placeholder + 1, lengthOfFeedingArray);
      monday = monday.slice(0, placeholder - 1) + monday.slice(placeholder + 1, lengthOfFeedingArray);
      tuesday = tuesday.slice(0, placeholder - 1) + tuesday.slice(placeholder + 1, lengthOfFeedingArray);
      wednesday = wednesday.slice(0, placeholder - 1) + wednesday.slice(placeholder + 1, lengthOfFeedingArray);
      thursday = thursday.slice(0, placeholder - 1) + thursday.slice(placeholder + 1, lengthOfFeedingArray);
      friday = friday.slice(0, placeholder - 1) + friday.slice(placeholder + 1, lengthOfFeedingArray);
      saturday = saturday.slice(0, placeholder - 1) + saturday.slice(placeholder + 1, lengthOfFeedingArray);
      sunday = sunday.slice(0, placeholder - 1) + sunday.slice(placeholder + 1, lengthOfFeedingArray);
      cups = cups.slice(0, placeholder - 1) + cups.slice(placeholder + 1, lengthOfFeedingArray);
    }
  }

  if ((placeholder == lengthOfFeedingArray) || lengthOfFeedingArray <= 0) {
    lengthOfFeedingArray++;

    feedingHours.add(server.arg("hours").toInt());
    feedingMinutes.add(server.arg("minutes").toInt());
    AMPM.add(server.hasArg("ampm"));
    monday.add(server.hasArg("monday"));
    tuesday.add(server.hasArg("tuesday"));
    wednesday.add(server.hasArg("wednesday"));
    thursday.add(server.hasArg("thursday"));
    friday.add(server.hasArg("friday"));
    saturday.add(server.hasArg("saturday"));
    sunday.add(server.hasArg("sunday"));
    double temp = server.arg("cupsoffood").toDouble();
    cups.add(temp);
  } else {
    feedingHours[placeholder] = server.arg("hours").toInt();
    feedingMinutes[placeholder] = server.arg("minutes").toInt();

    if ((server.arg("ampm").toInt()) == 1) {
      AMPM[placeholder] = true;
    } else {
      AMPM[placeholder] = false;
    }

    monday[placeholder] = server.hasArg("monday");
    tuesday[placeholder] = server.hasArg("tuesday");
    wednesday[placeholder] = server.hasArg("wednesday");
    thursday[placeholder] = server.hasArg("thursday");
    friday[placeholder] = server.hasArg("friday");
    saturday[placeholder] = server.hasArg("saturday");
    sunday[placeholder] = server.hasArg("sunday");
    String temp = server.arg("cupsoffood");
    cups[placeholder] = temp.toDouble();
  }

  writeSettings();
  handleControlNoPassword();
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
    if (line.indexOf("otapassword=") >= 0) {
      OTA_Password = line.substring(line.lastIndexOf("otapassword=") + 12).toInt();
      Serial.println("otapassword=" + String(OTA_Password));
    }
    //add suport for afarray
    //add variable for length of afarray
  }
  fr.close();
}

void handleNotFound() {
  server.send(404, "text/plain", "404: <span style='color: red'><strong>N</strong></span>   ot found"); // Send HTTP status 404 (Not Found) when there's no handler for the URL in the request
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

  server.send(200, "text/html", form);
}

void handleConfigureNoPassword() {
  String form = parseConfigurePage();
  form.replace("%USERID%", www_username);
  form.replace("%STATIONPASSWORD%", www_password);
  form.replace("%OTAPASSWORD%", OTA_Password);
  form.replace("%TIMEZONE%", String(timeZone));

  server.send(200, "text/html", form);
}

void handleControl() {
  if (!server.authenticate(www_username, www_password)) {
    return server.requestAuthentication();
  }

  String form = parseControlPage();
  form.replace("%FEEDINGTIMEMENU%", parseFeedingTimes(lengthOfFeedingArray));
  form.replace("%FEEDINGTIMES%", reportFeedingTimes());

  server.send(200, "text/html", form);
}

void handleControlNoPassword() {
  String form = parseControlPage();
  form.replace("%FEEDINGTIMEMENU%", parseFeedingTimes(lengthOfFeedingArray));
  form.replace("%FEEDINGTIMES%", reportFeedingTimes());

  server.send(200, "text/html", form);
}

String reportFeedingTimes() {
  String temp = "<div class='w3-container'><h2>Feeding Times:";
  temp.concat(getTimeZone(0));
  temp.concat(":");
  temp.concat(timeClient.getMinutes());
  temp.concat(" ");
  temp.concat(getTimeZone(1));
  temp.concat("</h2>");

  for (int i = 0; i < lengthOfFeedingArray; i++) {
    String temp2 = "<p class='w3-container'>";
    temp2.concat("<div class='w3-container'><label>Feeding Time: ");
    temp2.concat((i + 1));
    temp2.concat("</label></div>");

    temp2.concat("<div class='w3-container'>Time: ");
    temp2.concat(feedingHours[i]);
    temp2.concat(":");
    if (feedingMinutes[i] < 10) {
      temp2.concat("0");
    }
    temp2.concat(feedingMinutes[i]);
    temp2.concat(" ");

    if (AMPM[i]) {
      temp2.concat("AM");
    } else {
      temp2.concat("PM");
    }

    temp2.concat("</div>");

    temp2.concat("<div class='w3-container'>");
    if (monday[i]) {
      temp2.concat("Mon: <span style='color: green'><strong>Y</strong></span>   ");
    } else {
      temp2.concat("Mon: <span style='color: red'><strong>N</strong></span>   ");
    }

    if (tuesday[i]) {
      temp2.concat("Tue: <span style='color: green'><strong>Y</strong></span>   ");
    } else {
      temp2.concat("Tue: <span style='color: red'><strong>N</strong></span>   ");
    }

    if (wednesday[i]) {
      temp2.concat("Wed: <span style='color: green'><strong>Y</strong></span>   ");
    } else {
      temp2.concat("Wed: <span style='color: red'><strong>N</strong></span>   ");
    }

    if (thursday[i]) {
      temp2.concat("Thur: <span style='color: green'><strong>Y</strong></span>   ");
    } else {
      temp2.concat("Thur: <span style='color: red'><strong>N</strong></span>   ");
    }

    if (friday[i]) {
      temp2.concat("Fri: <span style='color: green'><strong>Y</strong></span>   ");
    } else {
      temp2.concat("Fri: <span style='color: red'><strong>N</strong></span>   ");
    }

    if (saturday[i]) {
      temp2.concat("Sat: <span style='color: green'><strong>Y</strong></span>   ");
    } else {
      temp2.concat("Sat: <span style='color: red'><strong>N</strong></span>   ");
    }

    if (sunday[i]) {
      temp2.concat("Sun: <span style='color: green'><strong>Y</strong></span>   ");
    } else {
      temp2.concat("Sun: <span style='color: red'><strong>N</strong></span>   ");
    }

    temp2.concat("</div>");
    temp2.concat("<div class='w3-container'>");
    temp2.concat("Cups: ");
    temp2.concat(cups[i]);
    temp2.concat("</div>");
    temp2.concat("</p>");

    temp.concat(temp2);
  }

  temp.concat("</div>");
  return temp;
}

int getTimeZone(int modes) {
  int tempHours = timeClient.getHours();

  tempHours += timeZone;

  if (tempHours < 0) {
    tempHours += 24;
  } else if (tempHours > 24) {
    tempHours -= 24;
  }

  if (modes == 0) {
    return tempHours;
  }

  if (modes == 1) {
    int tempDays = timeClient.getDay();
    if ((timeClient.getHours() + timeZone) > 24) {
      tempDays++;
      return tempDays;

      if (tempDays > 6) {
        return 0;
      }
    } else if ((timeClient.getHours() + timeZone) < 0) {
      tempDays--;
      return tempDays;

      if (tempDays < 0) {
        return 6;
      }
    }
    } else {
      return timeClient.getDay();
    }
  }
}

