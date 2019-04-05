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

#define SS_PIN 4
#define RST_PIN 9

#ifndef STASSID
#define STASSID "AndroidAP"
#define STAPSK  "hari1234"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const char* ntpServer = "europe.pool.ntp.org";
const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 3600;

sqlite3 *db1;
int rc;
sqlite3_stmt *res;
const char *tail;

volatile int userNum = 0; //user login id


//=========== NTP Function===================//

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

//=========== Callback SQL Functions=========//
static const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
  int i;
  Serial.printf("%s: ", (const char*)data);
  for (i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.printf("\n");
  return 0;
}

//================ Open SQL function============//
int db_open(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}


//=============== Execute SQL lines========//
char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql ) {
  Serial.println(sql);
  long start = micros();
  int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.printf("Operation done successfully\n");
  }
  Serial.print(F("Time taken:"));
  Serial.println(micros() - start);
  return rc;
}

//==============Server handlers===========//
void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

#include <esp_wifi.h> //NEED THIS TO COMPILE
#define WIFI_CHANNEL 1
uint8_t masterCustomMac[] = {0x88, 0xBD, 0x45, 0xAE, 0x83, 0x8C};
 
void initVariant() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_mac(ESP_IF_WIFI_STA, &masterCustomMac[0]); // esp32 code
}



void setup(void) {
  
  Serial.begin(115200); //Start Serial conn
  SPIFFS.begin(); //Start the SPI Flash File System

//============Connect to a WiFi AP============//
  initVariant();
  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println( WiFi.macAddress() );
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

//===========List SPIFFS contents=================//
  File root = SPIFFS.open("/spiffs/");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }

  sqlite3_initialize(); //Init SQLite3

  if (db_open("/spiffs/pills.db", &db1))
    return;

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  
  Serial.println("Server started");
}

void loop(void) {
  server.handleClient();
  webSocket.loop();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) 
  {
  switch (type) {
    case WStype_DISCONNECTED:
      userNum = userNum - 1;
        break;
    case WStype_CONNECTED:
      {
        userNum = userNum + 1;
        if (userNum == 1){
//        String sql = "Select * from user_info2 where card_id is not null;";
//        rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
//        if (rc != SQLITE_OK) {
//          String resp = "Failed to fetch data: ";
//          resp += sqlite3_errmsg(db1);
//          Serial.println(resp.c_str());
//          return;
//        }
//        while (sqlite3_step(res) == SQLITE_ROW) {
//          Serial.println((const char *) sqlite3_column_text(res, 1));
//          webSocket.sendTXT(0, sqlite3_column_text(res, 1));
//         }
//       }
//      sqlite3_finalize(res);
        String row_count_query = "select count(card_id) from user_info2";
        db_exec(db1, row_count_query.c_str());
        Serial.println("DATA: "  data);
        }
      break;
    case WStype_TEXT:
    {
      if (userNum == 1){
      String sql = ((const char*) payload);
      db_exec(db1, sql.c_str());
      if (rc != SQLITE_OK) {
        Serial.println("ne e dobre polojenieto");
        }
      String sql2 = "Select * from user_info2";
      db_exec(db1, sql2.c_str()); 
      if (rc != SQLITE_OK) {
        Serial.println("ne e dobre polojenieto");
        }
       }
      }
     }
      break;
  }
}
