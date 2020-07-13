#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
//#include <FS.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FFat.h>

#define USE_SERIAL Serial

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleNotFound();
void handleRoot();
void configureDevice();
void startConfigWebpage();

const char *wifiSsid = "xxxx";
const char *wifiPassword = "xxxx";
const char *apSsid = "ESP32 IoT";
const char *apPassword = "";

uint8_t configButton = 23;
uint8_t builtinLed = 5;  

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

unsigned long connectTimeout;

WiFiClient client;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(9090);

void setup()
{
  Serial.begin(115200);
  pinMode(configButton, INPUT);
  pinMode(builtinLed, OUTPUT);

  if (!FFat.begin(true)){
    Serial.println("Couldn't mount the filesystem.");
  }

  //FFat.remove("/deviceConfig.txt");

  configureDevice();

  // ssid = "";
  // password = "";
  // isStaticIp = false;
  // staticIp = "";
  // netmask = "";
  // gateway = "";
  //blankDevice = true;

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

      DynamicJsonDocument deviceInfo(JSON_ARRAY_SIZE(500));
      deviceInfo["deviceInfo"]["macAddress"] = WiFi.macAddress();
      deviceInfo["deviceInfo"]["ssid"] = ssid;
      deviceInfo["deviceInfo"]["password"] = password;
      deviceInfo["deviceInfo"]["isStaticIp"] = isStaticIp;
      deviceInfo["deviceInfo"]["staticIp"] = staticIp;
      deviceInfo["deviceInfo"]["netmask"] = netmask;
      deviceInfo["deviceInfo"]["gateway"] = gateway;

      len = measureJson(deviceInfo);
      char jsonToSend[len];
      serializeJson(deviceInfo, Serial);
      serializeJson(deviceInfo, jsonToSend, len + 1);
      webSocket.sendTXT(num, jsonToSend, strlen(jsonToSend));
      //webSocket.sendTXT(num, "{Connected}");
    }
    break;
    case WStype_TEXT:
    {
      USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
      
      StaticJsonDocument<512> jsonBuffer;
      deserializeJson(jsonBuffer, payload);
      JsonObject jsonObject = jsonBuffer.as<JsonObject>();

      if(strcmp((const char*)payload, "scanNetworks") == 0)
      {
        uint8_t n;
        size_t len;
        DynamicJsonDocument networksList(JSON_ARRAY_SIZE(50));
        Serial.println("scan start");
        n = WiFi.scanNetworks();
        if (n == 0)
        {
          Serial.println("no networks found");
        }
        else
        {
          Serial.print(n);
          Serial.println(" networks found");
          for (uint8_t i = 0; i < n; ++i)
          {
            // Pruint8_t SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.println(")");
            //networks[i] = WiFi.SSID(i);
            networksList["ssidArray"].add(WiFi.SSID(i));
            delay(10);
          }
        }
        len = measureJson(networksList);
        char networksArray[len];
        serializeJson(networksList, Serial);
        //Serial.println("");
        //Serial.println(len);
        serializeJson(networksList, networksArray, len + 1);
        webSocket.sendTXT(num, networksArray, strlen(networksArray));
      }

      if(strcmp((const char*)payload, "ping") == 0)
      {
        webSocket.sendTXT(num, "{\"pong\":true}");
      }

      if(strcmp((const char*)payload, "deviceConfiguration") == 0)
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
        char buff[len];
        serializeJson(jsonObject, buff, len + 1);
        File file = FFat.open("/deviceConfig.txt", "w");
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
        FFat.remove("/deviceConfig.txt");
        blankDevice = true;
        webSocket.sendTXT(num, "{\"eraseOk\":true}");
      }

      if(strcmp((const char*)payload, "startDevice") == 0)
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
  SPIFFS.begin();
  File file = SPIFFS.open("/index.html.gz", "r");
  server.streamFile(file, "text/html");
  file.close();
  SPIFFS.end();
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not found");
}

void configureDevice()
{
  Serial.println("Configuring device...");
  File file = FFat.open("/deviceConfig.txt", "r");
  int len = file.size();
  char buff[len]; 
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
  while(file.available()){
    file.readBytes(buff, len);
  }
  file.close();
  StaticJsonDocument<512> jsonBuffer;
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
  }  
}
