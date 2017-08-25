/////////////////////////////////////////////
// Finalized EC and PH code for the Greaux
// Engineering System.  Includes wireless
// connectivity and MQTT data transfer
////////////////////////////////////////////
// DO NOT FORGET TO CHANGE MQTT IP, NETWORK SSID
// NAME, AND NETWORK PASSWORD VARIABLES TO 
// MATCH YOUR NETWORK
///////////////////////////////////////////
///////////////////////////////////////////
// 1. Coneccts to Wifi and MQTT
// 2. Measures EC and checks accuracy
// 3. Measures PH 
// 4. If EC is found accurate doses based on range
//    HIGH --> Do nothing (system will acclimate)
//    LOW  --> Add nutrients
//    IN RANGE --> Move to test PH accuracy
// 5. Reads PH and checks accuracy
// 6. If PH is found accuracte doses based on range
//    HIGH --> Doses PH down
//    LOW  --> Doses PH up
//    IN RANGE --> Move back to EC reading
// 7. Reports all readings and status via MQTT for viewing
///////////////////////////////////////////////////////
// Written by: Chase Brumfield
///////////////////////////////////////////////////////

///////////////////////
#include <PubSubClient.h>        //enable MQTT Library
#include <ESP8266WiFi.h>
#include <Wire.h>                //enable I2C Library.
extern "C"{
#include "user_interface.h"
}
///////////////////////////////////////////
/////////COUNTER VARIABLES/////////////////
///////////////////////////////////////////

int release_up_counter = 0;
int release_down_counter = 0;
int release_nutrients_counter = 0;

////////////////////////////////////////////
// PH and EC Variables /////////////////////
////////////////////////////////////////////

#define PHaddress 99             //default I2C ID number for EZO pH Circuit.
#define ECaddress 100            //default I2C ID number for EZO EC Circuit

char computerdata[20];           //we make a 20 byte character array to hold incoming data from a pc/mac/other.
byte received_from_computer = 0; //we need to know how many characters have been received.
byte code = 0;                   //used to hold the I2C response code.
char ph_data[20];                //we make a 20 byte character array to hold incoming data from the pH circuit.
char ec_data[48];                //we make a 48 byte character array to hold incoming data from the EC circuit.
byte in_char = 0;                //used as a 1 byte buffer to store in bound bytes from the pH Circuit.
byte i = 0;                      //counter used for ph_data array.
int time_ = 1800;                //used to change the delay needed depending on the command sent to the EZO Class pH Circuit.
int delay_time = 2000;           //used to change the delay needed depending on the command sent to the EZO Class EC Circuit.

int PHup = D6;        //Wemos pin attached to PH up dosing motor
int PHdown = D7;      //Wemos pin attached to PH down dosing motor
int Nutrients = D8;   //Wemos pin attached to nutrients dosing motor

float max_diffPH = 0.10;     //maximum difference between PH readings for readings to be considered accurate
float max_diffEC = 10.0;     //maximum difference between PH readings for readings to be considered accurate
float PHmax = 6.5;           //Desired PH Range high value [Log Scale]
float PHmin = 5.5;           //Desired PH Range low value [Log Scale]
float ECmax = 1300.0;        //Desired EC Range high value [uS/cm]
float ECmin = 1200.0;        //Desired EC Range low value [uS/cm]
float ph_float;              //float var used to hold the float value of pH
int PHup_delay = 15000;      //amount of time to pump PH up solution when reading is LOW
int PHdown_delay = 7500;     //amount of time to pump PH down solution when reading is HIGH
int Nutrients_delay = 15000; //amount of time to release nutrients when reading is LOW

char *ec;          //char pointer used in string parsing.
char *tds;         //char pointer used in string parsing.
char *sal;         //char pointer used in string parsing.
char *sg;          //char pointer used in string parsing.

float ec_float;
float tds_float;
float sal_float;
float sg_float;

float PH_1;   //Storing PH Readings
float PH_2;   
float PH_3;

float EC_1;   //Storing EC Readings
float EC_2;
float EC_3;

float diff1;  //For accuracy checking of sequential EC and PH readings
float diff2;
float diff3;

////////////////////////////////
// MQTT SETUP //////////////////
////////////////////////////////

