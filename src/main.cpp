#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

#ifdef ESP8266
  #include <LittleFS.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266WiFi.h>
  #define CONFIG_FS LittleFS
  #define WEB_FS LittleFS
#else
  #include <FFat.h>
  #include <SPIFFS.h>
  #include <WebServer.h>
  #include <WiFi.h>
  #include <WiFiClient.h>
  #define CONFIG_FS FFat
  #define WEB_FS SPIFFS
#endif

#define USE_SERIAL Serial

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleNotFound();
void handleRoot();
void configureDevice();
void startConfigWebpage();
void processNetworkScan();
void mountFilesystems();

const char *wifiSsid = "xxxx";
const char *wifiPassword = "xxxx";
#ifdef ESP8266
const char *apSsid = "ESP8266 WiFiManagerVue";
#else
const char *apSsid = "ESP32 WiFiManagerVue";
#endif
const char *apPassword = "";

#ifdef ESP8266
const uint8_t configButton = D5;
const uint8_t builtinLed = LED_BUILTIN;
#else
const uint8_t configButton = 35;
const uint8_t builtinLed = 4;
#endif

String ssid;
String password;
bool isStaticIp;
String staticIp;
String netmask;
String gateway;
bool blankDevice;
bool keepConfigWegpage;
//uint8_t restartConfigWebpage = 0;
uint8_t wifiStatus;
bool networkScanInProgress = false;
uint8_t networkScanClient = 0;
bool configFilesystemReady = false;
bool webFilesystemReady = false;

unsigned long connectTimeout;

WiFiClient client;
#ifdef ESP8266
ESP8266WebServer server(80);
#else
WebServer server(80);
#endif
WebSocketsServer webSocket = WebSocketsServer(9090);

