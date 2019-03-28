#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <MFRC522.h>
#include <sqlite3.h>
#include <SPI.h>
#include <FS.h>
#include "SPIFFS.h"

#define SS_PIN 4
#define RST_PIN 9
#ifndef STASSID
#define STASSID "Momchilovi1"
#define STAPSK  "momchilovi93"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;
WebServer server(80);

sqlite3 *db1;
int rc;

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
  int i;
  Serial.printf("%s: ", (const char*)data);
  for (i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.printf("\n");
  return 0;
}

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

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
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

void handleSearch(){ 
  File file = SPIFFS.open("/index.html", "r");
  server.streamFile(file, "text/html");

  String sql = "Select * from user_info where name between '";
      sql += server.arg("from");
      sql += "' and '";
      sql += server.arg("to");
      sql += "'";
      rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
      if (rc != SQLITE_OK) {
          String resp = "Failed to fetch data: ";
          resp += sqlite3_errmsg(db1);
          resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
          server.send ( 200, "text/html", resp.c_str());
          Serial.println(resp.c_str());
          return;
      }
      while (sqlite3_step(res) == SQLITE_ROW) {
          rec_count = sqlite3_column_int(res, 1);
          if (rec_count > 5000) {
              String resp = "Too many records: ";
              resp += rec_count;
              resp += ". Please select different range";
              resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
              server.send ( 200, "text/html", resp.c_str());
              Serial.println(resp.c_str());
              sqlite3_finalize(res);
              return;
          }
      }
      sqlite3_finalize(res);

      String sql = "Select card_id, name, medicine, hours from user_info where name between '";
      sql += server.arg("from");
      sql += "' and '";
      sql += server.arg("to");
      sql += "'";
      rc = sqlite3_prepare_v2(db1, sql.c_str(), 1000, &res, &tail);
      if (rc != SQLITE_OK) {
          String resp = "Failed to fetch data: ";
          resp += sqlite3_errmsg(db1);
          resp += "<br><br><a href='/'>back</a>";
          server.send ( 200, "text/html", resp.c_str());
          Serial.println(resp.c_str());
          return;
      }

      rec_count = 0;
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      String resp = "<html><head><title>ESP32 Sqlite local database query through web server</title>\
          <style>\
          body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: large; Color: #000088; }\
          </style><head><body><h1>ESP32 Sqlite local database query through web server</h1><h2>";
      resp += sql;
      resp += "</h2><br><table cellspacing='1' cellpadding='1' border='1'><tr><td>card_id</td><td>name</td><td>medicine</td><td>hours</td>";
      server.send ( 200, "text/html", resp.c_str());
      while (sqlite3_step(res) == SQLITE_ROW) {
          resp = "<tr><td>";
          resp += sqlite3_column_int(res, 0);
          resp += "</td><td>";
          resp += (const char *) sqlite3_column_text(res, 1);
          resp += "</td><td>";
          resp += (const char *) sqlite3_column_text(res, 2);
          resp += "</td><td>";
          resp += sqlite3_column_int(res, 3);
          resp += (const char *) sqlite3_column_text(res, 4);
          server.sendContent(resp);
          rec_count++;
      }
      resp = "</table><br><br>Number of records: ";
      resp += rec_count;
      resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
      server.sendContent(resp);
      sqlite3_finalize(res);
  
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
  Serial.begin(115200);
  SPIFFS.begin();

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

  // list SPIFFS contents
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

   sqlite3_initialize();

   if (db_open("/spiffs/pills.db", &db1))
       return;

  //server.serveStatic("/", SPIFFS, "/index.html");
  server.on("/", handleSearch);
  
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
}

