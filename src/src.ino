// RTC Libs
#include "time.h"
// SQLite3 Libs
#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include <sqlite3.h>
#include "SPIFFS.h"
// Web Server Libs
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// RFID Libs
#include <MFRC522.h>
#include <SPI.h>

#define SS_PIN 4
#define RST_PIN 9

#ifndef STASSID
#define STASSID "Momchilovi1"
#define STAPSK "momchilovi93"
#endif

#define COLUMN_COUNT 3

const char *SSID = STASSID;
const char *PASSWORD = STAPSK;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const char *NTP_SERVER = "europe.pool.ntp.org";
const long GMT_OFFSET_SEC = 7200;
const int DAYLIGHT_OFFSET_SEC = 3600;

sqlite3 *DataBase;
//int rc;
sqlite3_stmt *res;
const char *TAIL;

volatile int UserCount = 0;

// NTP Function
void DisplayLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

std::vector<String> results;
const char *data = "Callback function called";

static int callback(void *data, int argc, char **argv, char **azColName)
{
  results.clear();
  for (int i = 0; i < argc; ++i)
  {
    String current = String(argv[i]);
    if (current != "NULL")
      results.push_back(current);
  }
  return 0;
}

// Open DataBase Table
int db_open(const char *filename, sqlite3 **db)
{
  int rc = sqlite3_open(filename, db);
  if (rc)
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
  else
    Serial.printf("Opened database successfully\n");

  return rc;
}

// Execute SQL Queries
char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql)
{
  Serial.println(sql);
  int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
  if (rc != SQLITE_OK)
  {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else
  {
    Serial.printf("Operation done successfully\n");
  }
  return rc;
}

// Server Handlers
void HandleRoot()
{
  File file = SPIFFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void HandleNotFound()
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
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}

String StackPills(String id)
{
  String pResult = "";
  String getPillsQuery = "SELECT medicines FROM MEDICINES WHERE id='" + id + "';";
  db_exec(DataBase, getPillsQuery.c_str());

  for (String r : results)
    pResult += (r + " ");

  return pResult;
}

void GetDataForStartingTable()
{
  String getIDsQuery = "SELECT id FROM DETAILS;";
  String getNamesQuery = "SELECT person FROM DETAILS;";

  db_exec(DataBase, getIDsQuery.c_str());
  const int LEN = results.size();
  String IDs[LEN], Names[LEN], Pills[LEN];
  std::copy(results.begin(), results.end(), IDs);

  db_exec(DataBase, getNamesQuery.c_str());
  std::copy(results.begin(), results.end(), Names);

  for (int i = 0; i < LEN; ++i)
    Pills[i] = StackPills(IDs[i]);

  for (int i = 0; i < LEN; ++i)
  {
    webSocket.sendTXT(0, IDs[i] + ";" + Names[i] + ";" + Pills[i]);
    delay(10);
  }
}

void PrintTable()
{
  String getDetailsTable = "SELECT * from Details;";
  db_exec(DataBase, getDetailsTable.c_str());

  String getMedicinesTable = "SELECT * from Medicines;";
  db_exec(DataBase, getMedicinesTable.c_str());
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    UserCount--;
    break;

  case WStype_CONNECTED:
    UserCount++;
    if (UserCount == 1)
      GetDataForStartingTable();
    sqlite3_finalize(res);
    break;

  case WStype_TEXT:
    if (UserCount == 1)
    {
      String sql = ((const char *)payload);
      Serial.println(sql);
      db_exec(DataBase, sql.c_str());

      PrintTable();
   
    }
    break;
  }
}

int ListSPIFFSContent(File *root)
{
  if (!(*root && root->isDirectory()))
  {
    Serial.println("Not a directory or failed opening");
    return 0;
  }

  File file = root->openNextFile();

  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("\tDIR: ");
      Serial.println(file.name());
    }
    else
    {
      Serial.print("\tFILE: ");
      Serial.print(file.name());
      Serial.print(" with SIZE: ");
      Serial.println(file.size());
    }
    file = root->openNextFile();
  }

  return 1;
}

void ConfigServer()
{
  server.on("/", HandleRoot);
  server.onNotFound(HandleNotFound);
  server.begin();
}

void ConfigWebSocket()
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void setup(void)
{
  Serial.begin(115200);
  SPIFFS.begin();

  // Connect to a WiFi AP
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print("Loading...");
  }

  Serial.println("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  File root = SPIFFS.open("/spiffs/");
  if (!ListSPIFFSContent(&root))
    return;

  sqlite3_initialize(); //Init SQLite3

  if (db_open("/spiffs/data.db", &DataBase))
    return;

  ConfigServer();
  ConfigWebSocket();

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  DisplayLocalTime();

  Serial.println("Server started.");
}

void loop(void)
{
  server.handleClient();
  webSocket.loop();
}