const char* mqtt_server = "YOUR MQTT IP ADDRESS HERE";    //MQTT Server IP Address
WiFiClient espClient2;           //Create Wifi client named espClient
PubSubClient client(espClient2);

/////////////////////////////////////////////
/////// Wifi Variables //////////////////////
/////////////////////////////////////////////

const char* ssid = "YOUR NETWORK ID NAME";        //Internet Network Name
const char* password = "YOUR NETWORK PASSWORD";    //Wifi Password
char c[8];
long lastMsg = 0;
int value = 0;

///////////////////////////////////////////////
// MQTT Reconnect Function ////////////////////
///////////////////////////////////////////////

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client2")) {
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

///////////////////////////////////////////////
// PH ACCURACY AND ACTION FUNCTION ////////////
///////////////////////////////////////////////

//////////////////////////////////////////////
// Takes 3 readings and compares the difference
// between them to make sure this difference is below
// a set value.  If the difference is below this value,
// the reading is considered accurate.
// This part of the code also handles PH dosing if the measured
// value is within certain ranges.
//////////////////////////////////////////////

int PHloop() {  
 
 char msg[10];
 
 do
 {
  Serial.println("Measuring PH...");
  PH_1 = readPH();
  Serial.println(PH_1);
  delay(3000);
  PH_2 = readPH();
  Serial.println(PH_2);
  delay(3000);
  PH_3 = readPH();
  Serial.println(PH_3);
  delay(3000);
  
  diff1 = PH_1 - PH_2;
  diff2 = PH_2 - PH_3;
  diff3 = PH_3 - PH_1;

  Serial.println("The differences between values are");
  Serial.println(fabs(diff1));
  delay(500);
  Serial.println(fabs(diff2));
  delay(500);
  Serial.println(fabs(diff3));
  delay(500);

  Serial.println("READING NOT ACCURATE");
  String PHString = String(PH_3);
  PHString.toCharArray(msg, PHString.length()+1);
  client.publish("sensor/ph/status","NOT ACCURATE");
  client.publish("sensor/ph/value", msg);
  delay(1000);
 }
 
 while ((PH_1 == 0) || (PH_2 == 0) || (PH_3 == 0)||(fabs(diff1) >= max_diffPH) || (fabs(diff2) >= max_diffPH) || (fabs(diff3) >= max_diffPH));
 
 Serial.println("PH Reading is ACCURATE...");

      ///////////////////////////////////////////////////////
      //Once we have an accurate reading, if the
      //PH level is outside our acceptable range
      //we power the perisaltic pump for a specified amount of time
      //to dose the appropriate amount of PH solution
      /////////////////////////////////////////////////////

      if (PH_3 <= PHmin) {
        Serial.print("PH Reading is LOW: ");
        Serial.println(PH_3);
        Serial.println("Adding PH UP Solution...");
        
        String PHString = String(PH_3);
        PHString.toCharArray(msg, PHString.length()+1); 
        client.publish("sensor/ph/status","LOW");
        client.publish("sensor/ph/value", msg);

        char counter_ph_up[4];
        release_up_counter = release_up_counter+1;
        String releaseString = String(release_up_counter);
        releaseString.toCharArray(counter_ph_up, releaseString.length()+1);
        Serial.println("PH UP Release Count:");
        Serial.println(counter_ph_up);
        client.publish("sensor/phup/counter",counter_ph_up); 
 
        digitalWrite(PHup, HIGH);
        delay(PHup_delay);
        digitalWrite(PHup, LOW);
      }

      else if ((PH_3 > PHmin) && (PH_3 < PHmax)) {
        Serial.println("PH is IN RANGE");
        Serial.println(PH_3);
        String PHString = String(PH_3);
        PHString.toCharArray(msg, PHString.length()+1);
        client.publish("sensor/ph/status","IN RANGE");
        client.publish("sensor/ph/value", msg);
        
        delay(1000);
        
      }

      else {
        Serial.println("PH is HIGH");
        Serial.println(PH_3);
        Serial.println("Adding PH DOWN Solution...");
        
        String PHString = String(PH_3);
        PHString.toCharArray(msg, PHString.length()+1);
        client.publish("sensor/ph/status","HIGH");
        client.publish("sensor/ph/value", msg);

        char counter_ph_down[4];
        release_down_counter = release_down_counter+1;
        String releaseString = String(release_down_counter);
        releaseString.toCharArray(counter_ph_down, releaseString.length()+1);
        Serial.println("PH UP Release Count:");
        Serial.println(counter_ph_down);
        client.publish("sensor/phdown/counter",counter_ph_down); 
 

        digitalWrite(PHdown, HIGH);
        delay(PHdown_delay);
        digitalWrite(PHdown, LOW);
        delay(1000);
      }
  }

