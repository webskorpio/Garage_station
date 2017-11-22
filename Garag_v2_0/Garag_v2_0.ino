#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Wire.h>

#define STATLED1 6    // номер пина, LED статус ошибки чтения датчика на 1-м этаже
#define STATLED2 5    // номер пина, LED статус ошибки чтения датчика в подвале
#define DHTPIN1 7     // номер пина, DHT22-1
#define DHTPIN2 8     // номер пина, DHT22-2
#define DHTPIN3 9     // номер пина, DHT22-3
#define ACDC 10       // номер пина, наличие 220
#define LEDGPRS 11    // номер пина, Led
#define LED220 12     // номер пина, LED состояния 220


// Переменные таймеров

// Проверка состояния соединения с Internet
unsigned long connectTime, newConnectTime;

// Интервал отправки данных
unsigned long currentTime, loopTime;

// Интервал обновления данных на дисплее
unsigned long lcdTime, newLcdTime;

//Переменные для 3-х DHT-22
float h1, t1, h2, t2, t3;     //Температура и влажность DHT-22
float readh1, readt1, readh2, readt2, readt3; 

bool ac;      // Состояние 220В
bool gprsIp;  // Состояние соединения с Internet
int LCD = 0;  // Изначальная строка на LCD
String val;   // Переменная с данными для отправки
String ip;    // Переменная текущего IP адресса в сети
#define gsm Serial1



//Указываем пины soft UART
SoftwareSerial gsm(3, 4); // RX, TX
//Устанавливаем i2c адресс дисплея
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Инициируем датчики
DHT dht1(DHTPIN1, DHT22);
DHT dht2(DHTPIN2, DHT22);
DHT dht3(DHTPIN3, DHT22);

// Переменная состояния датчиков
int errSensor1 = 0; int errSensor2 = 0; int errSensor3 = 0;

void setup() {
Serial.begin(19200);
  gsm.begin(19200);
  dht1.begin();
  dht2.begin();
  dht3.begin();


  //Cчитываем время, прошедшее с момента запуска программы таймеров
  //Проверка состояния соединения
  connectTime = millis();
  newConnectTime = connectTime;
  //Период отправки данных
  currentTime = millis();
  loopTime = currentTime;
  //Период вывода данных на LCD
  lcdTime = millis();
  newLcdTime = lcdTime;

  //Настраиваем пины
  pinMode(LED220, OUTPUT);
  pinMode(ACDC, OUTPUT);
  pinMode(LEDGPRS, OUTPUT);
  pinMode(STATLED1, OUTPUT);
  pinMode(STATLED1, OUTPUT);

  //Выставляем уровни на пинах при Вкл.
  digitalWrite(ACDC, HIGH);
  digitalWrite(LED220, LOW);
  digitalWrite(LEDGPRS, LOW);
  digitalWrite(STATLED1, LOW);
  digitalWrite(STATLED1, LOW);

  //Иницилизируем дисплей
  lcd.begin();
  lcd.backlight();

  //Проверяем состояние 220В
  ac = digitalRead(ACDC);

  //Выставляем индикацию 220В в зависимости от состояния
  if(ac == LOW) {digitalWrite(LED220, HIGH);}else{digitalWrite(LED220, LOW);}

  //Считываем показания сенсоров и выводим их на дисплей

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("TEST MODEM: ");
  //Проверяем готовность модема
  do{
     gsm.println("AT+CPAS");
     Serial.print(".");
     delay(100);
     }while(!gsm.find("0"));
     delay(1000);
     lcd.print("OK");
     delay(2000);
  //Проверяем регистрацию модема в сети
    lcd.setCursor(0,1);
    lcd.print("REG. THE NET: ");
  do{
     gsm.println("AT+CREG?");
     Serial.print(":");
     delay(100);
      }while(!gsm.find("+CREG: 0,1"));
    delay(1000);
    lcd.print("OK");
    delay(2000);
  //Выключаем эхо
  gsm.println("ATE0");
  delay(100);
  //ХЗ что за функция
  gsm.flush();

  //Соединяемся с Internet
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("INTERNET: ");
  gprsconnect();
  delay(1000);
    lcd.print("OK");
    delay(1000);
    lcd.setCursor(0,1);
    lcd.print("  START SYSTEM  ");
    ipRep(); // определяем наш IP
  sensorRead();
  lcdPrint();
}

