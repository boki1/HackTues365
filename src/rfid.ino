std::vector<std::pair<int, int>> split(String str, String delim)
{
  Serial.println("into the split, str" + str + "; length: " + String(str.length()));
  std::vector<std::pair<int, int>> res;

  for (int i = 0; i < str.length(); i++)
  {
    if (String(str.charAt(i)) == delim)
    {
      res.push_back(std::make_pair((int) str.substring(i - 2, i).toInt(), (int) str.substring(i + 1, i + 3).toInt()));
      if (i + 4 <= str.length()) i += 4;
      else break;
    }
  }

  return res;
}

bool isPillTime(String candidate, struct tm *current)
{
  String sDelColumn = ":";
  String sDelCommaSpace = ", ";
  //  Serial.println("Im in, 'isPillTime()' with cand " + String(candidate));
  candidate.replace(sDelCommaSpace, sDelColumn);

  std::vector<std::pair<int, int>> hourMin = split(candidate, sDelColumn);
  unsigned minCandidate, minCurrent = current->tm_hour * 60 + current->tm_min;
  Serial.println("Current time by var: " + String(minCurrent));
  for (std::pair<int, int> p : hourMin)
  {
    minCandidate = p.first * 60 + p.second;
    Serial.println("Candidate time: " + String(minCandidate));
    if (abs(minCandidate - minCurrent) <= 30)
      return true;
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

void OnClick(byte *buff, byte _size, struct tm *timeinfo)
{
  digitalWrite(LED, HIGH);
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
    if (!isPillTime(results[i], current))
      continue;
    indexes.push_back(i);
  }

  Serial.print("Indexes incoming... size: " + String(indexes.size()) + "; ");
  for (int i : indexes)
    Serial.print(i + " ");

  results.clear();

  String _getMedicinesQuery = "SELECT medicines FROM Medicines WHERE id='" + humanID + "';";
  ExecuteQuery(Database, _getMedicinesQuery.c_str());
  std::vector<String> pillsToDrop;
  Serial.print("Im droppin..");
  if (indexes.size() == 0) Serial.print("nothing :(");
  for (int i : indexes) {
    if (std::find(indexes.begin(), indexes.end(), i) != indexes.end())
    {
      pillsToDrop.push_back(results[i]);
      Serial.print(results[i] + " ");
    }
  }

  results.clear();

  for (String pill : pillsToDrop)
  {
    int id = GetServoIdByPill(pill);
    MoveServo(id);
  }
}
