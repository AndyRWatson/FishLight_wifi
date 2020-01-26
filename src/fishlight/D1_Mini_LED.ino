#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>
#include <NTPClient.h>


#define LED_PIN     D5
#define NUM_LEDS    7

CRGB leds[NUM_LEDS];

const IPAddress apIP(192, 168, 1, 1);
const char* apSSID = "FishTankLight";
boolean settingMode;
boolean ledOn=false;
boolean timeSet=false;
String ssidList;
const long utcOffsetInSeconds = 3600;  // One Hour
const long  gmtOffset_sec = 3600;      // One Hour
static int hour;
static int minute;
static int second;
int time_period = 60000;  // 10 minutes
unsigned long time_now=0;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
//NTPClient tc(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
NTPClient tc(ntpUDP, "pool.ntp.org", 0);

DNSServer dnsServer;
ESP8266WebServer webServer(80);

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);  
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  pinMode(LED_BUILTIN, OUTPUT);

  allOff();
  time_now = millis();
  if (restoreConfig()) {
    if (checkConnection()) {
      settingMode = false;
      startWebServer();
      return;
    }
  }
  settingMode = true;
  setupMode();
}

void loop() {
  
  // Serial.print("LED_BUILTIN=");
  // Serial.println(ledOn);
  
  if (settingMode) {
    // Config
    if(ledOn==false)
    {
      Serial.println("LED_BUILTIN,LOW");
      digitalWrite(LED_BUILTIN, LOW);
      ledOn=true;
    }
    dnsServer.processNextRequest(); 
  }
  webServer.handleClient();
  if (millis() >= (time_now + time_period))
  {
    setLight();
  }
}

void getTime()
{
  tc.update();
  hour=tc.getHours();
  minute=tc.getMinutes();
  second=tc.getSeconds();
  Serial.print(daysOfTheWeek[tc.getDay()]);
  Serial.print(", ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);
  Serial.println(tc.getFormattedTime());
  timeSet=true;
  time_now=millis();
  Serial.print("millis() =");
  Serial.println(time_now);
}

void setLight()
{
  getTime();
  Serial.print("Hour =");
  Serial.println(hour);
  Serial.print("Set Light to ");
  if (hour >=23)
  {
    allOff();
  }
  else if (hour >=21)
  {
      Moonlight();
  }
  else if (hour >=20)
    {
      Sunset();
    }
  else if(hour >= 9)
    {
      Sunny();
    }    
  else if(hour >=8)
  {
    Sunrise();
  }
  else
  {
      allOff();
  }  
}

void disco()
{
  stepper(255,0,0,100);
  slider(255,0,0,50);
  slider(255,255,0,50);
  slider(255,255,255,50);
  slider(0,255,255,50);
  slider(0,0,255,50);
  slider(0,255,0,50);
}

void Moonlight()
{
  allOn(0, 0, 255);
  Serial.println("Moonlight ");
}

void Sunny()
{
   allOn(255, 255, 0);
//  Serial.println("Sunny");
 
}

void Midday()
{
   allOn(255, 255, 255);
   Serial.println("Midday");
 
}

void Overcast()
{
   allOn(201, 226, 255);
   Serial.println("Overcast");
 
}

void Sunrise()
{
   allOn(255, 255, 0);
   Serial.println("Sunrise");
 
}

void Sunset()
{
   allOn(255, 0, 0);
   Serial.println("Sunset");
 
}

void stepper(int r,int g, int b, int d)
{
   for (int i = 0; i <= NUM_LEDS; i++) {
    leds[i] = CRGB ( r, g, b);
    leds[i-1] = CRGB ( 0, 0, 0);
    FastLED.show();
     delay(d);
   }
}
void slider(int r,int g, int b, int d)
{
   for (int i = 0; i <= NUM_LEDS; i++) {
    leds[i] = CRGB ( r, g, b);
    FastLED.show();
     delay(d);
   }
}

void allOn(int r, int g, int b)
{
    for (int i = 0; i <= NUM_LEDS; i++) {
    leds[i] = CRGB ( r, g, b);
    FastLED.show();
  }
}

void allOff()
{
  allOn(0,0,0);
}



boolean restoreConfig() {
  Serial.println("Reading EEPROM...");
  String ssid = "";
  String pass = "";
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (int i = 32; i < 96; ++i) {
      pass += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(pass);
    WiFi.begin(ssid.c_str(), pass.c_str());
    return true;
  }
  else {
    Serial.println("Config not found.");
    return false;
  }
}

boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print(".");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected!");
      // Serial.println("LED_BUILTIN,HIGH");
      // digitalWrite(LED_BUILTIN, HIGH);
      ledOn=false;
      setLight();
      return (true);
    }

    count++;
  }
  Serial.println("Timed out.");
  return false;
}

void startWebServer() {
  if (settingMode) {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.softAPIP());
    webServer.on("/settings", []() {
      String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    webServer.on("/setap", []() {
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      String ssid = urlDecode(webServer.arg("ssid"));
      Serial.print("SSID: ");
      Serial.println(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      Serial.print("Password: ");
      Serial.println(pass);
      Serial.println("Writing SSID to EEPROM...");
      for (int i = 0; i < ssid.length(); ++i) {
        EEPROM.write(i, ssid[i]);
      }
      Serial.println("Writing Password to EEPROM...");
      for (int i = 0; i < pass.length(); ++i) {
        EEPROM.write(32 + i, pass[i]);
      }
      EEPROM.commit();
      Serial.println("Write EEPROM done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>FishTank Light (AP mode)</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("AP mode", s));
    });
  }
  else {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    webServer.on("/", []() {
      String s = "<h1>FishTank Light</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("STA mode", s));
    });
    webServer.on("/reset", []() {
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
    });
  }
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  dnsServer.start(53, "*", apIP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}
