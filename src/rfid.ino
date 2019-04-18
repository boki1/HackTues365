//std::vector<int> split(String str, String delim)
//{
//  Serial.println("into the split, str" + str + "; length: " + String(str.length()));
//  std::vector<int> res;
//  int i = 2;
//  res.push_back((int) str.substring(0, 2).toInt());
//  Serial.println("into the end");
//  for (; i < str.length(); i++)
//    res.push_back((int) str.substring(i + 1, i + 2).toInt());
//
//  for (int r: res)
//    Serial.printf("%d ", r);
//  return res;
//}

  std::vector<int> splittedByCol;
  struct sd
  {
    int a : 5;
  };

bool CheckMedTime(String candidate, struct tm *current)
{
  String sDelColumn = ":";
  String sDelCommaSpace = ", ";
  //  Serial.println("Im in, 'CheckMedTime()' with cand " + String(candidate));
  candidate.replace(sDelCommaSpace, sDelColumn);
  Serial.print("ESP time (candidate): ");
  //Serial.println(candidate);
  Serial.println("#####candidate.length" + candidate.length());
  for (int i = 0; i < candidate.length() - 2; i+=2) 
  {
     splittedByCol[i]= candidate[i];
     splittedByCol[i+1]= candidate[i+1];
     Serial.print("While splitting: i=");
     Serial.print(splittedByCol[i]);
     Serial.print(" i + 1 =");
     Serial.println(splittedByCol[i+1]);
  }

  
  
  return false;
}

void startRFID()
{
  SPI.begin();
  rfid.PCD_Init();
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

  Serial.println("Results size: " + String(results.size()));

  struct tm *current = _GetLocalTime();
  std::vector<int> indexes;
  for (int i = 0; i < results.size(); ++i)
  {
    if (!CheckMedTime(results[i], current))
      continue;
    indexes.push_back(i);
  }

  Serial.print("Indexes incoming... size: " + String(indexes.size()) + "; ");
  for (int i : indexes)
    Serial.print(i + " ");

  results.clear();

  String _getMedicinesQuery = "SELECT medicines FROM Medicines WHERE id='" + humanID + "';";
  ExecuteQuery(Database, _getMedicinesQuery.c_str());
  results.clear();

  MoveServo();
}
