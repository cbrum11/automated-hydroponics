/* 
Have plans to better comment all the code below
[4/11/2017]
*/

#include <Wire.h>
#include <ESP8266WiFi.h>         //enable ESP8266 Wifi Library
#include <PubSubClient.h>        //enable MQTT Library

/////////////////////////////////////////////
/////// Wifi Variables //////////////////////
/////////////////////////////////////////////
const char* ssid = "YOUR SSID";        //Internet Network Name
const char* password = "YOUR PASSWORD";    //Wifi Password
const char* mqtt_server = "MQTT BROKER IP ADDRESS";    //MQTT Server IP Address
WiFiClient espClient;           //Create Wifi client named espClient
char c[8];
/////////////////////////////////////////////
// MQTT Variables ///////////////////////////
/////////////////////////////////////////////
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
/////////////////////////////////////////////
// CO2 VARIABLES ////////////////////////////
/////////////////////////////////////////////
float min_diff = 50;
int CO2min = 200;       //[PPM] Below which CO2 is considered LOW
int CO2max = 800;       //[PPm] Above which CO2 is considered HIGH
int SolenoidPin = D8;   //Solenoid that controls CO2 Release
int SensorPin = D7;     //Data pin for CO2 sensor
int BuzzerPin = D5;     //Buzzer pin to alert user if high CO2 concentration is present
long previousMillis = 0;
long interval = 7000; //[ms] time to keep solenoid open

int diff1;    //Stores the difference between sequential readings CO2 readings
int diff2;
int diff3;

int CO2_1;    //Stores each CO2 reading
int CO2_2;
int CO2_3;


/////////////////////////////////////////////
// Wifi-Setup Function //////////////////////
/////////////////////////////////////////////

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.persistent(false); //These 3 lines are a required bug work-around,
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

///////////////////////////////////////////////
// Callback Function //////////////////////////
///////////////////////////////////////////////

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

///////////////////////////////////////////////
// Reconnect Function /////////////////////////
///////////////////////////////////////////////

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//////////////////////////////////
//////////////////////////////////
// Read CO2 over I2C Function ////
//////////////////////////////////
//////////////////////////////////

int readCO2()
{
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

  /*
    We wait 10ms for the sensor to process our command.
    Waiting 10ms will ensure the  data is properly written to RAM
  */

  delay(10);

  /////////////////////////
  /* Begin Read Sequence */
  /////////////////////////

  /*
    Since we requested 2 bytes from the sensor we must
    read in 4 bytes. This includes the payload, checksum,
    and command status byte.

  */

  Wire.requestFrom(co2Addr, 4);

  byte i = 0;
  byte buffer[4] = {0, 0, 0, 0};

  /*
    Wire.available() is not nessessary. Implementation is obscure but we leave
    it in here for portability and to future proof our code
  */
  while (Wire.available())
  {
    buffer[i] = Wire.read();
    i++;
  }

  ///////////////////////
  /* End Read Sequence */
  ///////////////////////

  /*
    Bitwise manipulation to change buffer into
    integer
  */

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
  else
  {
    // Failure!
    /*
      Checksum failure.  Most likely the sensor was busy...
    */
    return 0;
  }
}

void setup()
{

  Serial.begin(9600);
  Serial.println("The Greaux Engineering CO2 System is Active...");
  pinMode(BuzzerPin, OUTPUT);
  digitalWrite(BuzzerPin, LOW);
  pinMode(SolenoidPin, OUTPUT);
  digitalWrite(SolenoidPin, LOW);
  Wire.begin();
  delay(1000);
  Serial.println("Initializing the Sensor...");
  pinMode (SensorPin, OUTPUT);
  digitalWrite(SensorPin, LOW);
  delay(2000);
  digitalWrite(SensorPin, HIGH);
  Serial.println("Sensor Initialized");
  delay(1000);
  setup_wifi();                         //run wifi setup function
  client.setServer(mqtt_server, 1883);  //Set MQTT server to port 1883
  client.setCallback(callback);         //Setup callback function

}

