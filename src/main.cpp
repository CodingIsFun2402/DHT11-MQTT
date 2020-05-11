// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor


//****************************************************
// Naechste Schritte
// Sichern in GITHUB
// Ausgabe Seriell, und Web trennen / strukturieren
// Einbau MQTT
//****************************************************


#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>


#define USE_MQTT
// #define USE_SERIAL
//#define USE_WEB

//DHT11
#define DHTPIN    8     // Digital pin connected to the DHT sensor 
#define DHTTYPE   DHT11     // DHT 11

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

//Webserver
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
// 90 A2 DA 10 21 22
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x10, 0x21, 0x22
};
EthernetServer server(80);
EthernetClient client;


// MQTT Configuration
const char* mqtt_server = "192.168.1.101";
const unsigned int mqtt_port = 1883;
const char* mqtt_user = "HomeMQTT";
const char* mqtt_pass = "3ebc2b3045";
const char* mqtt_root = "/mqtt/Arduino/DHT11/";

void reportSetupToWeb() {
  #ifdef USE_SERIAL
      Serial.println("Starting Ethernet Connection Step 1");
      while (!Serial) {
        Serial.println("Waiting for Ethernet Connection..."); // wait for serial port to connect. Needed for native USB port only
      }
      Serial.println("Starting Ethernet Connection Step 2");
    #endif

    if (Ethernet.begin(mac) == 0) {
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        #ifdef USE_SERIAL
          Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        #endif
      } else if (Ethernet.linkStatus() == LinkOFF) {
        #ifdef USE_SERIAL
          Serial.println("Ethernet cable is not connected.");
        #endif
      }
      // no point in carrying on, so do nothing forevermore:
      while (true) {
        delay(1);
      }
    } else {
      #ifdef USE_SERIAL
      Serial.println("Success to configure Ethernet using DHCP");
        // print your local IP address:
        Serial.print("My IP address: ");
        Serial.println(Ethernet.localIP());
      #endif
    }
  
}



