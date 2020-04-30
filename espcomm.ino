#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

//AP模式下連線的SSID跟PSK
#define STASSID "ERICESP"
#define STAPSK "!Qaz@Wsx"

#define MSG_LEN 1024
#define SSID_LEN 40
#define PSK_LEN 20
#define JSON_BUFFER_SIZE 1024

const char *ssid = STASSID;
const char *password = STAPSK;
const char *id_of_ssid = "ssid";
const char *id_of_psk = "psk";

char spiffs_info[MSG_LEN];
char ap_ssid[SSID_LEN];
char ap_psk[PSK_LEN];
char tmp_buf[JSON_BUFFER_SIZE];

boolean apMode = true;
size_t tmpLen;

ESP8266WebServer server(80);

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

//顯示預設進入的網頁
void handleRoot()
{
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send(200, "text/html", postForms);
}

//處理post上來的資料
void handleForm()
{
    if (server.method() != HTTP_POST)
    {
        server.send(405, "text/plain", "Method Not Allowed");
    }
    else
    {
        for (uint8_t i = 0; i < server.args(); i++)
        {
            if (!strcmp(id_of_ssid, server.argName(i).c_str()))
            {
                tmpLen = strlen(server.arg(i).c_str());
                if (tmpLen > SSID_LEN)
                    tmpLen = SSID_LEN;
                memset(ap_ssid, 0x0, SSID_LEN);
                memcpy(ap_ssid, server.arg(i).c_str(), tmpLen);
            }
            else if (!strcmp(id_of_psk, server.argName(i).c_str()))
            {
                tmpLen = strlen(server.arg(i).c_str());
                if (tmpLen > PSK_LEN)
                    tmpLen = PSK_LEN;
                memset(ap_psk, 0x0, PSK_LEN);
                memcpy(ap_psk, server.arg(i).c_str(), tmpLen);
            }
        }
        saveConfig();
        String message = "Flash info:\n";
        dumpFSInfo();
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

//啟動AP模式
void startAP()
{
    Serial.println("Start AP mode.");
    //讓LED閃兩下
    digitalWrite(2,HIGH);
    delay(500);
    digitalWrite(2,LOW);
    delay(500);
    digitalWrite(2,HIGH);
    delay(500);
    digitalWrite(2,LOW);

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

//讀取設定值
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
                Serial.printf("config file size :%d\n", size);
                // Allocate a buffer to store contents of the file.
                memset(tmp_buf, 0x0, JSON_BUFFER_SIZE);
                configFile.readBytes(tmp_buf, size);
                Serial.printf("config file :%s\n", tmp_buf);
                DynamicJsonDocument doc(JSON_BUFFER_SIZE);
                deserializeJson(doc, tmp_buf);
                Serial.println("\nparsed json");

                strcpy(ap_ssid, doc["ap_ssid"]);
                strcpy(ap_psk, doc["ap_psk"]);
                Serial.printf("ap_ssid:%s\n", ap_ssid);
                Serial.printf("ap_psk:%s\n", ap_psk);
            }
        }
        SPIFFS.end();
    }
    else
    {
        Serial.println("failed to mount FS");
    }
}

//儲存設定值
void saveConfig()
{
    Serial.println("saving config");
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    Serial.printf("ap_ssid:%s\n", ap_ssid);
    Serial.printf("ap_psk:%s\n", ap_psk);
    doc["ap_ssid"] = ap_ssid;
    doc["ap_psk"] = ap_psk;

    if (SPIFFS.begin())
    {
        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile)
        {
            Serial.println("failed to open config file for writing");
        }

        serializeJson(doc, configFile);
        configFile.close();
        SPIFFS.end();
    }
    else
    {
        Serial.println("failed to mount FS");
    }
}

void dumpFSInfo()
{
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
}

//連線到基地台
boolean connectWIFI()
{
    return false;
}

void setup(void)
{
    pinMode(2,OUTPUT);
    Serial.begin(115200);
    loadConfig();
    //如果連不上WIFI，就進入AP模式
    apMode = !connectWIFI();
    if (apMode)
        startAP();
}

void loop(void)
{
    if (apMode)
        server.handleClient();
}