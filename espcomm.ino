#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#ifndef STASSID
#define STASSID "ERICESP"
#define STAPSK "!Qaz@Wsx"
#endif

#define MSG_LEN 1024
#define HTML_LEN 1024
#define SSID_LEN 40
#define PSK_LEN 20
const char *ssid = STASSID;
const char *password = STAPSK;
const char *id_of_ssid = "ssid";
const char *id_of_psk = "psk";

char spiffs_info[MSG_LEN];
char ap_ssid[SSID_LEN];
char ap_psk[PSK_LEN];

uint8_t apMode = HIGH;
size_t tmpLen;

ESP8266WebServer server(80);

#ifndef CONFIG_PIN
#define CONFIG_PIN 2
#endif

const String postForms = "<html>\
  <head>\
    <title>ERIC ESP AP Configuration</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>SETUP STA Information</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">\
      SSID:<input type=\"text\" name=\"ssid\" value=\"\"/><br>\
      PSK:<input type=\"password\" name=\"psk\" value=\"\"/>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>";

void handleRoot()
{
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send(200, "text/html", postForms);
}

void handleForm()
{
    if (server.method() != HTTP_POST)
    {
        server.send(405, "text/plain", "Method Not Allowed");
    }
    else
    {
        String message = "POST form was:\n";
        for (uint8_t i = 0; i < server.args(); i++)
        {
            message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
            if (!strcmp(id_of_ssid, server.argName(i)))
            {
                tmpLen = strlen(server.arg(i));
                if (tmpLen > SSID_LEN)
                    tmpLen = SSID_LEN;
                memset(ap_ssid, 0x0, SSID_LEN);
                memcpy(ap_ssid, server.arg(i), tmpLen);
            }
            else if (!strcmp(id_of_psk, server.argName(i)))
            {
                tmpLen = strlen(server.arg(i));
                if (tmpLen > PSK_LEN)
                    tmpLen = PSK_LEN;
                memset(ap_psk, 0x0, PSK_LEN);
                memcpy(ap_psk, server.arg(i), tmpLen);
            }
        }
        saveConfig();
        message += spiffs_info;
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        server.send(200, "text/plain", message);
    }
}

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send(404, "text/plain", message);
}

void startAP()
{
    WiFi.softAP(STASSID, STAPSK);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.on("/", handleRoot);
    server.on("/postform/", handleForm);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}

void loadConfig()
{
    if (SPIFFS.begin())
    {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json"))
        {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile)
            {
                Serial.println("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject &json = jsonBuffer.parseObject(buf.get());
                json.printTo(Serial);
                if (json.success())
                {
                    Serial.println("\nparsed json");

                    strcpy(ap_ssid, json["ap_ssid"]);
                    strcpy(ap_psk, json["ap_psk"]);
                }
                else
                {
                    Serial.println("failed to load json config");
                }
            }
        }
    }
    else
    {
        Serial.println("failed to mount FS");
    }
}

void saveConfig()
{
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["ap_ssid"] = ssid;
    json["ap_psk"] = ap_psk;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
}

void setup(void)
{
    Serial.begin(115200);
    FSInfo fs_info;
    SPIFFS.begin();
    SPIFFS.info(fs_info);
    memset(spiffs_info, 0x0, MSG_LEN);
    sprintf(spiffs_info, "Total bytes : %d\nused bytes : %d\nblock size : %d\npage size : %d\nmax open files : %d\nmax path length:%d\n",
            fs_info.totalBytes,
            fs_info.usedBytes,
            fs_info.blockSize,
            fs_info.pageSize,
            fs_info.maxOpenFiles,
            fs_info.maxPathLength);
    Serial.println(spiffs_info);
    SPIFFS.end();
    delay(3000);
    // pinMode(CONFIG_PIN, INPUT);
    // apMode = digitalRead(CONFIG_PIN);
    // Serial.printf("apMode %d\n",apMode);
    // if (apMode == LOW)
    // {
    startAP();
    // }
    // else
    // {
    //     Serial.println("start STA Mode");
    // }
    // pinMode(CONFIG_PIN, OUTPUT);
}

void loop(void)
{
    // if (apMode == LOW)
    // {
    server.handleClient();
    // }
    // else
    // {
    //     // digitalWrite(CONFIG_PIN, HIGH);
    // }
}