void reportSetupToSerial(sensor_t iSensor, String sensorType) {
  if (sensorType == "Temperature") {
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(iSensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(iSensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(iSensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(iSensor.max_value); Serial.println(F("°C"));
    Serial.print  (F("Min Value:   ")); Serial.print(iSensor.min_value); Serial.println(F("°C"));
    Serial.print  (F("Resolution:  ")); Serial.print(iSensor.resolution); Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));
  }

  if (sensorType == "Humidity") {
    Serial.println(F("Humidity Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(iSensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(iSensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(iSensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(iSensor.max_value); Serial.println(F("%"));
    Serial.print  (F("Min Value:   ")); Serial.print(iSensor.min_value); Serial.println(F("%"));
    Serial.print  (F("Resolution:  ")); Serial.print(iSensor.resolution); Serial.println(F("%"));
    Serial.println(F("------------------------------------"));

  }
}

void reportSensorDataToWeb(sensors_event_t temp, sensors_event_t humid) {
  char msg[100]; // Buffer
  // Temperatur
  static char floatStr[15];
  String payLoad;
  String sensorString;
  float  sensorFloat;

  switch (Ethernet.maintain()) {
      case 1:
        //renewed fail
        #ifdef USE_SERIAL
          Serial.println("Error: renewed fail");
        #endif
        break;

      case 2:
        //renewed success
        #ifdef USE_SERIAL
          Serial.println("Renewed success");
          //print your local IP address:
          Serial.print("My IP address: ");
          Serial.println(Ethernet.localIP());
        #endif
        break;

      case 3:
        //rebind fail
        #ifdef USE_SERIAL
          Serial.println("Error: rebind fail");
        #endif
        break;

      case 4:
        //rebind success
        #ifdef USE_SERIAL
          Serial.println("Rebind success");
          //print your local IP address:
          Serial.print("My IP address: ");
          Serial.println(Ethernet.localIP());
        #endif
        break;

      default:
        //nothing happened
        break;
    }
  
  // listen for incoming clients
  EthernetClient client = server.available();
  // if (client) {
  //   #ifdef USE_SERIAL
  //     Serial.println("new client");
  //   #endif

    // an http request ends with a blank line
    #ifdef USE_WEB
      boolean currentLineIsBlank = true;
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          
          // if you've gotten to the end of the line (received a newline
          // character) and the line is blank, the http request has ended,
          // so you can send a reply
          if (c == '\n' && currentLineIsBlank) {
              // send a standard http response header
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: text/html");
              client.println("Connection: close");
              client.println();
              client.println("<!DOCTYPE HTML>");
              client.println("<html>");

              client.print("Temperatur: ");
              client.print(temp.temperature);
              client.print(" &deg;C");
              client.println("<br />"); 
            
              client.print("Luftfeuchtigkeit: ");
              client.print(humid.relative_humidity);
              client.print(" %");
              client.println("<br />");      
              
              client.println("</html>");
          }
          if (c == '\n') {
            // you're starting a new line
            currentLineIsBlank = true;
          } 
          else if (c != '\r') {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
        }
      }
      // give the web browser time to receive the data
      delay(1);
      // close the connection:
      client.stop();
      #ifdef USE_SERIAL
        Serial.println("client disonnected");
      #endif
    #endif

    #ifdef USE_MQTT
      PubSubClient pubSubClient(mqtt_server, mqtt_port, client);
      if (pubSubClient.connect("arduinoClient", mqtt_user, mqtt_pass)) {
        sensorFloat = temp.temperature;               // Messwert holen
        dtostrf( sensorFloat,7, 2, floatStr);         // Zahlenwert in String wandeln
        sensorString = floatStr;
        sensorString.trim();
        payLoad = "{\"Temperatur\":\"";                   // Nachricht zusammenbauen
        payLoad = payLoad + sensorString + "\"}";     // Messwert dazufügen
        payLoad.toCharArray(msg, 100 ); 
        
      	boolean mTempArrived = pubSubClient.publish("/mqtt/Arduino/DHT11/temperature", msg);
        #ifdef USE_SERIAL
          if (mTempArrived) {
            Serial.println("MQTT Message Temperatur erfolgreich");
          } else {
            Serial.println("MQTT Message Temperatur nicht erfolgreich");
          }
        #endif

        sensorFloat = humid.relative_humidity;         // Messwert holen
        dtostrf( sensorFloat,7, 2, floatStr);         // Zahlenwert in String wandeln
        sensorString = floatStr;
        sensorString.trim();
        payLoad = "{\"Luftfeuchte\":\"";                   // Nachricht zusammenbauen
        payLoad = payLoad + sensorString + "\"}";     // Messwert dazufügen
        sensorString.toCharArray(msg, 100 ); 
        boolean miArrived = pubSubClient.publish("/mqtt/Arduino/DHT11/humidity", msg);
        #ifdef USE_SERIAL
          if (mHumiArrived) {
            Serial.println("MQTT Message Luftfeuchte erfolgreich");
          } else {
            Serial.println("MQTT Message Luftfeuchte nicht erfolgreich");
          }
        #endif
      }
    #endif
  // }
}


void reportSensorDataToSerial(sensors_event_t temp, sensors_event_t humid) {
  Serial.print(F("Temperatur: "));
  Serial.print(temp.temperature);
  Serial.println(F(" °C"));

  Serial.print(F("Luftfeuchtigkeit: "));
  Serial.print(humid.relative_humidity);
  Serial.println(F(" %"));  
}

void setup() {

  
  // Initialize device.
  dht.begin();
  
  // Print temperature sensor details.
  sensor_t sensor;
  
  dht.temperature().getSensor(&sensor);
  #ifdef USE_SERIAL
    Serial.begin(9600);
    reportSetupToSerial(sensor, "Temperature");
  #endif
  // Print humidity sensor details.

  dht.humidity().getSensor(&sensor);
  #ifdef USE_SERIAL
    reportSetupToSerial(sensor, "Humidity");
  #endif
  // Set delay between sensor readings based on sensor details.

  delayMS = sensor.min_delay / 1000 * 20; //entspricht 10 Sekunden
  #ifdef USE_MQTT
    reportSetupToWeb();
  #endif

}



void loop() {
  sensors_event_t event_temp;
  sensors_event_t event_humid;
  // Get temperature event and print its value.
  dht.temperature().getEvent(&event_temp);
  dht.humidity().getEvent(&event_humid);
  if (isnan(event_temp.temperature) || isnan(event_humid.relative_humidity)) {
    #ifdef USE_SERIAL
      Serial.print("Error reading temperature!" );
      Serial.println(event_temp.temperature);
      Serial.print("Error reading humidity!");
      Serial.println(event_humid.relative_humidity);
    #endif
  } else {
    #ifdef USE_SERIAL
      reportSensorDataToSerial(event_temp, event_humid);
    #endif
    
    reportSensorDataToWeb(event_temp, event_humid);
    
  }
  delay(delayMS);
}

