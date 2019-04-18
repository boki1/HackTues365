
//===================RTC Libs==================//
#include "time.h"
//==================SQLite3 Libs==============//
#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include <sqlite3.h>
#include "SPIFFS.h"
//==================Web server Libs==========//
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
//=================RFID Libs================//
#include <MFRC522.h>
#include <SPI.h>
//=================Base connection==========//
#include "WiFi.h"
#include "AsyncUDP.h"
//=================mDNS=====================//
#include <ESPmDNS.h>
//=================RFID=====================//
#include <SPI.h>
#include <MFRC522.h>
//=================Servo====================//
#include <ESP32Servo.h>


#define COLUMN_COUNT 3

#ifndef STASSID
#define STASSID "viluu"
#define STAPSK "v12345678"
#endif

sqlite3 *Database;

static std::vector<String> results;
const char *data = "Callback function called";
int rc;
sqlite3_stmt *res;
const char *tail;

const char *ssid = STASSID;
const char *password = STAPSK;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

volatile int userCount = 0;

constexpr uint8_t RST_PIN = 15;
constexpr uint8_t SS_PIN = 2;

MFRC522 rfid(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

byte nuidPICC[4];

Servo myServo;

const char *ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 3600;

AsyncUDP udp;

void setup(void)
{
  Serial.begin(115200);
  SPIFFS.begin();

  WiFiConnect();
  ManageFileSystem();
  mDNS();
  startRFID();

  sqlite3_initialize(); //Init SQLite3

  if (OpenDatabase("/spiffs/data.db", &Database))
    return;

  server.on("/", HandleRoot);
  server.onNotFound(HandleNotFound);
  server.begin();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("Server started");
  webSocket.begin();
  webSocket.onEvent(WebSocketEvent);

  myServo.attach(5); //servo pin
  myServo.write(0);

  if (udp.listen(420)) {
    udpLog();
  }
  udp.broadcast("Anyone here?");
  myServo.write(0);
}

void loop(void)
{
  server.handleClient();
  webSocket.loop();

  if (Serial.available() > 0)
  {
    char Sinput = Serial.read();

    if (Sinput == '1') {
      struct tm *timeinfo = _GetLocalTime();
      String Query = "INSERT INTO medicines VALUES(' #####', 'random pill', '" + String(timeinfo->tm_hour) + ":" + String(timeinfo->tm_min) + "');";
      Serial.println(Query);
      ExecuteQuery(Database, Query.c_str());
    }
  }

  if ( ! rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return;
  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] )
  {

    for (byte i = 0; i < 4; i++)
      nuidPICC[i] = rfid.uid.uidByte[i];

    OnClick(rfid.uid.uidByte, rfid.uid.size, _GetLocalTime());
  }
  else Serial.println(F("Card read previously."));

  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();
  udp.broadcast("Anyone here?");
}
