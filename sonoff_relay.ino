/*
   1MB flash size

   Sonoff Relay serial header
   1 - vcc 3v3
   2 - rx
   3 - tx
   4 - gnd
   5 - gpio 14

   ESP8266 pin connections on relay board
   gpio  0 - button
   gpio 12 - relay (Operates from 12v off line power, not programmer power.)
   gpio 13 - green led - active low
   gpio 14 - pin 5 on header

*/

boolean debug = false;  //  true; // Set true for serial debug output

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

/************************* Relay Node Configuration ***************************/

#define WLAN_SSID       "xxxxxxxx"                  // WiFi ssid to connect
#define WLAN_PASS       "xxxxxxxxxxxxxxxxxxxxxxx"   // WiFi password

char thisLight[] = "frogFrontWindow";          // Light to be controlled

char pubTopic[] = "openhab/status/relays";    // Topic to receive from this light
char subTopic[] = "openhab/xmaslgts/relay";   // Topic to control this light

char thisLightOn[25];
char thisLightOff[25];
char checkin[35];
void callback(char* topic, byte* payload, unsigned int length);
void toggle(void);
volatile byte state = LOW;
byte lastState = LOW;

#define MQTT_SERVER "192.168.1.5"  // URL or IP address of MQTT broker

/************************* relay board config *********************************/
#define buttonPin 0       // Button pin on board
#define relayPin 12       // Pin number of relay
#define LEDpin  13        // LED on board
char message_buff[100];   // Message buffer for the relay command


// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient wlClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wlClient);


/***************** Setup function *************************/
void setup() {
  if (debug) { delay(1000); Serial.begin(115200); } //start serial commms for debugging 

  // Variables for various messages and functions
  strcat(thisLightOn, thisLight);    // Set up string for specific light on
  strcat(thisLightOn, "On");

  strcat(thisLightOff, thisLight);   // Set up string for specific light off
  strcat(thisLightOff, "Off");

  strcat(checkin, thisLight);        // Set up string for check-in message
  strcat(checkin, " checking in!");

      if (debug) { Serial.println(); Serial.println(); 
        Serial.println(thisLightOn); Serial.println(thisLightOff); }

  // Set up pins for relay, LED and button
  pinMode(relayPin, OUTPUT);
  pinMode(LEDpin, OUTPUT);
  pinMode(buttonPin, INPUT);
  
  delay(10);
  digitalWrite(relayPin, LOW);  // Relay is OFF
  digitalWrite(LEDpin, LOW);    // LED is ON
  
// Connect to WiFi access point.
      if (debug) { Serial.println(); Serial.println();
        Serial.print("Connecting to ");
        Serial.println(WLAN_SSID); }
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
      if (debug) { Serial.print("."); }
  }
      if (debug) { Serial.println();
        Serial.println("WiFi connected");
        Serial.println("IP address: "); Serial.println(WiFi.localIP()); }
  mqtt_connect();
      if (debug) { Serial.print(thisLight); Serial.println(F(" Relay Ready!")); }  

  // Attach interrupt to pin 0 to check for button press
  attachInterrupt(digitalPinToInterrupt(buttonPin), toggle, FALLING);
} // End setup()


/************** Main loop function *****************/
void loop() {
  
// Reconnect of connection lost.
   if (!client.connected()) {
    mqtt_connect();
  }

// Handle button input
  if (state != lastState){
    digitalWrite(relayPin, !digitalRead(relayPin));
    digitalWrite(LEDpin, !digitalRead(LEDpin));
    if (digitalRead(relayPin) == HIGH ) {
    client.publish(pubTopic, thisLightOn);
    }
    else {
    client.publish(pubTopic, thisLightOff);      
    }
  lastState = state;
  }
  
 // Handle MQTT work
  client.loop();
} // End main loop()


/****************** MQTT callback to handle incoming messages ******************/
void callback(char* topic, byte* payload, unsigned int length) {
  // MQTT inbound Messaging 
  int i = 0;
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  String msgString = String(message_buff);
  if (debug) { Serial.println("Inbound: " + String(topic) +":"+ msgString); }
  
  //Activate relay on or off based on message received
  if ( msgString == "LightsOn" || msgString == thisLightOn ) {
     digitalWrite(relayPin,HIGH);
     digitalWrite(LEDpin,LOW);
     client.publish(pubTopic, thisLightOn);
    }
  if ( msgString == "LightsOff" || msgString == thisLightOff ) {
     digitalWrite(relayPin,LOW);
     digitalWrite(LEDpin,HIGH);
     client.publish(pubTopic,thisLightOff);
    }
}


/*********************** Connect to MQTT server ***********************/
void mqtt_connect(){
    // Loop until we're reconnected

      if (debug) { Serial.print("Length thisLight: "); Serial.println(strlen(thisLight));
         Serial.print("Length checking in:"); Serial.println(strlen(" checking in!\0"));
         Serial.print("Length thisLight + checking in:"); Serial.println(strlen(" checking in!\0") + strlen(thisLight));
         Serial.print("Length checkin:"); Serial.println(strlen(checkin));
         Serial.print("checkin = "); Serial.println(checkin); } 
    
  while (!client.connected()) {
      if (debug) { Serial.print("Attempting MQTT connection..."); }
     // Attempt to connect
    if (client.connect(thisLight)) {   
      if (debug) { Serial.println("connected"); }
      // Once connected, publish an announcement...
      client.publish(pubTopic,checkin);
      client.subscribe(subTopic);      
    } else {
      if (debug) { Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds"); }
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


/**************** Interrupt service function on button press **************/
void toggle(void){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  
  if (interrupt_time - last_interrupt_time > 200)
  {
    state = !state;
  }
  last_interrupt_time = interrupt_time;
}

