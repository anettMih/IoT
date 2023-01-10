#include <ESP8266WiFi.h>

#include<SPI.h>

char buff[] = "S\n";
char kuldes_tesztje[20];
char returnbuff[100];

SPISettings spi_settings(100000, MSBFIRST, SPI_MODE0);
//100 kHz legyen a sebesseg, a Node tud 80MHzt de az Arduino csak 16MHzt
volatile bool motorState;

// Wi-Fi settings
const char* ssid = "DIGI-D69p";
const char* password = "8wjBc9cg";

// Values received from payload
String brightnessValue = "";
String temperatureValue = "";
String humidityValue = "";
String timeValue = "";
String motorValue = "";


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
}

void loop()
{

  SPI.beginTransaction(spi_settings);
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

  buff[0] = motorState ? '1' : '0';
  //elkuldi az uzenetet
  for (int i = 0; i < sizeof(buff); i++)
  {
    kuldes_tesztje[i] = SPI.transfer(buff[i]);
    delay(100);
  }

  //kuld meg 100 pontot,ezeket az Arduino felulirja
  //ami visszajon azt kiolvassuk betesszuk a returnbuffbe
  for (int i = 0; i < 50; i++)
  {
    returnbuff[i] = SPI.transfer('.');
    delay(10);
  }
  Serial.println();
  SPI.endTransaction();

  //itt kiirjuk. Az SPI sokkal gyorsabb volt mint a soros port
  //ezert ott kozben nem probaljuk meg kiirni, a soros kiiratas lassu
  //  for (int i = 0; i < 20; i++)
  //    Serial.print(kuldes_tesztje[i]);

  Serial.println("Reply: ");
  handleSignals();
  for (int i = 0; i < 50; i++)
    returnbuff[i] = 0;

  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  
  client.print("<meta http-equiv=\"refresh\" content=\"2\">");
  client.println("<body style=\"background-color:powderblue;\">");
  client.println("<label>Fenyero:</label><br>");
  client.println("<p>" + brightnessValue + "</p><br>");
  client.println("<label>Homerseklet:</label><br>");
  client.println("<p>" + temperatureValue + "</p><br>");
  client.println("<label>Nedvessegtartalom:</label><br>");
  client.println("<p>" + humidityValue + "</p><br>");
  client.println("<label>Ido:" + timeValue +" </label><br>");
  client.println("<p>" + timeValue + "</p><br><br>");
  client.println("<label>Motor: </label><br>");
  client.println("<p>" + String(motorState) + "</p><br><br>");
  client.println("<a href=\"/MOTOR=OFF\"\"><button>Megallit Motor</button></a>");
  client.println("<a href=\"/MOTOR=ON\"\"><button>Elindit Motor</button></a>");
  client.println("</body>");
  client.flush();
  delay(1000);
}


void handleSignals() {
  brightnessValue = "";
  temperatureValue = "";
  humidityValue = "";
  timeValue = "";
  motorValue = "";
  int i = 0;
  while (returnbuff[i] != ';') {
    timeValue += returnbuff[i];
    i++;
  }
  i++;
  while (returnbuff[i] != ';') {
    temperatureValue += returnbuff[i];
    i++;
  }
  i++;
  while (returnbuff[i] != ';') {
    humidityValue += returnbuff[i];
    i++;
  }
  i++;
  while (returnbuff[i] != ';') {
    brightnessValue += returnbuff[i];
    i++;
  }
  i++;
  motorValue = returnbuff[i];
  Serial.println("Time: " + timeValue);
  Serial.println("Temp: " + temperatureValue);
  Serial.println("Hum: " + humidityValue);
  Serial.println("Light: " + brightnessValue);
  Serial.println("Motor: " + motorValue);


}
