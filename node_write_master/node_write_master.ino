#include <Helper.h>
#include <ESP8266WiFi.h>

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert Firebase project API Key
#define API_KEY "AIzaSyB6NuA_1hEXGd2OSqmbfIicu0Q7A5vpf2k"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://iot23-89364-default-rtdb.europe-west1.firebasedatabase.app/"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

#include <SPI.h>

char buff[] = "S\n";
char kuldes_tesztje[20];
char returnbuff[32];

SPISettings spi_settings(100000, MSBFIRST, SPI_MODE0);
// 100 kHz legyen a sebesseg, a Node tud 80MHzt de az Arduino csak 16MHzt
volatile bool motorState;

// Wi-Fi settings
//const char *ssid = "DIGI-tXz2";
//const char *password = "5yb4wg7DF5";

// Wi-Fi settings
const char* ssid = "DIGI-D69p";
const char* password = "8wjBc9cg";

// Wi-Fi settings - uny
// const char* ssid = "Neumann";
// const char* password = "neumann1";

// Values received from payload
String brightnessValue = "";
String temperatureValue = "";
String humidityValue = "";
String timeValue = "";
String motorValue = "";

// Firebase related
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

WiFiServer server(80);

void setup()
{

  Serial.begin(9600);
  motorState = false;
  Serial.println("Node start");

  SPI.begin();
  delay(10);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  Serial.println("API key set");

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  Serial.println("DB url set");
  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop()
{

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client)
  {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available())
  {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  if (request.indexOf("/MOTOR=ON") != -1)
  {
    motorState = true;
  }

  if (request.indexOf("/MOTOR=OFF") != -1)
  {
    motorState = false;
  }
  Serial.println(request);
  SPI.beginTransaction(spi_settings);
  SPI.transfer(motorState ? '1' : '0');
  delay(100);
  SPI.transfer('\n');
  delay(100);

  for (int i = 0; i < 33; i++)
  {
    returnbuff[i] = SPI.transfer('.');
    delay(100);
  }
  SPI.endTransaction();

  Serial.print("Reply: ");
  Serial.println(sizeof(returnbuff));
  for (int i = 1; i < 32; i++)
  {
    Serial.print(returnbuff[i]);
    //    returnbuff[i] = 0;
  }
  Serial.println();
  handleSignals();

  Serial.println();
  String motor = motorState ? "ON" : "OFF";
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  d
  client.println("<!DOCTYPE HTML>");
  client.println("<html><head><meta http-equiv=\"refresh\" content=\"2\"><style>");
  client.println("body {font-weight: bold; color: white; background-color: #181123;} .title { display: flex; justify-content: center;} label {font-size: 30px; } ");
  client.println(".info { display: flex; margin-bottom: 18px;} span {align-self: center; padding-left: 25px;font-size: 18px;}");
  client.println("button { width: 80px; height: 80px; border-radius: 40px; margin-right: 20px;  font-weight: bold; background: linear-gradient(90deg, rgba(2, 0, 36, 1) 0%, rgba(76, 9, 121, 0.7071779395351891) 27%, rgba(0, 212, 255, 1) 100%);}");
  client.println("button:hover {background-color: lightblue;cursor: pointer;}  .buttons{display: flex;justify-content: center;}");
  client.println("</style></head>");
  client.println("<body>");
  client.println("<div class=\"title\"><label>Your last collected data set</label></div><br>");
  client.println("<div class=\"info\"><img src=\"https://cdn-icons-png.flaticon.com/512/9321/9321085.png\" width=\"60\" height=\"60\"><span> " + timeValue + " </span><br><br></div>");
  client.println("<div class=\"info\"><img src=\"https://cdn-icons-png.flaticon.com/512/1445/1445329.png\" width=\"60\" height=\"60\"><span>" + brightnessValue + " lux</span><br><br></div>");
  client.println("<div class=\"info\"><img src=\"https://cdn-icons-png.flaticon.com/512/1163/1163699.png\" width=\"60\" height=\"60\"><span>" + humidityValue + "%</span><br><br></div>");
  client.println("<div class=\"info\"><img src=\"https://cdn-icons-png.flaticon.com/512/6997/6997098.png\" width=\"60\" height=\"60\"><span>" + temperatureValue + "C</span></div>");
  client.println("<div class=\"info\"> <img src=\"https://cdn-icons-png.flaticon.com/512/3211/3211812.png\" width=\"60\" height=\"60\"><span>" + motor + "</span><br><br><br></div>");
  client.println("<div class=\"buttons\"><a href=\"/MOTOR=OFF\"><button>Motor Stop</button></a> <a href=\"/MOTOR=ON\"><button>Motor Start</button></a></div>");
  client.println("</body>");
  client.println("</html>");
  client.flush();
  delay(1000);
}

void handleSignals()
{
  brightnessValue = "";
  temperatureValue = "";
  humidityValue = "";
  timeValue = "";
  motorValue = "";
  int i = 1;
  while (returnbuff[i] != ';')
  {
    timeValue += returnbuff[i];
    i++;
  }
  i++;

  while (returnbuff[i] != ';')
  {
    temperatureValue += returnbuff[i];
    i++;
  }
  i++;

  while (returnbuff[i] != ';')
  {
    humidityValue += returnbuff[i];
    i++;
  }

  i++;
  while (returnbuff[i] != ';')
  {
    brightnessValue += returnbuff[i];
    i++;
  }
  i++;

  motorValue = returnbuff[i];

  Serial.print("Time: " + timeValue);
  Serial.print("Temp: " + temperatureValue);
  Serial.print("Hum: " + humidityValue);
  Serial.print("Light: " + brightnessValue);
  Serial.print("Motor: " + motorValue);
  writeToFirebase();
}

// Sends received values & motorValue to Firebase
void writeToFirebase()
{
  if (Firebase.ready() && signupOK)
  {

    if (Firebase.RTDB.setString(&fbdo, "signals/time", timeValue))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    count++;

    if (Firebase.RTDB.setString(&fbdo, "signals/temp", temperatureValue))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, "signals/humidity", humidityValue))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, "signals/brightness", brightnessValue))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, "signals/motor", motorValue))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}
