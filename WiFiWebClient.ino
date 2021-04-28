// Arduino Thesis Project
// Michael Brereton 2021
// This program incorporates some code from the Arduino examples https://www.arduino.cc/en/Tutorial/LibraryExamples/WiFiWebClient
// This program also incorporates some code from the accelerometer supplier https://wiki.dfrobot.com/Triple_Axis_Accelerometer_BMA220_Tiny__SKU_SEN0168

#include <Wire.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include "DHT.h"
#include "arduino_secrets.h" 

// pin for DHT sensor
#define DHTPIN 4
// pin for magnetic switch
#define MAG_SWITCH_PIN 2
// server port number
#define PORT_NUM 12100

// variables for accelerometer
byte Version[3];
int8_t x_data;
int8_t y_data;
int8_t z_data;
byte range = 0x01;
float divisor = 16;
float x,y,z, maxAccel;
float maxAccels[20];

// hide these in arduino secrets
char ssid[] = SECRET_SSID;        // network SSID
char pass[] = SECRET_PASS;    // network password 

int status = WL_IDLE_STATUS;
// server IP address
IPAddress server(192, 168, 1, 2);

// Initialize the WIFI client library
WiFiClient client;

// initialise DHT11 class
DHT dht(DHTPIN, DHT11);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, PORT_NUM)) {
    Serial.println("connected to server");
  }

  // start dht sensor
  dht.begin();

  // set magnetic switch pin to input mode
  pinMode(MAG_SWITCH_PIN, INPUT); 

  Wire.begin();
  Wire.beginTransmission(0x0A); // address of the accelerometer
  // range settings
  Wire.write(0x22); // register address
  Wire.write(range); //can be set from 00 to 04 depending on mode
  // low pass filter
  Wire.write(0x20); // register address
  Wire.write(0x05); 
  Wire.endTransmission();
}

void loop() {
  // read humidity from sensor
  int humidity = dht.readHumidity();
  bool doorOpen = false;
  float maxValue = maxAccels[0];

  switch(range)  //change the data dealing method based on the range
  {
    case 0x00:divisor=16;  break;
    case 0x01:divisor=8;  break;
    case 0x02:divisor=4;  break;
    case 0x03:divisor=2;  break;
    default: Serial.println("range setting is Wrong,range:from 0to 3.Please check!");while(1);
  }

  byte i = 0;
  while (i < 20) {
    maxAccels[i] = AccelerometerInit();
    delay(100);
    i++;
  }

  for(byte i = 0; i < 20; i++) {
      if(maxAccels[i] > maxValue) {
          maxValue = maxAccels[i];
      }
  }
  
  if (digitalRead(MAG_SWITCH_PIN) == HIGH) {
    doorOpen = false;
  } else {
    doorOpen = true;
  }

  // restart loop early if sensor read fails
  if (isnan(humidity)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }


  // if disconnected, attempt to reconnect
  if (!client.connected()) {
    Serial.println("disconnected from server.");
    Serial.println("attempting to reconnect");
    if (client.connect(server, PORT_NUM)) {
      Serial.println("reconnected to server");
    }
  }


  Serial.print("humidity = ");
  Serial.print(humidity);
  Serial.print(" doorOpen = ");
  Serial.println(doorOpen);
  Serial.print(" acceleration = ");
  Serial.println(maxValue);
  
  // if there are incoming bytes available
  // from the server, read them and print them:
  // prints initial server response then proceeds to continue messaging server and printing further responses
  if (client.connected()) {

    client.print(humidity);
    client.print(doorOpen);
    client.print(maxValue);
    client.print("~");
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
    };
  }
  
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

float AccelerometerInit() {
  Wire.beginTransmission(0x0A);
  // reset the accelerometer
  Wire.write(0x04); 
  Wire.endTransmission();
  Wire.requestFrom(0x0A,1);
  while(Wire.available()) {
    Version[0] = Wire.read();
  }
  x_data=(int8_t)Version[0]>>2;

  Wire.beginTransmission(0x0A);
  // reset the accelerometer
  Wire.write(0x06); // Y data
  Wire.endTransmission();
  Wire.requestFrom(0x0A,1);
  while(Wire.available()) {
    Version[1] = Wire.read();
  }
  y_data=(int8_t)Version[1]>>2;

  Wire.beginTransmission(0x0A);
  // reset the accelerometer
  Wire.write(0x08); // Y data
  Wire.endTransmission();
  Wire.requestFrom(0x0A,1);
  while(Wire.available()) {
    Version[2] = Wire.read();
  }
   z_data=(int8_t)Version[2]>>2;

   x=(float)x_data/divisor;
   y=(float)y_data/divisor;
   z=(float)z_data/divisor;

   return sqrt(sq(x)+sq(y)+sq(z));
}
