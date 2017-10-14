
/*
 Sketch which publishes temperature data from a DS1820 sensor to a MQTT topic.
 This sketch goes in deep sleep mode once the temperature has been sent to the MQTT
 topic and wakes up periodically (configure SLEEP_DELAY_IN_SECONDS accordingly).
 Hookup guide:
 - connect D0 pin to RST pin in order to enable the ESP8266 to wake up periodically
 - DS18B20:
     + connect VCC (3.3V) to the appropriate DS18B20 pin (VDD)
     + connect GND to the appopriate DS18B20 pin (GND)
     + connect D4 to the DS18B20 data pin (DQ)
     + connect a 4.7K resistor between DQ and VCC.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <Streaming.h>

#define SLEEP_DELAY_IN_SECONDS            1200   //20 minutes
#define ONE_WIRE_BUS_NorthFace            5      // DS18B20 pin GPIO5
#define ONE_WIRE_BUS_SouthFace            4      // DS18B20 pin GPIO4
#define ONE_WIRE_BUS_DirtSouth            12      // DS18B20 pin GPIO12
#define ONE_WIRE_BUS_DirtNorth            14      // DS18B20 pin GPIO14
#define ONE_WIRE_BUS_PWR                  13      // DS18B20 pin GPIO13

#define DS18B20_PWR_ON                digitalWrite(ONE_WIRE_BUS_PWR, HIGH);


const char* mqtt_server = "192.168.1.44";
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* mqtt_topic = "/dev/esp_gh01CementBed/Out/tempC";

WiFiClient espClient;
PubSubClient client(espClient);

OneWire oneWireNorthFace(ONE_WIRE_BUS_NorthFace);
OneWire oneWireSouthFace(ONE_WIRE_BUS_SouthFace);
OneWire oneWireDirtSouth(ONE_WIRE_BUS_DirtSouth);
OneWire oneWireDirtNorth(ONE_WIRE_BUS_DirtNorth);
//OneWire oneWire5(D4);
//OneWire oneWire6(15);

DallasTemperature DS18B20NorthFace(&oneWireNorthFace);
DallasTemperature DS18B20SouthFace(&oneWireSouthFace); 
DallasTemperature DS18B20DirtSouth(&oneWireDirtSouth); 
DallasTemperature DS18B20DirtNorth(&oneWireDirtNorth); 

void DS18B20_PWR(unsigned int pwr){
  digitalWrite(ONE_WIRE_BUS_PWR, pwr);
  delay(500); //wait .5 second to let them power up
}

void setup() {
  // setup serial port
  Serial.begin(74880);
  pinMode(ONE_WIRE_BUS_PWR, OUTPUT);  
  
  DS18B20_PWR(HIGH);
  
  
  // setup WiFi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // setup OneWire bus
  DS18B20NorthFace.begin();
  DS18B20SouthFace.begin();
  DS18B20DirtSouth.begin();
  DS18B20DirtNorth.begin();
  DS18B20_PWR(LOW);
  
}

void setup_wifi() {
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //WiFi.persistent(false);

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
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

int getNumberOfDevices(){
  return DS18B20NorthFace.getDeviceCount();
}
float getTemperature(DallasTemperature *DS18B20) {
  Serial.print("Requesting DS18B20 temperature...\n");
 // Serial.print("Devices: " + String(DS18B20NorthFace.getDeviceCount()) +"\n");

  float temp;
  //do {
    DS18B20->requestTemperatures(); 
    
    temp = DS18B20->getTempCByIndex(0);
 //   delay(100);
 // } while (temp == 85.0 || temp == (-127.0));
  return temp;
}

String CreateMQTTSensorMessage(){
  
  DS18B20_PWR(HIGH);
  delay(1000);
  
  char temperatureString[6];
  String message="";

  message +="{\n";
  message +="\"device\":\"esp8266_greenhouseCementBed\"\n";
  float temperature = getTemperature(&DS18B20NorthFace);
  // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temperature, 2, 2, temperatureString);
  message +="\"NorthFace\":"+String(temperatureString)+"\n";
  
   temperature = getTemperature(&DS18B20SouthFace);
  // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temperature, 2, 2, temperatureString);
  message +="\"SouthFace\":"+ String(temperatureString)+"\n";
  
  temperature = getTemperature(&DS18B20DirtNorth);
  // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temperature, 2, 2, temperatureString);
  message +="\"DirtNorth\":"+ String(temperatureString)+"\n";
  
  temperature = getTemperature(&DS18B20DirtSouth);
  // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temperature, 2, 2, temperatureString);
  message +="\"DirtSouth\":"+ String(temperatureString)+"\n";
  
  message +="}\n";
  
  DS18B20_PWR(LOW);
  
  return message;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  String message=CreateMQTTSensorMessage();
  
  // send temperature to the MQTT topic
  char charBuf[128];
  message.toCharArray(charBuf, message.length()) ;

  client.publish(mqtt_topic,charBuf);
  
  // send temperature to the serial console
  Serial.print("Sending temperature: \n" + String(charBuf)+"\n");

  Serial.print( "Closing MQTT connection...\n");
  client.disconnect();

 Serial.print("Closing WiFi connection...\n");
  WiFi.disconnect();

  delay(100);

  Serial.print("Entering deep sleep mode for "+String(SLEEP_DELAY_IN_SECONDS)+" seconds...");
  ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
  //ESP.deepSleep(10 * 1000, WAKE_NO_RFCAL);
  delay(500);   // wait for deep sleep to happen
}
