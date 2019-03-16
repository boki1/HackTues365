#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "HackTUES 365"
#define STAPSK  "elsysisthebest"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

const int led = 13;
char *temp = {" <!DOCTYPE html> \
<head> \
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /> \
<title>Spenser Web UI</title> \
<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\"> \
<link rel=\"stylesheet\" type=\"text/css\" media=\"screen\" href=\"stylesheet.css\" /> \
<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\"> \
<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\" integrity=\"sha384-ggOyR0iXCbMQv3Xipma34MD+dH/1fQ784/j6cY/iJTQUOhcWr7x9JvoRxT2MZw1T\" crossorigin=\"anonymous\"> \
<script type=\"text/javascript\" src=\"js/jquery-1.2.6.min.js\"></script> \
<script type=\"text/javascript\" src=\"js/index.js\"></script> \
</head> \
<body> \
  <div class=\"bg-image\"></div> \
  <div class=\"main\"> \
    <div id=\"_navbar\"> \
      <nav class=\"navbar navbar-dark bg-dark\" > \
        <button onclick=\"showAdder()\" id=\"add-btn\"><i class=\"fa fa-plus\"></i></button>             \
        <form id=\"search-form\" class=\"form-inline my-2 my-lg-0\"> \
          <input onkeyup=\"filterTable()\" id=\"search-inp\" style=\"font-size: 20px;\" class=\"form-control mr-sm-2\" type=\"search\" placeholder=\"Search\" aria-label=\"Search\"> \
          <button class=\"search-btn\" type=\"submit\"><i class=\"fa fa-search\"></i></button> \
        </form> \
      </nav> \
    </div> \
    <div id=\"db-table\"> \
      <table cellspacing=2 cellpadding=5 class=\"table table-hover\" id=\"data_table\"> \
        <thead> \
          <tr> \
            <th scope=\"col\">ID</th> \
            <th scope=\"col\">Name</th> \
            <th scope=\"col\">Pills</th> \
          </tr> \
        </thead> \
        <tbody> \
        </tbody> \
      </table> \
    </div> \
    <div id=\"adder\"> \
      <input type=\"text\" id=\"new_name\"> \
      <input type=\"text\" id=\"new_pills\"> \
      <input type=\"button\" class=\"add\" onclick=\"add_row();\" value=\"Add Row\"> \
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
.btn-arr:hover { \
  color: white; \
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
.search-btn, .edit-btn, .delete-btn, .save-btn { \
  background-color: rgb(51, 51, 51, 0); \
  color: white; \
  border: none;  \
  transition: .5s linear; \
} \
.search-btn, .edit-btn, .delete-btn, .save-btn { \
  color: lightgreen; \
} \
#_navbar { \
  background-color: rgb(51, 51, 51, 120); \
  margin: 30px; \
  font-size: 24px;  \
  position: relative; \
} \
body, html { \
  margin: 0; \
  height: 100%; \
} \
.bg-image { \
  background-image: url(\"background.jpeg\"); \
  height: 100%;  \
  filter: blur(8px); \
  background-position: center; \
  background-repeat: no-repeat; \
  background-size: contain; \
} \
.main { \
  top: 20px; \
  position: absolute; \
  left: 30%; \
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
  /* top: 85%; */ \
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
    td = tr[i].getElementsByTagName(\"td\")[0]; \
    if (td) { \
      value = td.textContent || td.innerText; \
      if (value.toUpperCase().indexOf(filter) > -1) { \
        tr[i].style.display = \"\"; \
      } else { \
        tr[i].style.display = \"none\"; \
      } \
    }        \
  } \
} \
function showAdder() { \
  document.getElementById(\"adder\").style.display = \"block\"; \
} \
function edit_row(no) \
{ \
 document.getElementById(\"edit_button\"+no).style.display=\"none\"; \
 document.getElementById(\"save_button\"+no).style.display=\"block\"; \
 var name=document.getElementById(\"name_row\"+no); \
 var country=document.getElementById(\"country_row\"+no); \
 var age=document.getElementById(\"age_row\"+no); \
 var name_data=name.innerHTML; \
 var country_data=country.innerHTML; \
 var age_data=age.innerHTML; \
 name.innerHTML=\"<input type='text' id='name_text\"+no+\"' value='\"+name_data+\"'>\"; \
 country.innerHTML=\"<input type='text' id='country_text\"+no+\"' value='\"+country_data+\"'>\"; \
 age.innerHTML=\"<input type='text' id='age_text\"+no+\"' value='\"+age_data+\"'>\"; \
} \
function save_row(no) \
{ \
 var name_val=document.getElementById(\"name_text\"+no).value; \
 var country_val=document.getElementById(\"country_text\"+no).value; \
 var age_val=document.getElementById(\"age_text\"+no).value; \
 document.getElementById(\"name_row\"+no).innerHTML=name_val; \
 document.getElementById(\"country_row\"+no).innerHTML=country_val; \
 document.getElementById(\"age_row\"+no).innerHTML=age_val; \
 document.getElementById(\"edit_button\"+no).style.display=\"block\"; \
 document.getElementById(\"save_button\"+no).style.display=\"none\"; \
} \
function delete_row(no) \
{ \
 document.getElementById(\"row\"+no+\"\").outerHTML=\"\"; \
} \
function add_row() \
{ \
 var new_name=document.getElementById(\"new_name\").value; \
 var new_country=document.getElementById(\"new_country\").value; \
 var new_age=document.getElementById(\"new_age\").value; \
 var table=document.getElementById(\"data_table\"); \
 var table_len=(table.rows.length)-1; \
 var row = table.insertRow(table_len).outerHTML=\"<tr id='row\"+table_len+\"'><td id='name_row\"+table_len+\"'>\"+new_name+\"</td><td id='country_row\"+table_len+\"'>\"+new_country+\"</td><td id='age_row\"+table_len+\"'>\"+new_age+\"</td><td><input type='button' id='edit_button\"+table_len+\"' value='Edit' class='edit' onclick='edit_row(\"+table_len+\")'> <input type='button' id='save_button\"+table_len+\"' value='Save' class='save' onclick='save_row(\"+table_len+\")'> <input type='button' value='Delete' class='delete' onclick='delete_row(\"+table_len+\")'></td></tr>\"; \
 document.getElementById(\"new_name\").value=\"\"; \
 document.getElementById(\"new_country\").value=\"\"; \
 document.getElementById(\"new_age\").value=\"\"; \
} \
</script> \
</html> "};


void handleRoot() {
  snprintf(temp, 1000, temp);
  server.send(1000, "text/html", temp);
}

void handleNotFound() {
  digitalWrite(led, 1);
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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