void loop() {

  //Вывод данных на дисплей раз в 5 сек.
  lcdTime = millis();
  if(lcdTime >= (newLcdTime + 4999) or lcdTime <= (newLcdTime - 1000)){
    sensorRead();
    lcdPrint();
    newLcdTime = lcdTime;
  }

  //Проверяем состояние соединения с Internet раз в 1 минуту
  connectTime = millis();
  if(connectTime >= (newConnectTime + 60000) or connectTime <= (newConnectTime - 1000)){
   gsm.println("at+xiic?");
   delay(100);
   if (gsm.find("0.0.0.0")){

     // если нет, то подключаемся
     gprsconnect();
     ipRep();
     delay(2000);
    }else{
        Serial.print("/");
        gprsIp = 1;
      }
    newConnectTime=connectTime;
   }

  //Отправляем данные на narodmon.ru раз в 5 минут.
  currentTime = millis();
  if(currentTime >= (loopTime + 299999) or currentTime <= (loopTime -1000)){
    gprssend();
    loopTime = currentTime;
  }
}

//Функция опроса и подготовки данных на отправку
void sensorRead(){

  //Опрос DHT22
  readh1 = dht1.readHumidity(); readt1 = dht1.readTemperature();
  readh2 = dht2.readHumidity(); readt2 = dht2.readTemperature();
  readt3 = dht3.readTemperature();

  //Проверяем датчики на "NAN" вместо данных

  //DHT-22 1-й этаж
  if (isnan(readt1) || isnan(readh1)) {
    //Если ошибки до этого не было ставим флаг 1
    if(errSensor1 == 0){ errSensor1++; }

  }else{
    //Если раньше была ошибка а теперь нет сбрасываем флаг на 0
    if(errSensor1 != 0){errSensor1 = 0;}
    //Обновляем данные в переменных на новые.
    h1=readh1;
    t1=readt1;
  }

  //DHT-22 подвал
  if (isnan(readt2) || isnan(readh2)) {
    //Если ошибки до этого не было ставим флаг 1
    if(errSensor2 == 0){ errSensor2++; }

  }else{
    //Если раньше была ошибка а теперь нет сбрасываем флаг на 0
    if(errSensor2 != 0){errSensor2 = 0;}
    //Обновляем данные в переменных на новые.
    h2=readh2;
    t2=readt2;
  }

    //DHT-22 Улица
  if (isnan(readt3)) {
    //Если ошибки до этого не было ставим флаг 1
    if(errSensor3 == 0){ errSensor3++; }

  }else{
    //Если раньше была ошибка а теперь нет сбрасываем флаг на 0
    if(errSensor3 != 0){errSensor3 = 0;}
    //Обновляем данные в переменных на новые.
    t3=readt3;
  }

    //Проверяем состояние датчиков. Если есть ошибка чтения включаем индикацию
    if(errSensor1 != 0) { digitalWrite(STATLED1, HIGH); }else{ digitalWrite(STATLED1, LOW); }
    if(errSensor2 != 0) { digitalWrite(STATLED2, HIGH); }else{ digitalWrite(STATLED2, LOW); }


  //Считываем состояние 220В
  ac = digitalRead(ACDC);

  //Выставляем статус светодиода наличия 220В
  if(ac == HIGH) { digitalWrite(LED220, HIGH); }else{ digitalWrite(LED220, LOW); }

  //Выставляем статус светодиода наличия соединения с Internet
  if (gprsIp == 0){ digitalWrite(LEDGPRS, LOW); }else{ digitalWrite(LEDGPRS, HIGH); }

    //Собираем данные в кучу для отправки
    val = "#9512973831000000#Garage.Station\n#H1DHT22#";
    val = val + h1 + "\n#T1DHT22#"+t1+"\n#H2DHT22#"+h2+"\n#T2DHT22#"+t2+"\n#T3DHT22#"+t3+"\n#S0#"+!ac+"\n#ERR1#"+!errSensor1+"\n#ERR2#"+!errSensor2+"\n#ERR3#"+!errSensor3;
    val = val + "\n##";

}

