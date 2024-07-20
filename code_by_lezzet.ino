#include <EEPROM.h>

//esp is working as builtin led's high and low are opposite

// Pin Definitions for NodeMCU
const int TRIG_PIN = 14;            // GPIO14   D5
const int ECHO_PIN = 12;            // GPIO12   D6
const int RELAY_PIN = 4;            // GPIO5  D2
const int SETTINGS_BUTTON_PIN = 5;  // GPIO4  D1


// Constants
const int EMPTY_CHECK_COUNT = 7;
const int NON_EMPTY_CHECK_COUNT = 7;

// EEPROM Addresses
const int EEPROM_MIN_DIST_ADDR = 0;

long duration; // to get duration while measuring distance
int distance; // to store distance
int emptyCount = 0; //To track the duration for which the container remains empty.

int minDist;
int maxDist;

int measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

void saveSettingsToEEPROM() {
  EEPROM.put(EEPROM_MIN_DIST_ADDR, minDist);
  EEPROM.commit();
}

void loadSettingsFromEEPROM() {
  EEPROM.get(EEPROM_MIN_DIST_ADDR, minDist);
}

void initializeSettings() {
  digitalWrite(RELAY_PIN, HIGH);
  delay(500);
  digitalWrite(RELAY_PIN, LOW);
  delay(500);
  digitalWrite(RELAY_PIN, HIGH);
  delay(500);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  int initialDistance = measureDistance();
  int prevDistance = initialDistance;

  int iterationCount = 0;
  while (abs(initialDistance - prevDistance) <= 3) {
    prevDistance = initialDistance;
    iterationCount++;

    if (iterationCount >= 5) {
      minDist = initialDistance - 1;
      saveSettingsToEEPROM();
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(RELAY_PIN, HIGH);
      delay(2500);
      digitalWrite(RELAY_PIN, LOW);
      break;
    }

    delay(300);
    initialDistance = measureDistance();
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SETTINGS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);

  EEPROM.begin(4);  // Initialize EEPROM
  loadSettingsFromEEPROM();

  /*  If the switch is turned off while the ESP is powering up, this code snippet checks 
  whether the switch is turned on and off twice to set up the initial distance measurement.  */
  if (digitalRead(SETTINGS_BUTTON_PIN) == HIGH) { //first switch off
    digitalWrite(LED_BUILTIN, HIGH);
    while (digitalRead(SETTINGS_BUTTON_PIN) == HIGH) {
      //nothing
    }
    if (digitalRead(SETTINGS_BUTTON_PIN) == LOW) { //first switch on
      int j = 0;
      while (digitalRead(SETTINGS_BUTTON_PIN) == LOW && j < 8) {
        j++;
        delay(200);
        //nothing
      }
      if (digitalRead(SETTINGS_BUTTON_PIN) == HIGH) { //second switch off
        while (digitalRead(SETTINGS_BUTTON_PIN) == HIGH) {
          //nothing
        }
        if (digitalRead(SETTINGS_BUTTON_PIN) == LOW) { //second switch on
          initializeSettings(); // set up empty distance
        }
      }
    }
  }

  // if (digitalRead(SETTINGS_BUTTON_PIN) == LOW) {
  //   initializeSettings();
  // }
  maxDist = minDist + 3;
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  
  if (digitalRead(SETTINGS_BUTTON_PIN) == HIGH) { // To halt esp when switch is off
    digitalWrite(LED_BUILTIN, HIGH);
    while(digitalRead(SETTINGS_BUTTON_PIN) == HIGH){}
  }

  distance = measureDistance();
  while (distance == 0) { //to halt when there is a problem with ultrasonic sensor
    if (digitalRead(SETTINGS_BUTTON_PIN) == HIGH) {
      digitalWrite(LED_BUILTIN, HIGH);
      while(digitalRead(SETTINGS_BUTTON_PIN) == HIGH){}
    }
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
    delay(500);
    distance = measureDistance();
  }

  bool isEmpty = (distance > minDist && distance < maxDist);
  bool isNonEmpty = (distance < minDist);

  if (isEmpty) {
    emptyCount++;
  } else if (isNonEmpty) {
    emptyCount = max(emptyCount - 5, 0);
  }

  if (emptyCount > EMPTY_CHECK_COUNT) {
    digitalWrite(LED_BUILTIN, HIGH);
    emptyCount = 0;
    for (int i = 0; i < 10; i++) {
      distance = measureDistance();
      if (distance < 4) {
        i++;
        continue;
      }
      digitalWrite(RELAY_PIN, HIGH);
      delay(400);
      digitalWrite(RELAY_PIN, LOW);
      delay(400);
    }
    int j=1;
    while (true) { // To halt esp when switch is off
      if (digitalRead(SETTINGS_BUTTON_PIN) == HIGH) {
        digitalWrite(LED_BUILTIN, HIGH);
        while(digitalRead(SETTINGS_BUTTON_PIN) == HIGH){}
      }
      distance = measureDistance();
      isNonEmpty = (distance < minDist);

      if (isNonEmpty) {
        emptyCount++;
      } else {
        emptyCount = 0;
      }

      if (emptyCount > NON_EMPTY_CHECK_COUNT) {
        break;
      }

      //to control led pattern on built in led
      if (j%4 == 0) {
        digitalWrite(LED_BUILTIN, LOW);
        j = 1 ;
        delay(1000);
      }
      else{
        digitalWrite(LED_BUILTIN, HIGH);
        j++;
        delay(1000);
      }
    }
    digitalWrite(LED_BUILTIN, LOW);
    emptyCount = 0;
  }
}
