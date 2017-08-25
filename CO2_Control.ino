/////////////////////////////////////////////////
// Finalized code for the Co2 portion of the 
// Greaux Engineering Section.  Includes Wifi
// Connectivity and MQTT messaging protocol
//
// ** DO NOT FORGET TO CHANGE NETWORK SSID AND
//    NETWORK PASSWORD IN "WIFI VARIABLES" 
//    SECTION **
//
// Written by: Chase Brumfield
/////////////////////////////////////////////////

#include <PubSubClient.h>        //enable MQTT Library
#include <Wire.h>
#include <ESP8266WiFi.h>         //enable ESP8266 Wifi Library
extern "C"{
#include "user_interface.h"
}
////////////////////////////////////////////
//////COUNTER VARIABLES/////////////////////
////////////////////////////////////////////

int release_counter = 0;  //counter for number of times CO2 releases

/////////////////////////////////////////////
/////// Wifi Variables //////////////////////
/////////////////////////////////////////////

const char* ssid = "YOUR NETWORK SSID";        //Internet Network Name
const char* password = "YOUR NETWORK PASSWORD";    //Wifi Password
char c[8];
long lastMsg = 0;
int value = 0;

////////////////////////////////
//MQTT//////////////////////////
////////////////////////////////

const char* mqtt_server = "192.168.0.1";    //MQTT Server IP Address
WiFiClient espClient;           //Create Wifi client named espClient
PubSubClient client(espClient);

/////////////////////////////////////////////
// CO2 VARIABLES ////////////////////////////
/////////////////////////////////////////////

float min_diff = 50;
int CO2min = 400;       //Below which CO2 is considered LOW
int CO2max = 1200;       //Above which CO2 is considered HIGH
int buzzerON = 5000;
int SolenoidPin = D8;
int SensorPin = D7;
int BuzzerPin = D5;
long previousMillis = 0;
int co2_release_time = 5000; //time to keep solenoid open
int co2_read_interval = 3000; //delay between CO2 readings

int diff1;    //Stores the difference between sequential readings
int diff2;
int diff3;

int CO2_1;    //Stores each CO2 reading
int CO2_2;
int CO2_3;

///////////////////////////////////////////////
// MQTT RECONNECT FUNCTION ////////////////////
///////////////////////////////////////////////

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
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
// WIFI-SETUP FUNCTION //////////////////////
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


//////////////////////////////////
// READ CO2 VIA I2C FUNCTION /////
//////////////////////////////////

int readCO2() {
  
  int co2Addr = 0x68; //address of CO2 sensor
  int co2_value = 0;  //CO2 value stored inside this variable.

  //////////////////////////
  /* Begin Write Sequence */
  //////////////////////////
  Wire.beginTransmission(co2Addr);
  Wire.write(0x22);
  Wire.write(0x00);
  Wire.write(0x08);
  Wire.write(0x2A);

  Wire.endTransmission();

  /////////////////////////
  /* End Write Sequence. */
  /////////////////////////

  delay(10); // Necessary to ensure data is properly written to RAM

  /////////////////////////
  /* Begin Read Sequence */
  /////////////////////////

  // Request 2 bytes and read 4 bytes. 
  // Includes payload, checksum, and command status byte.

  Wire.requestFrom(co2Addr, 4);

  byte i = 0;
  byte buffer[4] = {0, 0, 0, 0};

  while (Wire.available())
  {
    buffer[i] = Wire.read();
    i++;
  }

client.loop();    // Added to make sure MQTT stays connected

  ///////////////////////
  /* End Read Sequence */
  ///////////////////////

// Bitwise mmanipulation is used to create an interger value
  
  co2_value = 0;
  co2_value |= buffer[1] & 0xFF;
  co2_value = co2_value << 8;
  co2_value |= buffer[2] & 0xFF;

  byte sum = 0; //Checksum Byte
  sum = buffer[0] + buffer[1] + buffer[2]; //Byte addition utilizes overflow

  if (sum == buffer[3])
  {
    return co2_value;
  }
}


void setup()
{

  Serial.begin(9600);
  Serial.println("The Greaux Engineering CO2 System is Active...");
  pinMode(BuzzerPin, OUTPUT); // First, Set all necessary pins OUTPUT and LOW
  digitalWrite(BuzzerPin, LOW);
  pinMode(SolenoidPin, OUTPUT);
  digitalWrite(SolenoidPin, LOW);
  Serial.println("Initializing the Sensor...");
  pinMode (SensorPin, OUTPUT);
  digitalWrite(SensorPin, LOW);
  delay(3000);
  Wire.begin();
  delay(1000);
  setup_wifi();                         //run wifi setup function
  delay(1000);
  client.setServer(mqtt_server, 1883);  //Set MQTT server to port 1883
  delay(1000);
  digitalWrite(SensorPin, HIGH);
  delay(4000);

}

