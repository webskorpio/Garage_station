#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
//#include <iarduino_RTC.h>  
#include <DHT.h>
#include <Wire.h>

#define STATLED1 6                                // LED статус ошибки чтения датчика на 1-м этаже
#define STATLED2 5                                // LED статус ошибки чтения датчика в подвале
#define DHTPIN1 7                                 // DHT22-1
#define DHTPIN2 8                                 // DHT22-2
#define DHTPIN3 9                                 // DHT22-3
#define ACDC 10                                   // наличие 220
#define LEDGPRS 11                                // Led
#define LED220 12                                 // LED состояния 220
#define gsm Serial1                               // Изменение имени Software Serial на более убобный

// Переменные таймеров--------------------------------------------------------------------------------

unsigned long connectTime, newConnectTime;        // Проверка состояния соединения с Internet
unsigned long currentTime, loopTime;              // Интервал отправки данных
unsigned long lcdTime, newLcdTime;                // Интервал обновления данных на дисплее

//Переменные------------------------------------------------------------------------------------------

float h1, t1, h2, t2, t3;                         // Температура и влажность DHT-22
float readh1, readt1, readh2, readt2, readt3; 

bool ac;                                          // Состояние 220В
bool gprsIp;                                      // Состояние соединения с Internet
int LCD = 0;                                      // Изначальная строка на LCD
int errSensor1 = 0;                               // Переменная состояния датчика 1
int errSensor2 = 0;                               // Переменная состояния датчика 2 
int errSensor3 = 0;                               // Переменная состояния датчика 3
String val;                                       // Переменная с данными для отправки
String ip;                                        // Переменная текущего IP адресса в сети
int connetError = 0;                              // Количество попыток реконекта соединения
boolean stringComplete = false;
  String input;
  String comm;
  String com;

//iarduino_RTC time(RTC_DS1307);                    // Объявляем объект time для работы с RTC модулем на базе чипа DS1307, используется аппаратная шина I2C
SoftwareSerial gsm(3, 4);                         // Указываем пины soft UART (RX, TX) 
LiquidCrystal_I2C lcd(0x27, 16, 2);               // Устанавливаем i2c адресс дисплея

// Инициируем датчики---------------------------------------------------------------------------------

DHT dht1(DHTPIN1, DHT22);
DHT dht2(DHTPIN2, DHT22);
DHT dht3(DHTPIN3, DHT22);

// Старт программы------------------------------------------------------------------------------------

