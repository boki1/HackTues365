int GetServoIdByPill(String pillName)
{
  int                              id = -1;
  if (pillName == "Nut0")          id = 4;
  else if (pillName == "Nut1")     id = 13;
  else if (pillName == "TikTak")   id = 14;
  else if (pillName == "Nut2")     id = 27;
  
  return id;
}

void MoveServo(int servoId) {
  if (servoId < 0) return;
  Servo myServo;
  myServo.attach(servoId);
  const unsigned PAUSE = 13;
  const int END = (servoId == 4 || servoId == 14) ? END_ODD : END_EVEN;
  for (int i = START; i <= END; i++) {
    myServo.write(i);
    delay(PAUSE);
  }

  for (int i = END; i >= START; i--) {
    myServo.write(i);
    delay(PAUSE);
  }

  Serial.printf("Servo %d has moved succesfully :)\n", servoId);
}