//////////////////////////////////////////
// PH READING FUNCTION ///////////////////
//////////////////////////////////////////

float readPH()
{
  client.loop();
  Wire.beginTransmission(PHaddress);      //call PHsensor by its ID number.
  Wire.write("R");                        //transmit the "R" command to PH sensor to take a reading
  Wire.endTransmission();                 //end the I2C data transmission.

  delay(time_);                           //wait the correct amount of time for the PH sensor to complete its instruction.

  Wire.requestFrom(PHaddress, 20, 1);     //call the circuit and request 20 bytes (this may be more than we need)
  code = Wire.read();                     //the first byte is the response code, we read this separately.

  while (Wire.available()) {         //are there bytes to receive.
    in_char = Wire.read();           //receive a byte.
    ph_data[i] = in_char;            //load this byte into our array.
    i += 1;                          //incur the counter for the array element.
    if (in_char == 0) {              //if we see that we have been sent a null command.
      i = 0;                         //reset the counter i to 0.
      Wire.endTransmission();        //end the I2C data transmission.
      break;                         //exit the while loop.
    }
  }
  ph_float = atof(ph_data);
  return ph_float;
}


////////////////////////////
//EC READING FUNCTION //////
////////////////////////////

int readEC()
{
  client.loop();
  Wire.beginTransmission(ECaddress);          //call the EC sensor by it's ID number
  Wire.write("R");                            //Send "R" command to EC sensor to take a reaing
  Wire.endTransmission();                     //end the I2C data transmission.

  delay(delay_time);                          //wait the correct amount of time for the circuit to complete its instruction.

  Wire.requestFrom(ECaddress, 48, 1);         //call the EC circuit and request 48 bytes (this is more than we need)
  code = Wire.read();                         //the first byte is the response code, we read this separately.

  while (Wire.available()) {                  //are there bytes to receive.
    in_char = Wire.read();                    //receive a byte.
    ec_data[i] = in_char;                     //load this byte into our array.
    i += 1;                                   //incur the counter for the array element.
    if (in_char == 0) {                       //if we see that we have been sent a null command.
      i = 0;                                  //reset the counter i to 0.
      Wire.endTransmission();                 //end the I2C data transmission.
      break;                                  //exit the while loop.
    }
  }

  //the below function will break up the CSV string into its 4 individual parts. EC|TDS|SAL|SG.
  //this is done using the C command “strtok”.

  ec = strtok(ec_data, ",");          //let's pars the string at each comma.
  tds = strtok(NULL, ",");            //let's pars the string at each comma.
  sal = strtok(NULL, ",");            //let's pars the string at each comma.
  sg = strtok(NULL, ",");             //let's pars the string at each comma.

  // The below is commented out because we are only interested in the EC value.
  // It is left in this code, however, because it may be useful in future additions

  //Serial.print("EC:");                //we now print each value we parsed separately.

  //Serial.println(ec);                 //this is the EC value.

  //Serial.print("TDS:");               //we now print each value we parsed separately.
  //Serial.println(tds);                //this is the TDS value.

  //Serial.print("SAL:");               //we now print each value we parsed separately.
  //Serial.println(sal);                //this is the salinity value.

  //Serial.print("SG:");               //we now print each value we parsed separately.
  //Serial.println(sg);                //this is the specific gravity.

  ec_float = atof(ec);
  //tds_float = atof(tds);
  //sal_float = atof(sal);
  //sg_float = atof(sg);

  return ec_float;
}

/////////////////////////////////////////
// SETUP FUNCTION ///////////////////////
/////////////////////////////////////////

