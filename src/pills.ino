int GetServoIdByPill(String pillName)
{
  int                               id = -1;
  if (pillName == "TikTak")         id = 2;
  else if (pillName == "Nut")       id = 3;
  else if (pillName == "Cocaine")   id = 4;
  else if (pillName == "other")     id = 5;
  
  return id;
}

void MoveServo(int servoId, unsigned _start, unsigned _end) {
  if (servoId < 0) return;
  Servo myServo;
  myServo.attach(servoId);
  const unsigned PAUSE = 13;
  for (int i = 0; i <= _end; i++) {
    myServo.write(i);
    delay(PAUSE);
  }

  for (int i = _end; i >= _start; i--) {
    myServo.write(i);
    delay(PAUSE);
  }

  Serial.printf("Servo %d has moved succesfully :)", servoId - 2);
}