void loop()
{
  ////////////////////////////////////////
  // Handeling Wifi disconnect ///////////
  ////////////////////////////////////////

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Serial.println("Measuring CO2:...");

  /////////////////////////////////////////
  //Trying to achieve an accurate CO2 Reading
  ////////////////////////////////////////

  CO2_1 = readCO2();
  Serial.println(CO2_1);
  delay(2000);
  CO2_2 = readCO2();
  Serial.println(CO2_2);
  delay(2000);
  CO2_3 = readCO2();
  Serial.println(CO2_3);
  delay(2000);

  if ((CO2_1 == 0) || (CO2_2 == 0) || (CO2_3 == 0))
  {
    Serial.println("Publish message: CO2 Value is Reading 0: SENSOR MALFUNCTION");
    client.publish("sensor/co2/status", "SENSOR MALFUNCTION");
    client.publish("sensor/co2/value", "0");
  }
  else {

    diff1 = CO2_1 - CO2_2;
    diff2 = CO2_2 - CO2_3;
    diff3 = CO2_3 - CO2_1;

    //////////////////////////////////////////
    //If the difference between three readings
    //is less than threshold we set, we can assume
    //reading is accurate.  If not, we skip back up
    //and take 3 more readings
    //////////////////////////////////////////

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
      client.publish("sensor/co2/status", "CO2 Reading is ACCURATE...");

      ///////////////////////////////////////////////////////
      //Once we have an accurate reading, We check the range.
      //LOW --> Open solenoid for specified amount of time, send to MQTT, read again
      //IN RANGE --> sent to MQTT, read again
      //HIGH --> Sound buzzer, read again
      //The millis stuff keeps looping the code until
      //we reach our set time interval.
      /////////////////////////////////////////////////////

      if (CO2_3 <= CO2min) {
        Serial.print("Publish message: CO2 Reading is LOW: ");
        Serial.println(CO2_3);
        Serial.println("RELEASING CO2");
        client.publish("sensor/co2/status","LOW.");
        snprintf(msg, 75, "%ld", CO2_3);
        client.publish("sensor/co2/value", msg);
        client.publish("sensor/co2/status","RELEASING CO2");

        unsigned long currentMillis = millis();
        Serial.println(currentMillis);
        while (abs(currentMillis - millis()) < interval)
        {
          digitalWrite(SolenoidPin, HIGH);
        }
        delay(7000);
        Serial.println(millis());
        digitalWrite(SolenoidPin, LOW);
        client.publish("sensor/co2/status", "END RELEASE CO2");
      }


      else if ((CO2_3 > CO2min) && (CO2_3 < CO2max)) {
        Serial.println("CO2 is IN RANGE");
        Serial.println(CO2_3);
        client.publish("sensor/co2/status","IN RANGE");
        snprintf(msg, 75, "%ld", CO2_3);
        client.publish("sensor/co2/value", msg);
        delay(1000);
      }


      else {
        Serial.println("CO2 is HIGH");
        Serial.println(CO2_3);
        tone(BuzzerPin, 2000, 10000);
        client.publish("sensor/co2/status", "HIGH");
        snprintf(msg, 75, "%ld", CO2_3);
        client.publish("sensor/co2/value", msg);
        delay(1000);
      }
    }
    else
    {
      Serial.println("Reading is NOT ACCURATE. BEGINNING NEW READINGS...");
      Serial.println(CO2_3);
      client.publish("sensor/co2/status", "CO2 Reading is NOT ACCURATE");
      snprintf(msg, 75, "%ld", CO2_3);
      client.publish("sensor/co2/value", msg);
      client.publish("sensor/co2/status", "BEGINNING NEW READINGS");
      delay(1000);
    }
  }
  delay(2000);


}