void setup()                            
{
  Serial.begin(9600);                   //enable serial port.
  Wire.begin();                         //enable I2C port.
  delay(3000);
  pinMode(PHup, OUTPUT);                //Initialize all pins as outputs
  digitalWrite(PHup, LOW);              //and low so there aren't any
  pinMode(PHdown, OUTPUT);              //accidental dosings when power is
  digitalWrite(PHdown, LOW);            //first applies
  pinMode(Nutrients, OUTPUT);
  digitalWrite(Nutrients, LOW);
  setup_wifi();                         //run wifi setup function
  delay(1000);
  client.setServer(mqtt_server, 1883);  //Set MQTT server to port 1883
  delay(1000);
}

void loop() {
  char msg[10];
   if (!client.connected()) {   //Reconnect if MQTT disconnects
    reconnect();
  }
  client.loop();
  delay(1000);
  
  // Although system reports a PH reading after every 3 EC readings,
  // Code does NOT move to validate the PH reading for accuracy (or
  // dose PH solution) until the EC reading has reached the appropriate level.
  
  do
  {
    Serial.println ("Measuring EC....");
    EC_1 = readEC();
    Serial.println(EC_1);
    delay(3000);
    EC_2 = readEC();
    Serial.println(EC_2);
    delay(3000);
    EC_3 = readEC();
    Serial.println(EC_3);
    delay(3000);

    PH_1 = readPH();
    Serial.println("PH NOT VALIDATED Value:");
    Serial.println(PH_1);

    String PHString = String(PH_1);
    PHString.toCharArray(msg, PHString.length()+1);
    client.publish("sensor/ph/status","NOT VALIDATED");
    client.publish("sensor/ph/value", msg);
    delay(1000);
    
    diff1 = EC_1 - EC_2;
    diff2 = EC_2 - EC_3;
    diff3 = EC_3 - EC_1;


    Serial.println("The differences between values are");
    Serial.println(fabs(diff1));
    delay(1000);
    Serial.println(fabs(diff2));
    delay(1000);
    Serial.println(fabs(diff3));
    delay(1000);

  }
  while ((fabs(diff1) > max_diffEC) || (fabs(diff2) > max_diffEC) || (fabs(diff3) > max_diffEC) || (EC_1 == 0.0) || (EC_2 == 0.0) || (EC_3 == 0.0));

  Serial.println("EC Reading is ACCURATE...");
  Serial.println(EC_3);

  if (EC_3 <= ECmin) {
    client.loop();
    
    Serial.print("EC Reading is LOW: ");
    Serial.println(EC_3);
    Serial.println("Adding Nutrient Solution...");
          
    String ECString = String(EC_3);
    ECString.toCharArray(msg, ECString.length()+1); 
    client.publish("sensor/ec/status","LOW");
    client.publish("sensor/ec/value", msg);

////////////////////////////////////////////////////////////////
//The below code block counts the number of doses since power up
//and sends them via MQTT for display
////////////////////////////////////////////////////////////////

    char counter_nutrients[4];
    release_nutrients_counter = release_nutrients_counter+1;
    String releaseString = String(release_nutrients_counter);
    releaseString.toCharArray(counter_nutrients, releaseString.length()+1);
    Serial.println("Nutrients Release Count:");
    Serial.println(counter_nutrients);
    client.publish("sensor/nutrients/counter",counter_nutrients); 
////////////////////////////////////////////////////////////////////
    
    digitalWrite(Nutrients, HIGH);
    delay(Nutrients_delay);
    digitalWrite(Nutrients, LOW);
     
  }
  else if ((EC_3 > ECmin) && (EC_3 < ECmax)) {
    Serial.println("EC is IN RANGE");
    Serial.print("Publish message: EC Reading is ");
    Serial.println(EC_3);
    String ECString = String(EC_3);
    ECString.toCharArray(msg, ECString.length()+1); 
    client.publish("sensor/ec/status","IN RANGE");
    client.publish("sensor/ec/value", msg);
    delay(1000);

    PHloop();
    
  }
  else {
    Serial.println("EC is HIGH");
    Serial.println(EC_3);
    String ECString = String(EC_3);
    ECString.toCharArray(msg, ECString.length()+1); 
    client.publish("sensor/ec/status","HIGH");
    client.publish("sensor/ec/value", msg);

    PHloop();
    
  }
}