void setup() {
  
  Serial.begin(19200);
  gsm.begin(19200);
  dht1.begin();
  dht2.begin();
  dht3.begin();
  lcd.begin(); lcd.backlight();                           // Иницилизируем дисплей
//  time.begin();                                           // Инициируем DS1307
//  Функция settime(секунды [, минуты [, часы [, день [, месяц [, год [, день недели]]]]]]):
//      записывает время в модуль
//      год указывается без учёта века, в формате 0-99
//      часы указываются в 24-часовом формате, от 0 до 23
//      день недели указывается в виде числа: 0-воскресенье, 1-понедельник, 2-вторник ..., 6-суббота
//      если предыдущий параметр надо оставить без изменений, то можно указать отрицательное или заведомо большее значение
//      пример: time.settime(-1, 10); установит 10 минут, а секунды, часы и дату, оставит без изменений
//      пример: time.settime(0, 5, 13); установит 13 часов, 5 минут, 0 секунд, а дату оставит без изменений
//      пример: time.settime(-1, -1, -1, 9, 2, 17); установит дату 09.02.2017 , а время и день недели оставит без изменений

 // Serial.println(time.gettime("Y/m/d,H:i:s+3"));          // +3 возможно не верно

  //Cчитываем время, прошедшее с момента запуска программы
  connectTime = millis(); newConnectTime = connectTime;   // Проверка состояния соединения
  currentTime = millis(); loopTime = currentTime;         // Период отправки данных
  lcdTime = millis(); newLcdTime = lcdTime;               // Период вывода данных на LCD

  // Настраиваем порты
  pinMode(LED220, OUTPUT);
  pinMode(ACDC, OUTPUT);
  pinMode(LEDGPRS, OUTPUT);
  pinMode(STATLED1, OUTPUT);
  pinMode(STATLED1, OUTPUT);

  // Выставляем уровни на портах при Вкл.
  digitalWrite(LED220, LOW);
  digitalWrite(LEDGPRS, LOW);
  digitalWrite(STATLED1, LOW);
  digitalWrite(STATLED1, LOW);

  ac = digitalRead(ACDC);                                  //Проверяем состояние 220В и ыставляем индикацию зависимости от состояния
  
  if(ac == LOW) {digitalWrite(LED220, HIGH);}else{digitalWrite(LED220, LOW);}

  // Поэтапный запуск системы:

  lcd.clear(); lcd.setCursor(0,0);
  lcd.print("TEST MODEM: ");
    
  // Проверяем готовность модема
  do{
     gsm.println("AT+CPAS");
     Serial.print("!");
     delay(100);
     }while(!gsm.find('0'));
     
  delay(1000);
  lcd.print("OK");
  delay(2000);
     
  // Проверяем регистрацию модема в сети
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
    
  // Выключаем эхо
  gsm.println("ATE0");
  delay(100);
  
  // Соединяемся с Internet
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("INTERNET: ");
  gprsconnect();
  delay(1000);
  lcd.print("OK");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("  START SYSTEM  ");
  sensorRead();
  lcdPrint();
  gprssend();
}



//------------------------------------------------------------------------------------------------------------------
//
//
void loop() {

  // Вывод данных на дисплей раз в 5 сек.
  lcdTime = millis();
  if(lcdTime >= (newLcdTime + 4999) or lcdTime <= (newLcdTime - 1000)){
    sensorRead(); lcdPrint(); newLcdTime = lcdTime;
  }
  // Проверяем состояние соединения с Internet раз в 1 минуту
  connectTime = millis();
  if(connectTime >= (newConnectTime + 60000) or connectTime <= (newConnectTime - 1000)){
   gsm.println("at+xiic?");
   delay(100);
   if (gsm.find("0.0.0.0")){
     gprsconnect();                                                                                   // Если нет, то подключаемся
     delay(2000);
    }else{
        Serial.print("/");
        gprsIp = 0;
      }
    newConnectTime=connectTime;
   }
  // Отправляем данные на narodmon.ru раз в 5 минут. Если есть соединение 
  if(gprsIp != 1){
    currentTime = millis();
    if(currentTime >= (loopTime + 299999) or currentTime <= (loopTime -1000)){
      gprssend();
      loopTime = currentTime;
    }
  }
  // Проверяем Serial на наличие AT команд
  //serialCommad();
}
//
//
//------------------------------------------------------------------------------------------------------------------






