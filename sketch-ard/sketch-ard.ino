#include <Wire.h>               // библиотека для управления устройствами по I2C

#include "general-code.hpp"

extern LiquidCrystal_I2C lcd;  // присваиваем имя LCD для дисплея

int find_lcd_id() {
  Serial.println("I2C scanner. Scanning ...");
  byte count = 0;
  Wire.begin();
  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found address: ");
      Serial.print(i, DEC);
      Serial.print(" (0x");
      Serial.print(i, HEX);
      Serial.println(")");
      count++;
      delay(1);  // maybe unneeded?
    }              // end of good response
  }                  // end of for loop
  Serial.println("Done.");
  Serial.print("Found ");
  Serial.print(count, DEC);
  Serial.println(" device(s).");
}

void wait_state() {
  lcd.clear();
  lcd.print("Hello, badcast!");
  lcd.setCursor(0, 1);
  lcd.print("^_^ - ready go!");
}

int content = 0;
int timeout = 0;
void reset(bool printing) {
  digitalWrite(LED_BUILTIN, LOW);
  content = 0;
#ifdef USE_PRINT
  if (!printing) return;
  lcd.clear();
  lcd.print("RESETING...");
  delay(1000);
  wait_state();
#endif
}

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;
  pinMode(LED_BUILTIN, OUTPUT);

  reset(false);

  lcd.init();       // инициализация LCD дисплея
  lcd.backlight();  // включение подсветки дисплея
  lcd.display();

  wait_state();
  // init complete and send to awaiting
  send(&default_attribute, 1);  // send attrib init
}

void serialEvent() {
  if (!content || Serial.available() != content) return;

  timeout = 0;
#ifdef USE_PRINT
  lcd.clear();

#endif
  void* requestData = malloc(content);

  // read command
  if (read_timeout(requestData, content) != content) {
    free(requestData);
    reset(true);
    return;
  }
#ifdef USE_PRINT
  lcd.setCursor(0, 1);
  lcd.print("echo command");
  delay(1000);
#endif

  // echo
  if (!echo_test()) {
    reset(true);
    return;
  }
#ifdef USE_PRINT
  lcd.clear();
  lcd.print("CMD: ");
#endif

  content = 0;

  arduino_reply(reinterpret_cast<command_request*>(requestData));
  free(requestData);
#ifdef USE_PRINT
  wait_state();
#endif
}


void loop() {
  if (content == 0) {
    digitalWrite(LED_BUILTIN, LOW);
    content = Serial.read();
    switch (content) {
      case cmd_reset:
        reset(true);
        send(cmd_echo, 1);
      case -1:
        content = 0;
        return;
    }
#ifdef USE_PRINT
    lcd.clear();
    lcd.print("check verify");
#endif
    // echo and wait initialize
    // check verify
    if (!echo_test()) {
      reset(true);
      return;
    }
    digitalWrite(LED_BUILTIN, HIGH);
#ifdef USE_PRINT
    lcd.clear();
    lcd.print("verifyed");
#endif
    timeout = millis() + 5000;
  } else if (timeout && timeout < millis()) {
    timeout = 0;
    // clear unordered resources
    while (Serial.available() > 0) Serial.read();
    reset(true);
  }
}