//Функция вывода на lcd
void lcdPrint(){
  if(LCD == 0){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("1T");
    lcd.print(t1);
    lcd.print("C H");
    lcd.print(h1);
    lcd.print("%");
    lcd.setCursor(0,1);
    lcd.print("2T");
    lcd.print(t2);
    lcd.print("C H");
    lcd.print(h2);
    lcd.print("%");

    //Переходим на следующую страницу дисплея
    LCD++;
    }else{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("3T");
      lcd.print(t3);
      lcd.print("C    AC:");
      lcd.print(!ac);
      lcd.setCursor(0,1);
      lcd.print(ip);
      //Сбрасываем на 1 страницу отображения
      LCD = 0;
    }
}

//Функция соединения с интернетом
void gprsconnect(){

  // включаем РРР
  gsm.println("AT+XISP=0");
  delay(100);
  //Настраиваем соединение
  gsm.println("AT+CGDCONT=1,\"IP\",\"internet.tele2.ru\"");
  delay(100);
  gsm.println("AT+XGAUTH=1,1,\"\",\"\"");
  delay(100);
  gsm.println("at+xiic=1");
  delay(100);

  //Проверяем выдали ли нам IP
  do{
    gsm.println("at+xiic?");
    Serial.print(".");
    delay(300);

    //Если нет соединения с Internet гасим диод
    gprsIp = 0;
  }while(gsm.find("0.0.0.0"));

  //Если соединение установлено зажигаем диод
  gprsIp = 1;

}
//Функция определения IP адресса.
String ipRep(){
  
  String inputString;
  boolean stringComplete = false;
  
  gsm.println("at+xiic?"); // Отправляем запрос в модем
  while (gsm.available()) // если модем ответил
  {
    char inChar = (char)gsm.read(); //заполняем буфер
    inputString += inChar;
    if (inChar == '\n') stringComplete = true; // ставим флаг что есть данные
  }
  if(stringComplete == true){
    ip = inputString.substring(12); //Записываем с 13-го симво и до конца строки. Это наш IP
    inputString = "";       // очищаем буфер
    stringComplete = false; // снимаем флаг
  }
  return ip;
}

//Функция отправки данных на narodmon.ru
void gprssend(){

  //Закрываем соединение, на всякий случай
  gsm.println("AT+TCPCLOSE=0");

  //В цикле соединяемся с сервером народмон
  while(1){
    gsm.println("AT+TCPSETUP=0,94.142.140.101,8283");
    delay(2500);

    //Если соединились, выходим из цикла
    if (gsm.find("+TCPSETUP:0,OK")) break;
    Serial.println("tcp_err");

    //Если нет, проверяем соединины ли с интернетом
    gsm.flush();
    gsm.println("at+xiic?");
    delay(100);
    if (gsm.find("0.0.0.0")){

      //Если нет, то подключаемся
      gprsconnect();
      delay(2000);
    }
  }

  //Отправляем
  int buff = val.length();
  gsm.print("at+tcpsend=0,");
  gsm.println(buff);
  delay(100);
  gsm.println(val);
  delay(250);
  if (gsm.find("+TCPSEND")) Serial.println("sendOK");
  else Serial.println("sendERROR");

  //Закрываем соединение
  gsm.println("AT+TCPCLOSE=0");
}
