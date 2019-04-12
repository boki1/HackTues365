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

#define COLUMN_COUNT 3

#ifndef STASSID
#define STASSID "Momchilovi1"
#define STAPSK  "momchilovi93"
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
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

//--------------------------------------------------------------------------------------------
//   _____     _ _ _           _      _____ _____ __       _____             _   _
//  |     |___| | | |_ ___ ___| |_   |   __|     |  |     |   __|_ _ ___ ___| |_|_|___ ___ ___
//  |   --| .'| | | . | .'|  _| '_|  |__   |  |  |  |__   |   __| | |   |  _|  _| | . |   |_ -|
//  |_____|__,|_|_|___|__,|___|_,_|  |_____|__  _|_____|  |__|  |___|_|_|___|_| |_|___|_|_|___|
//
std::vector<String> results;
const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
  //Serial.printf("%s: ", (const char*) data);
  for (int i = 0; i < argc; ++i) {
    String current = String(argv[i]);
    if (current != "NULL")
      results.push_back(current);
  }
  // Serial.printf("Results len: %d\n", results.size());
  return 0;
}

//---------------------------------------------------------
//   _____                _____ _____ __       ____  _____
//  |     |___ ___ ___   |   __|     |  |     |    \| __  |
//  |  |  | . | -_|   |  |__   |  |  |  |__   |  |  | __ -|
//  |_____|  _|___|_|_|  |_____|__  _|_____|  |____/|_____|
//        |_|                     |__|

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
void db_exec(sqlite3 *db, const char *sql) {
  int rc = sqlite3_exec(db, sql, callback, (void*) data, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.printf("Operation done successfully\n");
  }
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

void setup(void) {

  Serial.begin(115200); //Start Serial conn
  SPIFFS.begin(); //Start the SPI Flash File System

  //============Connect to a WiFi AP============//
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //   __    _     _      _____ _____ _____ _____ _____ _____                _           _
  //  |  |  |_|___| |_   |   __|  _  |     |   __|   __|   __|   ___ ___ ___| |_ ___ ___| |_ ___
  //  |  |__| |_ -|  _|  |__   |   __|-   -|   __|   __|__   |  |  _| . |   |  _| -_|   |  _|_ -|
  //  |_____|_|___|_|    |_____|__|  |_____|__|  |__|  |_____|  |___|___|_|_|_| |___|_|_|_| |___|

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

  if (db_open("/spiffs/data.db", &db1))
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

void stack_results() {
  std::vector<String> _new;
  String curr = "";
  int i = 0;
  for (String r : results) {
    if (i >= COLUMN_COUNT) {
      _new.push_back(curr);
      curr = "";
      i = 0;
    }
    curr += (r + " ");
    i++;
  }
  results.clear();
  results = _new;
}

String stack_pills(String id) {
  String pillsResult = "";
  String getPills = "SELECT medicines from medicines where id='" + id + "';";
  db_exec(db1, getPills.c_str());

  for (int i; i < results.size() - 1; ++i)
    pillsResult += (results[i] + ", ");
  pillsResult += results[results.size() - 1];
  results.clear();
  return pillsResult;
}

String PrepFullInfo() {
  String fullInfo = "";
  for (String r : results) {
    fullInfo += (r + ";");
  }
  results.clear();
  return fullInfo;
}

String prepDetails() {
  String details = "";
  for (String r : results) {
    details += r;
  }
  return details;
}
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type) {
    case WStype_DISCONNECTED:
      userNum = userNum - 1;
      break;
    case WStype_CONNECTED:
      userNum++;
      if (userNum == 1) {
        //String rowCountQuery = "SELECT count(id) FROM details;";
        //db_exec(db1, rowCountQuery.c_str());

        String getIdQuery = "SELECT id from details;";
        db_exec(db1, getIdQuery.c_str());

        std::vector<String> IDs = results;
        results.clear();

        String getNameQuery = "SELECT person from details;";
        db_exec(db1, getNameQuery.c_str());

        std::vector<String> Names = results;
        results.clear();


        std::vector<String> Pills;
        for (String id : IDs) {
          Pills.push_back(stack_pills(id));
        }
        Serial.printf("%d %d %d", IDs.size(), Names.size(), Pills.size());
        //        stack_results();
        //        Serial.printf("Results length: %d\n", results.size());
        for (String r : Pills) {
          Serial.println("Result: " + r);
        }

        const int LEN = IDs.size();
        for (int i = 0; i < LEN; ++i) {
          webSocket.sendTXT(0, "main;" + IDs[i] + ";" + Names[i] + ";" + Pills[i]);
          delay(10);
        }

      }
      sqlite3_finalize(res);
      break;

    case WStype_TEXT:
      if (userNum == 1) {
        String sql = ((const char*) payload);
        String statusMsg = sql.substring(0, 4);

        sql = sql.substring(5);
        //Serial.println("SQL: " + sql);
        //Serial.println("statusMsg: " + statusMsg);

        if (statusMsg == "main") {
          db_exec(db1, sql.c_str());
          Serial.println(sql);
          if (rc != SQLITE_OK)
            Serial.println("ne e dobre polojenieto");
        } else if (statusMsg == "edit") {
          Serial.println("SQL_EDIT: " + sql);
          Serial.println("SQL_STATUS_MSG: " + statusMsg);
          String DetailQuery = "SELECT * FROM Details where id='" + sql + "';";
          db_exec(db1, DetailQuery.c_str());
          String Details = prepDetails();

          if (rc != SQLITE_OK)
            Serial.println("ne e dobre polojenieto!");
          String fullInfo = PrepFullInfo();

          String HoursQuery = "SELECT hours FROM Medicines where id='" + sql + "';";
          db_exec(db1, HoursQuery.c_str());
          std::vector<String> Hours = results;
          results.clear();

          String MedQuery = "SELECT medicines FROM Medicines where id='" + sql + "';";
          db_exec(db1, MedQuery.c_str());
          std::vector<String> Meds = results;
          results.clear();
          
          String fPills = "";
          for (int i = 0; i < Meds.size(); ++i)
            fPills += (Meds[i] + " -> " + Hours[i] + ", ");
            
          webSocket.sendTXT(0, "edit;" + fullInfo + fPills + ";");
          DetailQuery = "";
          Serial.println("edit;" + fullInfo + fPills + ";");
          fPills = "";
        }
        break;
      }
  }
}
