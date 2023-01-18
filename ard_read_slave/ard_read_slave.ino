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

bool light;
bool dark;

int lightCounter;
int darkCounter;
volatile bool motorState;

char incomingMsg[100];
String sendMessage;
String msg;

byte sendingIndex;
byte incomingIndex;
volatile bool receive;
volatile bool sending;
volatile bool done;
volatile bool remoteMotor;
volatile bool sendingMotor;

bool sending_on = false;

void setup() {
  Serial.begin (9600);
  Serial.println("Arduino start");
  lcd.init();
  lcd.backlight();

  motor.setSpeed(60);

  SPCR |= bit(SPE);         //bekapcsolja az SPIt
  //SPI.begin();
  pinMode(MISO, OUTPUT);    //MISOn valaszol

  incomingIndex = 0;
  sendingIndex = 0;
  sendMessage = "";
  msg = "";
  remoteMotor = false;

  receive = true;
  sending = false;
  done = false;

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
  sendingMotor = false;
}

void loop() {

  startMotor();
  if (done)
  {
    Serial.print("Motor from web: " );
    for (int i = 0; i < incomingIndex; i++)
      Serial.print(incomingMsg[i]);
    if (incomingMsg[0] == '1') {
      remoteMotor = true;
    } else {
      remoteMotor = false;
    }
    Serial.println();
    incomingIndex = 0;
    done = false;
    receive = true;
    sending = false;
  }
  collectData();

}


void collectData()
{
  msg = "";
  msg = '\0';
  lcd.setCursor(0, 0);
  displayTime();

  lcd.setCursor(0, 1);
  displayTempHumidity();

  lcd.setCursor(0, 2);
  displayBrightness();
  msg += '\n';

  if (!sending) {
    sendMessage = "";
    sendMessage = msg;
    Serial.println(sendMessage.length());
    Serial.println(sendMessage);
  }
  delay(1000);

}

// ----------- Displaying functions
void displayTime()
{
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
    if (!sending_on) {
      msg = "\0";
    }
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
}

void displayMotor(int brightness)
{
  lcd.setCursor(0, 2);
  if (brightness > 10)
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

  if (light && dark)
  {
    light = false;
    dark = false;
    startMotor();
  }
  else
  {
    motorState = false;
    lcd.setCursor(0, 3);
    lcd.print("Motor is not running");
  }
  appendString(String(int(motorState)));
  sendingMotor = false;
}


// SPI interrupt routine
ISR(SPI_STC_vect)
{
  char c = SPDR;

  if (receive)
  {
    incomingMsg[incomingIndex] = c;
    incomingIndex++;
  }

  if (sending)
  {
    if (sendingIndex < sendMessage.length() - 1) {
      SPDR = sendMessage[sendingIndex];
    }
    if (sendingIndex == sendMessage.length()) {
      done = true;
      receive = true;
    }
    sendingIndex++;
  }

  if (c == '\n')
  {
    receive = false;
    sending = true;
    sendingIndex = 0;
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
  int len = text.length();
  for (int i = 0; i < len; i++) {
    msg += text[i];
  }
  msg += ';';
}

void startMotor()
{
  lcd.setCursor(0, 3);
  if (motorState || remoteMotor)
  {
    lcd.setCursor(0, 3);
    lcd.print("Motor is running    ");
    motor.step(steps_per_rev);
    motor.step(steps_per_rev);
    motor.step(steps_per_rev);
    motor.step(steps_per_rev);
  }
  lcd.print("Motor is not running");
}
