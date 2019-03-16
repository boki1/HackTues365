#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <MFRC522.h>
#include <sqlite3.h>
#include <vfs.h>
#include <SPI.h>
#include <FS.h>
extern "C" {
#include "user_interface.h"
}

#define SS_PIN 5
#define RST_PIN 9
#ifndef STASSID
#define STASSID "Momchilovi1"
#define STAPSK  "momchilovi93"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;
ESP8266WebServer server(80);

sqlite3 *db1;
int rc;
sqlite3_stmt *res;
int rec_count = 0;
const char *tail;


MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];

const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
   int i;
   Serial.printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   Serial.printf("\n");
   return 0;
}

char temp[] = "<!DOCTYPE html> \
<head> \
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /> \
<title>Spenser Web UI</title> \
<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\"> \
<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\"> \
<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\" integrity=\"sha384-ggOyR0iXCbMQv3Xipma34MD+dH/1fQ784/j6cY/iJTQUOhcWr7x9JvoRxT2MZw1T\" crossorigin=\"anonymous\"> \
</head> \
<body> \
  <div class=\"bg-image\"></div> \
  <div class=\"main\"> \
    <div id=\"_navbar\"> \
      <nav class=\"navbar navbar-dark bg-dark\" > \
        <button onclick=\"showAdder()\" id=\"add-btn\"><i class=\"fa fa-plus\"></i></button> \
        <form id=\"search-form\" class=\"form-inline my-2 my-lg-0\"> \
          <input onkeyup=\"filterTable()\" id=\"search-inp\" style=\"font-size: 20px;\" class=\"form-control mr-sm-2\" type=\"search\" placeholder=\"Search\" aria-label=\"Search\"> \
          <button class=\"search-btn\" type=\"submit\"><i class=\"fa fa-search\"></i></button> \
        </form> \
      </nav> \
    </div> \
    <div id=\"db-table\"> \
      <table cellspacing=2 cellpadding=5 class=\"table table-hover\"> \
        <thead> \
          <tr> \
            <th scope=\"col\">ID</th> \
            <th scope=\"col\">Name</th> \
            <th scope=\"col\">Pills</th> \
          </tr> \
        </thead> \
        <tbody> \
          <tr> \
            <td>1</td> \
            <td contenteditable=\"true\">gosho</td> \
            <td contenteditable=\"true\">a, b</td> \
          </tr> \
          <tr> \
            <td>2</td> \
            <td contenteditable=\"true\">pesho</td> \
            <td contenteditable=\"true\">a, b</td> \
          </tr> \
          <tr> \
            <td>1</td> \
            <td contenteditable=\"true\">mitko</td> \
            <td contenteditable=\"true\">a, b</td> \
          </tr> \
        </tbody> \
      </table> \
    </div> \
  </div> \
</body> \
<style> \
  #db-table { \
    width: 800px; \
    margin: 0px auto; \
    font-size: 24px; \
  } \
  .table > tbody > tr > td { \
     background-color: lightgrey; \
     color: black; \
  } \
  .table > tbody > tr > th { \
     background-color: lightgrey; \
     color: black; \
  } \
  .table > thead > tr > th { \
    background-color: grey; \
    color: black; \
  } \
  .table-hover tbody tr:hover td, .table-hover tbody tr:hover th { \
    background-color: lightgreen; \
  } \
  #menu-lst { \
    list-style-type: none; \
    margin: 0; \
    padding: 0; \
    width: 20%; \
    position: fixed; \
    height: 100%; \
    overflow: auto; \
    display: none; \
    transition: 1s; \
  } \
  .menu-opt a { \
    display: block; \
    color: #000; \
    padding: 8px 16px; \
    text-decoration: none; \
    transition: .5s linear; \
  } \
  .menu-opt a:hover { \
    background-color: rgb(51, 51, 51, 120); \
    color: white; \
    text-decoration: none; \
  } \
  #search { \
    display: inline-block; \
    text-align: center; \
    margin: 0 auto; \
    width: 475px; \
    margin: 44px 44px 44px 300px; \
    background-color: rgb(51, 51, 51); \
    color: black; \
  } \
  .search-btn { \
    background-color: rgb(51, 51, 51, 0); \
    color: white; \
    border: none; \
    transition: .5s linear; \
  } \
  .search-btn { \
    color: lightgreen; \
  } \
  #_navbar { \
    background-color: rgb(51, 51, 51, 120); \
    margin: 30px; \
    font-size: 24px; \
    position: relative; \
  } \
  body, html { \
    margin: 0; \
    height: 100%; \
  } \
  #add-btn { \
    background-color: rgb(51, 51, 51, 0); \
    color: white; \
    border: none; \
    transition: .5s linear; \
  } \
  #add-btn:hover { \
    color: lightgreen; \
  } \
  #adder { \
    display: none; \
  } \
</style> \
<script> \
  function filterTable () { \
    let value; \
    let input = document.getElementById(\"search-inp\"); \
    let filter = input.value.toUpperCase(); \
    let table = document.getElementById(\"db-table\"); \
    let tr = table.getElementsByTagName(\"tr\"); \
    for (i = 0; i < tr.length; ++i) { \
      td = tr[i].getElementsByTagName(\"td\")[1]; \
      if (td) { \
        value = td.textContent || td.innerText; \
        if (value.toUpperCase().indexOf(filter) > -1) { \
          tr[i].style.display = \"\"; \
        } else { \
          tr[i].style.display = \"none\"; \
        } \
      } \
    } \
  } \
  function calcNextIdx () { \
    return document.getElementById(\"db-table\").getElementsByTagName(\"tr\").length; \
  } \
</script> \
</html>";


void handleRoot() {
  snprintf(temp, 1000, temp);
  server.send(1000, "text/html", temp);
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
   Serial.println(micros()-start);
   return rc;
}

void setup(void) {
  Serial.begin(115200);
  SPI.begin();
  
  rfid.PCD_Init();
  
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

  if (!SPIFFS.begin()) {
       Serial.println("Failed to mount file system");
       return;
   }

   // list SPIFFS contents
   Dir dir = SPIFFS.openDir("/");
   while (dir.next()) {    
       String fileName = dir.fileName();
       size_t fileSize = dir.fileSize();
       Serial.printf("FS File: %s, size: %ld\n", fileName.c_str(), (long) fileSize);
   }
   Serial.printf("\n");

   sqlite3_initialize();
   File db_file_obj_1;
   vfs_set_spiffs_file_obj(&db_file_obj_1);
   if (db_open("/FLASH/pills.db", &db1))
       return;

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}










void edit_values(){
rc = db_exec(db1, "INSERT INTO test1 VALUES (" + value +","+ value2");");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
}

void user_info(){
  rc = db_exec(db1, "SELECT * FROM user_info");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
}



void loop(void) {
  server.handleClient();
  
  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  }
  else Serial.println(F("Card read previously."));
  rfid.PCD_StopCrypto1();
}


// Print RFID HEX VALUE
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