// Функция опроса и подготовки данных на отправку-------------------------------------------------------------------
void sensorRead(){

  // Опрос DHT22
  readh1 = dht1.readHumidity(); readt1 = dht1.readTemperature();
  readh2 = dht2.readHumidity(); readt2 = dht2.readTemperature();
  readt3 = dht3.readTemperature();

  // Проверяем датчики на "NAN" вместо данных

    // DHT-22 1-й этаж //
  if (isnan(readt1) || isnan(readh1)) {
    if(errSensor1 == 0){ errSensor1++; }                                                               // Если ошибки до этого не было ставим флаг 1
  }else{
    if(errSensor1 != 0){errSensor1 = 0;}                                                               // Если раньше была ошибка а теперь нет сбрасываем флаг на 0
    h1=readh1; t1=readt1;                                                                              // Обновляем данные в переменных на новые.
  }
    // DHT-22 подвал //
  if (isnan(readt2) || isnan(readh2)) {
    if(errSensor2 == 0){ errSensor2++; }                                                               // Если ошибки до этого не было ставим флаг 1
  }else{
    if(errSensor2 != 0){errSensor2 = 0;}                                                               // Если раньше была ошибка а теперь нет сбрасываем флаг на 0
    h2=readh2; t2=readt2;                                                                              // Обновляем данные в переменных на новые.
  }
    // DHT-22 Улица //
  if (isnan(readt3)) {
    if(errSensor3 == 0){ errSensor3++; }                                                               // Если ошибки до этого не было ставим флаг 1
  }else{
    if(errSensor3 != 0){errSensor3 = 0;}                                                               // Если раньше была ошибка а теперь нет сбрасываем флаг на 0
    t3=readt3;                                                                                         // Обновляем данные в переменных на новые.
  }

    // Проверяем состояние датчиков. Если есть ошибка чтения включаем индикацию
  if(errSensor1 != 0) { digitalWrite(STATLED1, HIGH); }else{ digitalWrite(STATLED1, LOW); }
  if(errSensor2 != 0) { digitalWrite(STATLED2, HIGH); }else{ digitalWrite(STATLED2, LOW); }

  ac = digitalRead(ACDC);                                                                               // Считываем состояние 220В
  if(ac == HIGH) { digitalWrite(LED220, HIGH); }else{ digitalWrite(LED220, LOW); }                      // Выставляем статус светодиода наличия 220В
  if (gprsIp == 0){ digitalWrite(LEDGPRS, LOW); }else{ digitalWrite(LEDGPRS, HIGH); }                   // Выставляем статус светодиода наличия соединения с Internet



}

// Функция вывода на lcd-------------------------------------------------------------------------------------------------
void lcdPrint(){
  if(LCD == 0){
    lcd.clear(); lcd.setCursor(0,0);
    lcd.print("1T");    lcd.print(t1);    lcd.print("C H");    lcd.print(h1);    lcd.print("%");        // Температура и влажность на первом этаже
    lcd.setCursor(0,1);
    lcd.print("2T");    lcd.print(t2);    lcd.print("C H");    lcd.print(h2);    lcd.print("%");        // Температура и влажность в подвале
    LCD++;                                                                                              // Переходим на следующую страницу дисплея
    }else{
      lcd.clear(); lcd.setCursor(0,0);
      lcd.print("3T");      lcd.print(t3);      lcd.print("C    AC:");      lcd.print(!ac);             // Температура на улице и наличие 220V
      lcd.setCursor(0,1);
      lcd.print(ip);                                                                                    // Текущий IP адресс устройства
      LCD = 0;                                                                                          // Сбрасываем на 1 страницу отображения
    }
}

// Функция соединения с интернетом-----------------------------------------------------------------------------------------
void gprsconnect(){

  // dключаем РРР
  gsm.println("AT+XISP=0");
  delay(100);
  //Настраиваем соединение
  gsm.println("AT+CGDCONT=1,\"IP\",\"internet.tele2.ru\"");
  delay(100);
  gsm.println("AT+XGAUTH=1,1,\"\",\"\"");
  delay(100);
  gsm.println("at+xiic=1");
  delay(100);

  // Проверяем выдали ли нам IP
  do{
    gsm.println("at+xiic?");
    if(connetError != 0) { Serial.println("no_ip"); }
    delay(300);
    gprsIp = 1;                                                     //Если нет соединения с Internet гасим диод
    connetError++;
  }while(gsm.find("0.0.0.0") and connetError != 9);

  if(connetError == 9){
    connetError = 0;
    Serial.println("The number of attempts to obtain IP has been exhausted.");
    Serial.println("Reconnecting");
    ip = ipRep();
    }else{
    connetError = 0;
    Serial.println("ok_ip");
    ip = ipRep();
    gprsIp = 0;                                                     // Если соединение установлено зажигаем диод
    }
}

