#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "ERICESP"
#define STAPSK "!Qaz@Wsx"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

uint8_t startMode = HIGH;

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
    server.on("/postplain/", handlePlain);
    server.on("/postform/", handleForm);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}

void setup(void)
{
    delay(3000);
    pinMode(CONFIG_PIN, INPUT);
    startMode = digitalRead(CONFIG_PIN);
    if (startMode)
    {
        startAP();
    }
    else
    {
        Serial.println("start STA Mode");
    }
}

void loop(void)
{
    server.handleClient();
}