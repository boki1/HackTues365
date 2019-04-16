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

AsyncUDP udp;

#define SS_PIN 4
#define RST_PIN 9

#define COLUMN_COUNT 3

#ifndef STASSID
#define STASSID "Robotics"
#define STAPSK "elsysbot&"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const char *ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 3600;

sqlite3 *Database;
int rc;
sqlite3_stmt *res;
const char *tail;

volatile int userCount = 0;

void PrintLocalTime()
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
  for (int i = 0; i < argc; ++i)
  {
    String current = String(argv[i]);
    if (current != "NULL")
      results.push_back(current);
    else
      return 1;
  }
  return 0;
}

int OpenDatabase(const char *filename, sqlite3 **db)
{
  int rc = sqlite3_open(filename, db);
  if (rc)
  {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  }
  else
  {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}

char *zErrMsg = 0;
void ExecuteQuery(sqlite3 *db, const char *sql)
{
  rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
  if (rc != SQLITE_OK)
  {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else
  {
    Serial.printf("Operation done successfully\n");
  }
}

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
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void WiFiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("WiFi Failed");
    while (1)
    {
      delay(1000);
      ESP.restart();
    }
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void ManageFileSystem()
{
  File root = SPIFFS.open("/spiffs/");
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void setup(void)
{
  Serial.begin(115200);
  SPIFFS.begin();

  WiFiConnect();
  ManageFileSystem();

  sqlite3_initialize(); //Init SQLite3

  if (OpenDatabase("/spiffs/data.db", &Database))
    return;

  server.on("/", HandleRoot);
  server.onNotFound(HandleNotFound);
  server.begin();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  PrintLocalTime();

  Serial.println("Server started");

  if (udp.listen(420))
  {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
      Serial.print("UDP Packet Type: ");
      Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length());
      Serial.print(", Data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println();
      //reply to the client
      packet.printf("Got %u bytes of data", packet.length());
    });
  }
  webSocket.begin();
  webSocket.onEvent(WebSocketEvent);
}

void loop(void)
{
  server.handleClient();
  webSocket.loop();
}

void StackResults()
{
  std::vector<String> _new;
  String curr = "";
  int i = 0;
  for (String r : results)
  {
    if (i >= COLUMN_COUNT)
    {
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

String StackPills(String id)
{
  String pillsResult = "";
  String getPills = "SELECT medicines from medicines where id='" + id + "';";
  ExecuteQuery(Database, getPills.c_str());

  for (int i; i < results.size() - 1; ++i)
    pillsResult += (results[i] + ", ");
  pillsResult += results[results.size() - 1];
  results.clear();
  return pillsResult;
}

String PrepFullInfo()
{
  String fullInfo = "";
  for (String r : results)
  {
    fullInfo += (r + ";");
  }
  results.clear();
  return fullInfo;
}

String PrepDetails()
{
  String details = "";
  for (String r : results)
  {
    details += r;
  }
  return details;
}

void OnDisconnect()
{
  userCount--;
}

void OnConnect()
{
  userCount++;
  if (userCount != 1)
    return;

  String GetIdQuery = "SELECT id from details;";
  ExecuteQuery(Database, GetIdQuery.c_str());
  std::vector<String> IDs = results;
  results.clear();

  String GetNameQuery = "SELECT person from details;";
  ExecuteQuery(Database, GetNameQuery.c_str());
  std::vector<String> Names = results;
  results.clear();

  std::vector<String> Pills;
  for (String id : IDs)
    Pills.push_back(StackPills(id));

  const int LEN = IDs.size();
  for (int i = 0; i < LEN; ++i)
  {
    webSocket.sendTXT(0, "main;" + IDs[i] + ";" + Names[i] + ";" + Pills[i]);
    delay(10);
  }
}

void HandleMainQuery(String query)
{
  ExecuteQuery(Database, query.c_str());
  Serial.println("SQL query in 'HandleMainQuery()': " + query);
  if (rc != SQLITE_OK)
    Serial.println("Error in executing query 'HandleMainQuery'");
}

void HandleEditQuery(String query)
{
  Serial.println("ID passed to 'HandleEditQuery()': " + query);
  String DetailQuery = "SELECT * FROM Details WHERE id='" + query + "';";
  ExecuteQuery(Database, DetailQuery.c_str());
  String Details = PrepDetails();

  if (rc != SQLITE_OK)
    Serial.println("Error in executing query 'HandleEditQuery'");

  String fullInfo = PrepFullInfo();
  String HoursQuery = "SELECT hours FROM Medicines where id='" + query + "';";
  ExecuteQuery(Database, HoursQuery.c_str());
  std::vector<String> Hours = results;
  results.clear();

  String MedQuery = "SELECT medicines FROM Medicines where id='" + query + "';";
  ExecuteQuery(Database, MedQuery.c_str());
  std::vector<String> Meds = results;
  results.clear();

  String fPills = "";
  for (int i = 0; i < Meds.size(); ++i)
    fPills += (Meds[i] + " -> " + Hours[i] + " | ");

  webSocket.sendTXT(0, "edit;" + fullInfo + fPills + ";");
  DetailQuery = "";
  Serial.println("edit;" + fullInfo + fPills + ";");
  fPills = "";
}

void OnMessage(String fullQuery)
{
  if (userCount != 1)
    return;

  String statusMsg = fullQuery.substring(0, 4);
  String query = fullQuery.substring(5);
  Serial.println("Query in 'OnMessage()': " + query);
  Serial.println("Status message in 'OnMessage()': " + statusMsg);
  if (statusMsg == "main")
    HandleMainQuery(query);
  else if (statusMsg == "edit")
    HandleEditQuery(query);
}

void WebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
    case WStype_DISCONNECTED:
      OnDisconnect();
      break;
      
    case WStype_CONNECTED:
      OnConnect();
      break;
      
    case WStype_TEXT:
      String query = (const char *) payload;
      OnMessage(query);
      break;
  }
}
