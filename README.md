# automated-hydroponics
An effort towards an automated hydroponic system.

This system was created for a senior project in mechanical engineering.  Three seperate Wemos (ESP8266) communicate via MQTT to a Mosquitto broker running on a Raspberry Pi.  Node-Red was used on the Pi to create a quick/effecient user interface.

The three seperate Wemos/ESP8266 are responsible for the following:

1. CO2 Measurement and Control
2. PH/EC Measurement and Control (Dosing Pumps)
3. Sump Tank Water Level (Broken-Finger Switches)

# CO2 Measurment and Control

Components
1. A K30 CO2 sensor | https://www.co2meter.com/products/k-30-co2-sensor-module
2. A 5V relay | https://www.amazon.com/gp/product/B01ICYMF08/ref=oh_aui_detailpage_o01_s00?ie=UTF8&psc=1
3. A 12V solenoid | *NEED TO ADD LINK*
4. Safety Buzzer | *NEED TO ADD LINK*

# PH/EC Measurment and Control (Dosing Pumps)

Components
1. Atlas Scientific EC probe/circuit | https://www.atlas-scientific.com/conductivity.html
3. Atlas Scientific ELECTRICAL ISOLATOR | https://www.atlas-scientific.com/product_pages/circuits/pwr-iso.html
2. Atlas Scientific PH probe/circuit | https://www.atlas-scientific.com/ph.html
3. Three Perisaltic Pumps | https://www.amazon.com/Homecube-Peristaltic-Miniature-Aquarium-Analytical/dp/B0196LDFJQ/ref=sr_1_sc_1?ie=UTF8&qid=1491888964&sr=8-1-spell&keywords=perisaltic+pump

# Sump Tank Water Level

Components
1. Three Broken Finger Switches | https://www.amazon.com/URBEST-Controller-Vertical-Floating-Pressure/dp/B01LX627TC/ref=sr_1_7?ie=UTF8&qid=1491889220&sr=8-7&keywords=arduino+water+level

***WORK IN PROGRESS | INSTRUCTIONS AND UPDATED CODE TO FOLLOW***
[4/11/2017]


