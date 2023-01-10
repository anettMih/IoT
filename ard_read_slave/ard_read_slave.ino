// Real time clock to get timestamp/ definitions
#include "RTClib.h"
RTC_DS3231 rtc;

// Libreries for geting the brighness/ definitions
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// Librarie for getting temperature and humidity/ definitions
#include <SimpleDHT.h>
int pinDHT11 = A3;
SimpleDHT11 dht11(pinDHT11);

// Library for LCD
#include "LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Library for motor controlling
#include <Stepper.h>
#define IN1 8
#define IN2 6
#define IN3 5
#define IN4 4
const int steps_per_rev = 2048;
Stepper motor(steps_per_rev, IN1, IN3, IN2, IN4);

// Library for multi-threading
#include <Thread.h>
#include <ThreadRunOnce.h>
#include <ThreadController.h>
Thread myThread = Thread();
ThreadController controll = ThreadController();

bool light;
bool dark;

int lightCounter;
int darkCounter;
bool motorState;

// Row 1, Col 17
byte moonChart1[8] = {
  B00000,
  B00001,
  B00001,
  B00111,
  B00111,
  B01111,
  B01111,
  B01111,
};
// Row 1, Col 18
byte moonChart2[8] = {
  B00000,
  B11110,
  B11100,
  B11000,
  B10000,
  B10000,
  B10000,
  B10000,
};
// Row 2, Col 17
byte moonChart3[8] = {
  B01111,
  B01111,
  B01111,
  B00111,
  B00111,
  B00001,
  B00001,
  B00000,
};
// Row 2, Col 18
byte moonChart4[8] = {
  B10000,
  B10000,
  B10000,
  B10000,
  B11000,
  B11100,
  B11110,
  B00000,
};

// Row 1, Col 17
byte sunChart1[8] = {
  B00000,
  B00001,
  B01001,
  B00101,
  B10011,
  B01001,
  B00111,
  B11111,
};
// Row 1, Col 18
byte sunChart2[8] = {
  B00000,
  B00000,
  B00010,
  B00100,
  B01001,
  B10010,
  B11100,
  B11110,
};
// Row 2, Col 17
byte sunChart3[8] = {
  B01111,
  B10111,
  B01001,
  B10010,
  B00100,
  B01000,
  B00000,
  B00000,
};
// Row 2, Col 18
byte sunChart4[8] = {
  B11110,
  B11101,
  B10010,
  B11001,
  B10100,
  B10000,
  B00000,
  B00000,
};

#include <SPI.h>

SPISettings spi_settings(100000, MSBFIRST, SPI_MODE0);

char bejovo_uzenet[100];
String sendMessage;

volatile byte message_len;
volatile byte index_bejovo;
volatile byte index_kimeno;
volatile bool fogadunk;
volatile bool kuldunk;
volatile bool vege;
volatile bool allRead;
volatile bool remoteMotor;

void setup() {
  Serial.begin (9600);
  Serial.println("Arduino start");
  lcd.init();
  lcd.backlight();

  motor.setSpeed(60);

  SPCR |= bit(SPE);         //bekapcsolja az SPIt
  //SPI.begin();
  pinMode(MISO, OUTPUT);    //MISOn valaszol

  index_bejovo = 0;
  index_kimeno = 0;
  message_len = 0;
  sendMessage = "";
  remoteMotor = false;

  fogadunk = true; //elobb fogadunk
  kuldunk = false; //aztan kuldunk
  vege = false; //aztan vege
  allRead = false;

  SPI.attachInterrupt();   //ha jon az SPIn  valami beugrik a fuggvenybe

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1)
      delay(10);
  }
  if (rtc.lostPower())
  {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!tsl.begin())
  {
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while (1)
      ;
  }
  configureLightSensor();

  lightCounter = 0;
  darkCounter = 0;
  light = false;
  dark = false;
  motorState = false;
  myThread.onRun(startMotor);
  controll.add(&myThread);
  
}

void loop() {
  controll.run();
  if (vege)
  {
    Serial.println("END:");
    for (int i = 0; i < index_bejovo; i++)
      Serial.print(bejovo_uzenet[i]);
    remoteMotor = bejovo_uzenet[0] == '1' ? true : false;
    index_bejovo = 0;
    vege = false;
    fogadunk = true;
    kuldunk = false;
  }
  collectData();


}