void setup()
{
  Serial.begin(115200);
#ifdef ESP8266
  pinMode(configButton, INPUT_PULLUP);
#else
  pinMode(configButton, INPUT);
#endif
  pinMode(builtinLed, OUTPUT);

  mountFilesystems();

  //CONFIG_FS.remove("/deviceConfig.txt");

  configureDevice();

  // ssid = "";
  // password = "";
  // isStaticIp = false;
  // staticIp = "";
  // netmask = "";
  // gateway = "";
  // blankDevice = true;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  if(digitalRead(configButton) == LOW || blankDevice == true)
  {
    startConfigWebpage();
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  if(isStaticIp == true) {
    IPAddress _ip;
    IPAddress _gateway;
    IPAddress _netmask;
    _ip.fromString(staticIp);
    _gateway.fromString(gateway);
    _netmask.fromString(netmask);
    WiFi.config(_ip, _gateway, _netmask);
  }

  if(password == "") {
    WiFi.begin(ssid.c_str());
  } else {
    WiFi.begin(ssid.c_str(), password.c_str());
  }

  connectTimeout = millis() + (15 * 1000);
  while(true) {
    wifiStatus = WiFi.status();

    if ((wifiStatus == WL_CONNECTED) || (wifiStatus == WL_NO_SSID_AVAIL) ||
    (wifiStatus == WL_CONNECT_FAILED) || (millis() >= connectTimeout)) 
      break;

    delay(100);
  }
  if(wifiStatus != WL_CONNECTED)
  {
    startConfigWebpage();
  }

  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   restartConfigWebpage++;
  //   delay(1000);
  //   if(restartConfigWebpage >= 10){
  //     startConfigWebpage();
  //   }
  // }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(100);

  // server.on("/", handleRoot);
  // server.onNotFound(handleNotFound);
  // server.begin();
}

void loop()
{
  digitalWrite(builtinLed, HIGH);
  delay(500);
  digitalWrite(builtinLed, LOW);
  delay(500);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      size_t len;

      JsonDocument deviceInfo;
      deviceInfo["deviceInfo"]["macAddress"] = WiFi.macAddress();
      deviceInfo["deviceInfo"]["ssid"] = ssid;
      deviceInfo["deviceInfo"]["password"] = password;
      deviceInfo["deviceInfo"]["isStaticIp"] = isStaticIp;
      deviceInfo["deviceInfo"]["staticIp"] = staticIp;
      deviceInfo["deviceInfo"]["netmask"] = netmask;
      deviceInfo["deviceInfo"]["gateway"] = gateway;

      len = measureJson(deviceInfo);
      char jsonToSend[len + 1];
      serializeJson(deviceInfo, Serial);
      serializeJson(deviceInfo, jsonToSend, len + 1);
      webSocket.sendTXT(num, jsonToSend, strlen(jsonToSend));
      //webSocket.sendTXT(num, "{Connected}");
    }
    break;
    case WStype_TEXT:
    {
      USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
      
      JsonDocument jsonBuffer;
      deserializeJson(jsonBuffer, payload);
      JsonObject jsonObject = jsonBuffer.as<JsonObject>();

      if(strcmp((const char*)payload, "scanNetworks") == 0)
      {
        if(!networkScanInProgress) {
          Serial.println("Starting asynchronous network scan");
          WiFi.scanDelete();
          networkScanClient = num;
          networkScanInProgress = true;
          WiFi.scanNetworks(true);
        }
      }

      if(strcmp((const char*)payload, "ping") == 0)
      {
        webSocket.sendTXT(num, "{\"pong\":true}");
      }

      if(jsonObject["deviceConfiguration"].is<JsonObject>())
      {
        //webSocket.sendTXT(num, "{\"pong\":true}");
        JsonVariant _ssid = jsonObject["deviceConfiguration"]["ssid"];
        JsonVariant _password = jsonObject["deviceConfiguration"]["password"];
        JsonVariant _isStaticIp = jsonObject["deviceConfiguration"]["isStaticIp"];
        JsonVariant _staticIp = jsonObject["deviceConfiguration"]["staticIp"];
        JsonVariant _netmask = jsonObject["deviceConfiguration"]["netmask"];
        JsonVariant _gateway = jsonObject["deviceConfiguration"]["gateway"];
        ssid = _ssid.as<String>();
        password = _password.as<String>();
        isStaticIp = _isStaticIp.as<bool>();
        staticIp = _staticIp.as<String>();
        netmask = _netmask.as<String>();
        gateway = _gateway.as<String>();

        int len = measureJson(jsonObject);
        char buff[len + 1];
        serializeJson(jsonObject, buff, len + 1);
        if(!configFilesystemReady) {
          Serial.println("Configuration filesystem is unavailable");
          webSocket.sendTXT(num, "{\"saveOk\":false}");
          return;
        }
        File file = CONFIG_FS.open("/deviceConfig.txt", "w");
        if(!file) {
          Serial.println("Wifi file write error");
          return;
        }
        if(file.print(buff)){
          Serial.println("Wifi file was written");
          Serial.println(buff);
        } else {
          Serial.println("File write failed");
        }
        file.close();
        blankDevice = false;
        webSocket.sendTXT(num, "{\"saveOk\":true}");
      }

      if(strcmp((const char*)payload, "eraseConfig") == 0)
      {
        ssid = "";
        password = "";
        isStaticIp = false;
        staticIp = "";
        netmask = "";
        gateway = "";
        if(configFilesystemReady) {
          CONFIG_FS.remove("/deviceConfig.txt");
        }
        blankDevice = true;
        webSocket.sendTXT(num, "{\"eraseOk\":true}");
      }

      if(jsonObject["startDevice"].is<JsonObject>())
      {
        //configureDevice();
        webSocket.sendTXT(num, "{\"startDeviceOk\":true}");
        //blankDevice = false;
        keepConfigWegpage = false;
      }

      if(strcmp((const char*)payload, "testWifi") == 0)
      {
        if(blankDevice == true) {
          Serial.print("Cant test blank device\n");
          webSocket.sendTXT(num, "{\"testWifiStatus\":\"blankdevice\"}");
          return;
        }
        Serial.print("Testing connection to: ");
        //WiFi.mode(WIFI_AP_STA);
        Serial.println(ssid);
        Serial.print("Password ");
        Serial.println(password);
        WiFi.disconnect(true);
        WiFi.begin(ssid.c_str(), password.c_str());
        connectTimeout = millis() + (20 * 1000);
        while(true) {
          wifiStatus = WiFi.status();
          Serial.printf("(%d).", wifiStatus);
          if ((wifiStatus == WL_CONNECTED) || (wifiStatus == WL_NO_SSID_AVAIL) ||
          (wifiStatus == WL_CONNECT_FAILED) || (millis() >= connectTimeout)) 
            break;
          delay(100);
        }
        switch (wifiStatus)
        {
          case WL_NO_SSID_AVAIL:
            Serial.print("WL_NO_SSID_AVAIL\n");
            webSocket.sendTXT(num, "{\"testWifiStatus\":\"WL_NO_SSID_AVAIL\"}");
            break;
          case WL_CONNECT_FAILED:
            Serial.print("WL_CONNECT_FAILED\n");
            webSocket.sendTXT(num, "{\"testWifiStatus\":\"WL_CONNECT_FAILED\"}");
            break;
          case WL_CONNECTED:
            Serial.print("WL_CONNECTED\n");
            webSocket.sendTXT(num, "{\"testWifiStatus\":\"WL_CONNECTED\"}");
            break;        
          default:
            Serial.print("default\n");
            webSocket.sendTXT(num, "{\"testWifiStatus\":\"error\"}");
            break;
        }
        WiFi.disconnect();
      }

    }
    break;
    case WStype_BIN: 
    case WStype_PING:
    case WStype_PONG:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void handleRoot()
{
  Serial.println("HandleRoot");
  if(!webFilesystemReady) {
    server.send(500, "text/plain", "Web filesystem is unavailable");
    return;
  }
  File file = WEB_FS.open("/index.html.gz", "r");
  if(!file) {
    server.send(404, "text/plain", "Web application not found");
    return;
  }
  server.sendHeader("Cache-Control", "no-cache");
  server.streamFile(file, "text/html");
  file.close();
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not found");
}

void configureDevice()
{
  Serial.println("Configuring device...");
  if(!configFilesystemReady || !CONFIG_FS.exists("/deviceConfig.txt")) {
      Serial.println("No saved configuration, blankDevice true");
      ssid = "";
      password = "";
      isStaticIp = false;
      staticIp = "";
      netmask = "";
      gateway = "";
      blankDevice = true;
      return;
  }
  File file = CONFIG_FS.open("/deviceConfig.txt", "r");
  if(!file){
      Serial.println("Failed to open config file, blankDevice true");
      ssid = "";
      password = "";
      isStaticIp = false;
      staticIp = "";
      netmask = "";
      gateway = "";
      blankDevice = true;
      return;
  }
  int len = file.size();
  char buff[len + 1];
  while(file.available()){
    file.readBytes(buff, len);
  }
  buff[len] = '\0';
  file.close();
  JsonDocument jsonBuffer;
  deserializeJson(jsonBuffer, buff);
  JsonObject jsonObject = jsonBuffer.as<JsonObject>();
  JsonVariant _ssid = jsonObject["deviceConfiguration"]["ssid"];
  JsonVariant _password = jsonObject["deviceConfiguration"]["password"];
  JsonVariant _isStaticIp = jsonObject["deviceConfiguration"]["isStaticIp"];
  JsonVariant _staticIp = jsonObject["deviceConfiguration"]["staticIp"];
  JsonVariant _netmask = jsonObject["deviceConfiguration"]["netmask"];
  JsonVariant _gateway = jsonObject["deviceConfiguration"]["gateway"];
  ssid = _ssid.as<String>();
  password = _password.as<String>();
  isStaticIp = _isStaticIp.as<bool>();
  staticIp = _staticIp.as<String>();
  netmask = _netmask.as<String>();
  gateway = _gateway.as<String>();

  //if(ssid == "" || (isStaticIp == true && (staticIp == "" || netmask == "" || gateway == "")))

  blankDevice = false;

  Serial.println("------ WIFI ------");
  Serial.println(ssid);
  Serial.println(password);
  Serial.println(isStaticIp);
  Serial.println(staticIp);
  Serial.println(netmask);
  Serial.println(gateway);
  Serial.println("------------------");

  Serial.println("Configure OK");
}

void mountFilesystems()
{
#ifdef ESP8266
  configFilesystemReady = LittleFS.begin();
  webFilesystemReady = configFilesystemReady;
  if(!configFilesystemReady) {
    Serial.println("Couldn't mount LittleFS");
  }
#else
  configFilesystemReady = FFat.begin(true);
  webFilesystemReady = SPIFFS.begin(false);
  if(!configFilesystemReady) {
    Serial.println("Couldn't mount FFat");
  }
  if(!webFilesystemReady) {
    Serial.println("Couldn't mount SPIFFS");
  }
#endif
}

void startConfigWebpage()
{
  Serial.println("Starting AP for configuration");
  keepConfigWegpage = true;
  WiFi.mode(WIFI_AP_STA);
  //WiFi.disconnect();
  WiFi.softAP(apSsid, apPassword);

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("Starting Websocket loop");
  while(keepConfigWegpage)
  {
    webSocket.loop();
    server.handleClient();
    processNetworkScan();
  }  
}

void processNetworkScan()
{
  if(!networkScanInProgress) return;

  int16_t result = WiFi.scanComplete();
  if(result == WIFI_SCAN_RUNNING) return;

  JsonDocument networksList;
  JsonArray ssidArray = networksList["ssidArray"].to<JsonArray>();

  if(result == WIFI_SCAN_FAILED) {
    Serial.println("Network scan failed");
  } else {
    Serial.printf("%d networks found\n", result);
    for(int16_t i = 0; i < result; ++i) {
      Serial.printf("%d: %s (%d)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
      ssidArray.add(WiFi.SSID(i));
    }
  }

  String response;
  serializeJson(networksList, response);
  webSocket.sendTXT(networkScanClient, response);

  WiFi.scanDelete();
  networkScanInProgress = false;
}
