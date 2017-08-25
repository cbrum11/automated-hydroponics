//////////////////////////////////////////////////
// Finalized code for water level
// measuring and reporting for
// The Greaux Engineering System.
// Includes wireless connectivity and MQTT
// data transfer.
//
// Written by Chase Brumfield and Kevin Baudoin
/////////////////////////////////////////////////
// DONT FORGET TO CHANGE NETWORK ID VARIABLES TO YOUR
// NETWORK NAME AND PASSWORD.  ALSO CHANGE THE MQTT IP
// ADDRESS VARIABLE TO YOUR MQTT IP ADDRESS
//////////////////////////////////////////////////
// 1. Connects to wifi and MQTT
// 2. Reports the water level based on the status (open
//    or closed) of three broken finger switches
//////////////////////////////////////////////////
// IMPORTANT NOTE: Because of the way
// The WEMOS uses pins to reset itself, you
// Must open the circuit associated with pin
// D4 to upload a new sketch
/////////////////////////////////////////////////

#include <PubSubClient.h>        //enable MQTT Library
#include <ESP8266WiFi.h>
extern "C"{
#include "user_interface.h"
}
///////////////////////////////////
//////////COUNTER VARIABLES////////
///////////////////////////////////

unsigned long previousMillis = 0;
int release_counter = 0;

////////////////////////////////
//MQTT//////////////////////////
////////////////////////////////

const char* mqtt_server = "YOUR MQTT SERVER IP ADDRESS";    //MQTT Server IP Address
WiFiClient espClient3;           //Create Wifi client named espClient
PubSubClient client(espClient3);

/////////////////////////////////////////////
/////// Wifi Variables //////////////////////
/////////////////////////////////////////////
const char* ssid = "YOUR NETWORK SSID NAME";        //Internet Network Name
const char* password = "YOUR NETWORK PASSWORD";    //Wifi Password
char c[8];
long lastMsg = 0;
int value = 0;

///////////////////////////////////////////////
// Reconnect Function /////////////////////////
///////////////////////////////////////////////

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client3")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/////////////////////////////////////////////
// Wifi-Setup Function //////////////////////
/////////////////////////////////////////////

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  wifi_set_sleep_type(NONE_SLEEP_T);
  WiFi.persistent(false); //These 3 lines are a required work around
  WiFi.mode(WIFI_OFF);    //otherwise the module will not reconnect
  WiFi.mode(WIFI_STA);    //if it gets disconnected
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//Initialize all pins as LOW and assign
//different pins to appripriate tri-color
//LED Values 

const int REED_PIN_LOW  = D1;      // LED_LOW pin - active-high
const int REED_PIN_MED  = D2;      // LED_MED pin - active-high
const int REED_PIN_HIGH = D4;      // LED_HIGH pin - active-high
const int NECESSARY_GROUND = D3;

int redPin = D5;
int greenPin = D6;
int bluePin = D7;

int buzzerPin = D8;

void setup() 
{ 
  Serial.begin(9600);

  pinMode(NECESSARY_GROUND, OUTPUT);
  digitalWrite(NECESSARY_GROUND, LOW);
  
  pinMode(REED_PIN_LOW, INPUT_PULLUP);
  pinMode(REED_PIN_MED, INPUT_PULLUP);
  pinMode(REED_PIN_HIGH, INPUT);
  digitalWrite(REED_PIN_HIGH, HIGH);
 
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  pinMode(buzzerPin, OUTPUT);

  setup_wifi();                         //run wifi setup function
  delay(1000);
  client.setServer(mqtt_server, 1883);  //Set MQTT server to port 1883
  delay(1000);
}

void loop() 
{
unsigned long currentMillis = millis();
  
 Serial.printf("Wifi Connection status: %d\n", WiFi.status());
   
   if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Serial.println("The Client State is...");
  Serial.println(client.state());

  char msg[40];  // Buffer variable for MQTT data
  int proximity_low = digitalRead(REED_PIN_LOW);                              // 
  int proximity_med = digitalRead(REED_PIN_MED);                              //
  int proximity_high = digitalRead(REED_PIN_HIGH);                            //
  
   
  if (proximity_low == HIGH)      // If the pin reads HIGH, the switch is OPEN, and water is LOW.
  {
    Serial.println("WATER LEVEL LOW!!! About 10 Gallons Remaining.");                                  //
    digitalWrite(redPin, HIGH);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);

    if(currentMillis - previousMillis >= 3000){
    client.publish("sensor/waterlevel/status", "LOW");
    client.publish("sensor/waterlevel/value", "10");
    previousMillis = currentMillis;
    }
  }
  
  else if (proximity_med == HIGH)
  {
    Serial.println("WATER LEVEL MEDIUM. About 15 Gallons Remaining.");
    digitalWrite(redPin, 1);
    digitalWrite(greenPin, 0);
    digitalWrite(bluePin, HIGH); 

    if(currentMillis - previousMillis >= 3000){
    client.publish("sensor/waterlevel/status", "MEDIUM");   
    client.publish("sensor/waterlevel/value", "15");    
    previousMillis = currentMillis;
    }
    
    }

    
  else if (proximity_high == HIGH)
  {
    Serial.println("SUMP TANK IS OVERFLOWING!!!");
    digitalWrite(redPin, HIGH);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);

    if(currentMillis - previousMillis >= 3000){
    client.publish("sensor/waterlevel/status", "HIGH -- OVERFLOWING!!!");  
    client.publish("sensor/waterlevel/value", "30");
    previousMillis = currentMillis;
    }
    
    tone(buzzerPin, 750, 500);
    //Serial.println("buzzer hot");
    delay(500);
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);
    //tone(buzzerPin, 0, 500);
    //Serial.println("buzzer off");
    delay(500);
    digitalWrite(redPin, HIGH);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);
    //tone(buzzerPin, 60, 500);
    //Serial.println("buzzer hot 2");
    }

    
  else
  {
    Serial.println("WATER LEVEL IS JUST SWELL.");
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, HIGH);
    digitalWrite(bluePin, LOW);

    if(currentMillis - previousMillis >= 3000){
    client.publish("sensor/waterlevel/status", "FULL");
    client.publish("sensor/waterlevel/value", "30"); 
    previousMillis = currentMillis;
    }
  }
}