void collectData()
{
  Serial.println("=================================Collect");
  sendMessage = "";
  sendMessage += '\0';
  lcd.setCursor(0, 0);
  displayTime();

  lcd.setCursor(0, 1);
  displayTempHumidity();

  lcd.setCursor(0, 2);
  displayBrightness();

  sendMessage[message_len] = '\n';
  allRead = true;
  Serial.println(sendMessage);
  delay(1000);
}

// ----------- Displaying functions
void displayTime()
{
  message_len = 0;
  DateTime time = rtc.now();
  String timeMessage = time.timestamp(DateTime::TIMESTAMP_FULL);
  appendString(timeMessage);
  lcd.print(timeMessage);
}

void displayTempHumidity()
{
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
  {
    Serial.print("Read DHT11 failed, err=");
    Serial.println(err);
    sendMessage = "";
    return;
  }

  String temp = String((int)temperature);
  appendString(temp);

  String humy = String((int)humidity);
  appendString(humy);

  String message = "Temp " + temp + "C Hum " + humy + "%";
  lcd.print(message);
}


void displayBrightness()
{
  sensors_event_t event;
  tsl.getEvent(&event);
  String bright = "Light " + String(int(event.light)) + " lux  ";
  appendString(String(int(event.light)));

  lcd.print(bright);

  displayMotor(int(event.light));
  Serial.println(motorState);
  appendString(String(int(motorState)));
}

void displayMotor(int brightness)
{
  lcd.setCursor(0, 2);
  if (brightness)
  {
    lightCounter++;
    if (lightCounter >= 10)
    {
      light = true;
    }
    showSunChart();
    darkCounter = 0;
  }
  else
  {
    darkCounter++;
    if (light && darkCounter >= 2)
    {
      dark = true;
    }
    showMoonChart();
    lightCounter = 0;
  }

  lcd.setCursor(0, 3);

  if (light && dark)
  {
    lcd.print("Motor is running    ");
    light = false;
    dark = false;
    motorState = true;
  }
  else
  {
    lcd.print("Motor is not running");
    motorState = false;
  }

}


// SPI interrupt routine
ISR(SPI_STC_vect)
{
  //  Serial.println("SPI interrupt routine");

  //kiolvassuk a kapott karaktert
  char c = SPDR;

  //SPDR = 'k';

  if (fogadunk)
  {
    bejovo_uzenet[index_bejovo] = c;
    index_bejovo++;
  }

  //  if (allRead) {
  //    Serial.println(sendMessage);
  //  }

  if (kuldunk && allRead)
  {

    if (index_kimeno < sendMessage.length()) {
      SPDR = sendMessage[index_kimeno];
    }
    if (index_kimeno == 99) {
      vege = true;

      Serial.println("END true");
      allRead = false;
    }
    index_kimeno++;
  }

  if (c == '\n')
  {
    Serial.println("Newline");
    fogadunk = false;
    kuldunk = true;
    index_kimeno = 0;
  }
}


// ----------- Helper functions
void configureLightSensor()
{
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
}

void showMoonChart()
{
  lcd.createChar(1, moonChart1);
  lcd.createChar(2, moonChart2);
  lcd.createChar(3, moonChart3);
  lcd.createChar(4, moonChart4);

  lcd.setCursor(17, 1);
  lcd.write(1);
  lcd.setCursor(18, 1);
  lcd.write(2);
  lcd.setCursor(17, 2);
  lcd.write(3);
  lcd.setCursor(18, 2);
  lcd.write(4);
}

void showSunChart()
{
  lcd.createChar(1, sunChart1);
  lcd.createChar(2, sunChart2);
  lcd.createChar(3, sunChart3);
  lcd.createChar(4, sunChart4);

  lcd.setCursor(17, 1);
  lcd.write(1);
  lcd.setCursor(18, 1);
  lcd.write(2);
  lcd.setCursor(17, 2);
  lcd.write(3);
  lcd.setCursor(18, 2);
  lcd.write(4);
}

void appendString(String text)
{
  //  Serial.print(text);
  //  Serial.print(" : ");
  //  Serial.println(text.length());
  int len = text.length();
  for (int i = 0; i < len; i++) {
    sendMessage += text[i];
  }
  sendMessage += ';';
}

void startMotor()
{
  if (motorState || remoteMotor)
  {
    motor.step(steps_per_rev);
    motor.step(steps_per_rev);
    motorState = false;
  }

}
