bool CheckMedTime(String candidate, struct tm *current)
{
   
}

void startRFID()
{
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;
  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  Serial.println(GetIDCardInHex(key.keyByte, MFRC522::MF_KEY_SIZE));
}

String GetIDCardInHex(byte *buffer, byte bufferSize) {
  String idCard = "";
  for (byte i = 0; i < bufferSize; i++) {
    idCard += (String(buffer[i] < 0x10 ? " 0" : " "));
    idCard += String(buffer[i], HEX);
  }
  idCard = idCard.substring(1, idCard.length());
  idCard.toUpperCase();
  return idCard;
}

void MoveServo() {
  for (int _position = 0; _position <= 70; _position++) {
    myServo.write(_position);
    delay(13);
  }

  for (int _position = 70; _position >= 0; _position--) {
    myServo.write(_position);
    delay(13);
  }
}

void OnClick(byte *buff, byte _size, struct tm *timeinfo)
{
  String idCard = GetIDCardInHex(buff, _size);
  String getIDQuery = "SELECT id FROM id_cards WHERE id_card='" + idCard + "';";
  ExecuteQuery(Database, getIDQuery.c_str());
  String humanID = results[0];
  results.clear();

  String _getHoursQuery = "SELECT hours FROM Medicines WHERE id='" + humanID + "';";
  ExecuteQuery(Database, _getHoursQuery.c_str());
  results.clear();

  struct tm current = _GetLocalTime();
  for (int i = 0; i < results.size(); ++i)
  {
    if (!CheckMedTime(hour, current))
      continue;
  }

  String _getMedicinesQuery = "SELECT medicines FROM Medicines WHERE id='" + humanID + "';";
  ExecuteQuery(Database, _getMedicinesQuery.c_str());
  results.clear();

  MoveServo();
}
