#include <SoftwareSerial.h>
#include <Wire.h>
#define gsm Serial1



//Указываем пины soft UART
SoftwareSerial gsm(3, 4); // RX, TX
//Устанавливаем i2c адресс дисплея

void setup() {
Serial.begin(19200);
  gsm.begin(19200);

}

void loop() {
     gsm.println("AT+CPAS");
     delay(500);

}