//Функция определения IP адресса----------------------------------------------------------------------------------------------

String ipRep(){
  int idEnd;
  String inputString;
  String result;
  
  gsm.println("at+xiic?");                                // Отправляем запрос в модем
  delay(300);
  while (gsm.available())                                 // Проверка наличия данных в порту
  {
    char inChar = (char)gsm.read();                       // Заполняем буфер
    delay(10);
    inputString += inChar;
    if (inChar == '\n') stringComplete = true;            // Ставим флаг что есть данные
  }
  if(stringComplete == true){
    idEnd = inputString.length() -8 ;                     // Определяем до какого символа считывать из строки
    result = inputString.substring(15,idEnd);             // Записываем с 13-го симво и до idEnd. Это наш IP
    idEnd = 0;                                            // Сбрачываем так как дли ip адреса может измениться при реконекте 
    inputString = "";                                     // Очищаем буфер
    stringComplete = false;                               // Снимаем флаг
  }
  return result;
}

//Функция отправки данных на narodmon.ru---------------------------------------------------------------------------------------

void gprssend(){
  gsm.println("AT+TCPCLOSE=0");                           // Закрываем соединение, на всякий случай
  
  //В цикле соединяемся с сервером народмон
  while(1){
    gsm.flush();
    gsm.println("AT+TCPSETUP=0,94.142.140.101,8283");     // Текущий IP сервера narodmon.ru 94.142.140.101 порт 8283
    delay(2500);
    if (gsm.find("+TCPSETUP:0,OK")) break;                // Если соединились, выходим из цикла
    delay(300);
    Serial.println("tcp_err"); 
    //   if (gsm.find("+TCPSETUP:0,FAIL") or gsm.find("+TCPSETUP:Error 2")) Serial.println("tcp_err");  // Выводим ошибку при отсутствии соединения
                               
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
    //Собираем данные в кучу для отправки
    val = "#9512973831000000#Garage.Station\n#H1DHT22#";  // MAC: 9512973831000000; Name: Garage.Station;
    val = val + h1 + "\n#T1DHT22#"+t1+"\n#H2DHT22#"+h2+"\n#T2DHT22#"+t2+"\n#T3DHT22#"+t3+"\n#S0#"+!ac+"\n#ERR1#"+!errSensor1+"\n#ERR2#"+!errSensor2+"\n#ERR3#"+!errSensor3;
    val = val + "\n##";

  //Отправляем
  int buff = val.length();
  gsm.print("at+tcpsend=0,");
  gsm.println(buff);
  delay(200);
  gsm.println(val);
  delay(250);
  if (gsm.find("+TCPSEND")) Serial.println("sendOK");
  else Serial.println("sendERROR");
  gsm.println("AT+TCPCLOSE=0");                           // Закрываем соединение
}


//Функция чтения команд из Serial port----------------------------------------------------------------------------------------

void serialCommad(){

  
  while (Serial.available())                                  // Проверка наличия данных в порту
  {                                 
   char inChar = (char)Serial.read();                        // Заполняем буфер
    delay(10);
    input += inChar;
    if (inChar == '\n') stringComplete = true;                // Ставим флаг что есть данные
  }
  
  if(stringComplete == true){                                 // Если данные есть проверяем на наличие команд
    comm = input.substring(3,8);
    com = input.substring(0,3);
    if(com == "AT+"){
      if(comm == "CCLK="){ Serial.println("Set time");}
 //     if(comm == "CCLK?"){ Serial.println(time.gettime("y/m/d,H:i:s"));}
      if(comm == "SEND="){ if(gprsIp != 1){ currentTime = millis(); Serial.println("Send date the narodmon.ru"); gprssend(); loopTime = currentTime; }else{Serial.println("No Internet connecting");}}
    }
    
  input = "";                                     // Очищаем буфер
  stringComplete = false;                               // Снимаем флаг
  
  }
  
}