void loop()
{ 

char msg[10];   //buffer variable for MQTT data transmission
  
  if (!client.connected()) {
    reconnect(); //reconnect MQTT server if disconnectec
  }
  client.loop();
  
  Serial.println("Measuring CO2:...");

  ///////////////////////////////////////////////
  // Trying to achieve an accurate CO2 Reading //
  ///////////////////////////////////////////////

  //System takes three readings, makes sure none of them is zero
  //and then compares the difference between all three readings.
  //If the difference is less than a chosen value, it considers
  //the last reading accurate and moves on.

  CO2_1 = readCO2();
  Serial.println(CO2_1);
  delay(co2_read_interval);
  CO2_2 = readCO2();
  Serial.println(CO2_2);
  delay(co2_read_interval);
  CO2_3 = readCO2();
  Serial.println(CO2_3);
  delay(co2_read_interval);

  if ((CO2_1 == 0) || (CO2_2 == 0) || (CO2_3 == 0))
  {
    Serial.println("SENSOR READING: NOT ACCURATE");
    client.loop();
    client.publish("sensor/co2/status","POWER CYCLE");
    digitalWrite(SensorPin, LOW);
    delay(50);
    digitalWrite(SensorPin, HIGH);
    delay(4000);

    //If a zero reading is received, we power cycle the sensor
    //using a relay.  This is not the best solution, but occasionally
    //the sensor hangs on zero and it's the only fix we could find.
    //the problem appears to be effected by the length of delays used
  }
  else {

    diff1 = CO2_1 - CO2_2;
    diff2 = CO2_2 - CO2_3;
    diff3 = CO2_3 - CO2_1;

    Serial.println("The differences between values are");
    Serial.println(fabs(diff1));
    delay(500);
    Serial.println(fabs(diff2));
    delay(500);
    Serial.println(fabs(diff3));
    delay(500);

    if ((fabs(diff1) <= min_diff) && (fabs(diff2) <= min_diff) && (fabs(diff3) <= min_diff))
    {

      Serial.println("CO2 Reading is ACCURATE...");
      client.loop();
     
      ///////////////////////////////////////////////////////
      //Once we have an accurate reading, if the
      //CO2 level is LOWER than the acceptable range
      //we open the solenoid for a specified amount of time
      //All information is transferred via MQTT for wireless
      //user viewing.
      /////////////////////////////////////////////////////

      if (CO2_3 <= CO2min) {
        Serial.print("Publish message: CO2 Reading is LOW: ");
        Serial.println(CO2_3);
        Serial.println("RELEASING CO2");
        String CO2String = String(CO2_3);
        CO2String.toCharArray(msg, CO2String.length()+1); 
        client.publish("sensor/co2/status","LOW");
        client.publish("sensor/co2/value", msg);

        char counter[4];
        release_counter = release_counter+1;
        String releaseString = String(release_counter);
        releaseString.toCharArray(counter, releaseString.length()+1);
        Serial.println("Number of RELEASES: ");
        Serial.println(counter);
        client.publish("sensor/co2/counter",counter);
        client.loop();

        digitalWrite(SolenoidPin, HIGH);
        delay(co2_release_time);
        digitalWrite(SolenoidPin, LOW);
      }


      else if ((CO2_3 > CO2min) && (CO2_3 < CO2max)) {
        Serial.println("CO2 is IN RANGE");
        Serial.println(CO2_3);
        client.loop();
        String CO2String = String(CO2_3);
        CO2String.toCharArray(msg, CO2String.length()+1);  
        client.publish("sensor/co2/status","IN RANGE");
        client.publish("sensor/co2/value", msg);
        delay(1000);
        }
      

      else {
        Serial.println("CO2 is HIGH");
        Serial.println(CO2_3);
        client.loop();
        String CO2String = String(CO2_3);
        CO2String.toCharArray(msg, CO2String.length()+1); 
        client.publish("sensor/co2/status","HIGH");
        client.publish("sensor/co2/value", msg);
        if (CO2_3 >= buzzerON){
        tone(BuzzerPin, 2000, 10000);
        delay(1000);
      } 
      }
    }
    else
    {
      Serial.println("Reading is NOT ACCURATE. BEGINNING NEW READINGS...");
      Serial.println(CO2_3);
      client.loop();
      String CO2String = String(CO2_3);
      CO2String.toCharArray(msg, CO2String.length()+1); 
      client.publish("sensor/co2/status","NOT ACCURATE");
      client.publish("sensor/co2/value", msg);
      delay(1000);
    }
  }
}




