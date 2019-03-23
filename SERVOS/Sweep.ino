
#include <Servo.h>

Servo servo1;  
Servo servo2;
Servo servo3;
Servo servo4;

void setup() {
servo1.attach(2); 
servo2.attach(3);
servo3.attach(4); 
servo4.attach(5);  
}

void loop() {
  int pos1;
  int pos2;
  int pos3;
  int pos4;

  for (pos1 = 0; pos1 <= 90; pos1 += 1) { 
    // in steps of 1 degree
    servo1.write(pos1);              
    delay(15);                       
  }
  for (pos1 = 90; pos1 >= 0; pos1 -= 1) { 
    servo1.write(pos1);              
    delay(15);                       
  }
  for (pos2 = 0; pos2 <= 70; pos2 += 1) { 
    // in steps of 1 degree
    servo2.write(pos2);              
    delay(15);                       
  }
  for (pos2 = 70; pos2 >= 0; pos2 -= 1) { 
    servo2.write(pos2);              
    delay(15);                       
  }
  for (pos3 = 0; pos3 <= 90; pos3 += 1) { 
    // in steps of 1 degree
    servo3.write(pos3);              
    delay(15);                       
  }
  for (pos3 = 90; pos3 >= 0; pos3 -= 1) { 
    servo3.write(pos3);              
    delay(15);                       
  }
  for (pos4 = 0; pos4 <= 70; pos4 += 1) { 
    // in steps of 1 degree
    servo4.write(pos4);              
    delay(15);                       
  }
  for (pos4 = 70; pos4 >= 0; pos4 -= 1) { 
    servo4.write(pos4);              
    delay(15);                       
  }
}
