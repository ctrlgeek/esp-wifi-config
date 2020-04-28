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
const char *ssid = STASSID;
const char *password = STAPSK;

char spiffs_info[MSG_LEN];

uint8_t apMode = HIGH;

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
        }
        message += spiffs_info;
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

void setup(void)
{
    Serial.begin(115200);
    FSInfo fs_info;
    SPIFFS.begin();
    SPIFFS.info(fs_info);
    memset(spiffs_info,0x0,MSG_LEN);
    sprintf(spiffs_info,"Total bytes : %d\nused bytes : %d\nblock size : %d\npage size : %d\nmax open files : %d\nmax path length:%d\n",
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
    // startMode = digitalRead(CONFIG_PIN);
    if (digitalRead(CONFIG_PIN))
    {
        startAP();
    }
    else
    {
        apMode = LOW;
        Serial.println("start STA Mode");
    }
    pinMode(CONFIG_PIN, OUTPUT);
}

void loop(void)
{
    if (apMode)
    {
        server.handleClient();
    }
    else
    {
        digitalWrite(CONFIG_PIN, HIGH);
    }
}