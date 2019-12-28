#include <LiquidCrystal_PCF8574.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "DHT.h"

#define DHTPIN D6 // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_PCF8574 lcd(0x27); // Set the LCD address to 0x27
StaticJsonDocument<2048> json;
StaticJsonDocument<2048> jsonTemp;
WiFiClient client;
char servername[]="api.openweathermap.org";  // remote server we will connect to
String APIKEY = "YOURAPIKEY";
String CityID = "3838583"; // Rosario, Argentina
int counter = 5;

WiFiUDP ntpUDP;
// Offset of -10800 seconds to show UTC-3 time
NTPClient timeClient(ntpUDP, "south-america.pool.ntp.org", -10800);
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup(){
  Serial.begin(9600);
  Serial.println();
 
  dht.begin(); // Initialize the temperature and humidity sensor
 
  lcd.begin(16, 2); // Initialize the lcd for a 16 chars and 2 line display
  lcd.setBacklight(true); // Turn on the LCD backlight

  WiFi.begin("YOURWIFINAME", "YOURWIFIPASSWORD");

  Serial.print("Connecting to WIFI");
  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  lcd.setCursor(0, 1);
  lcd.print("to WIFI...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP address:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(3000);
  lcd.clear();
}

void printLines(String firstLineToDisplay, String secondLineToDisplay) {
  lcd.setCursor(0, 0);
  lcd.print(firstLineToDisplay);
  lcd.setCursor(0, 1);
  lcd.print(secondLineToDisplay);
  delay(4000);
  lcd.clear();
}

void getWeatherData(){

  lcd.setCursor(0, 0);
  lcd.print("Getting data");
  lcd.setCursor(0, 1);
  lcd.print("from API...");
 
  if (client.connect(servername, 80)) {  //starts client connection, checks for connection
    client.println("GET /data/2.5/weather?id="+CityID+"&units=metric&APPID="+APIKEY);
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed"); //error message if no client connect
    Serial.println();
  }

  while(client.connected() && !client.available()) delay(1); //waits for data

  while (client.connected() || client.available()) { //connected or data available
    String serverResponse = client.readStringUntil('\n'); // Read and store server response
    deserializeJson(jsonTemp, serverResponse);
    int statusCode = jsonTemp["cod"];
    if(statusCode == 200){ // We check if we got a valid server response
      json = jsonTemp;
    }
  }
  client.stop(); //stop client
 
  lcd.clear();
}

void loop(){
  
  // We get new data from the server after 5 loops.
  if(counter == 5){
    getWeatherData();
    counter = 0;
  } else {
    counter++;
  }

  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  int currentHour = timeClient.getHours();
  int currentMinutes = timeClient.getMinutes();
  int internalTemp = dht.readTemperature();
  int internalHum = dht.readHumidity();

  printLines(daysOfTheWeek[timeClient.getDay()], (String)currentHour + ":" + (String)currentMinutes + "hs");
  
  printLines("Internal sensor", "Temp:" + (String)internalTemp + "C Hum:" + (String)internalHum + "%");

  // First we save as int to remove the decimals because of an LCD panel characters limitation.
  int tempWithoutDecimals = json["main"]["temp"];
  String hum = json["main"]["humidity"];
  printLines("Open weather map", "Temp:" + (String)tempWithoutDecimals + "C Hum:" + hum + "%");

  // We check for the rain object, (the API only sends it if it has rained).
  if(json["rain"]["1h"]){
    String rain1h = json["rain"]["1h"];
    printLines("Rain volume last", "hour: " + rain1h + "mm");
  }
  if(json["rain"]["3h"]){
    String rain3h = json["rain"]["3h"];
    printLines("Rain volume last", "3 hours: " + rain3h + "mm");
  }

}
