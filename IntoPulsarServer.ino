/*
   Author: Benjamin Low

   Date: August 2016

   Description:

       Code for an Espresso Lite ver 2 ESP8266 board.

       For a Night Festival 2016 installation at the National Museum of Singapore
       called 'Into Pulsar' by artist Ryf Zaini.

       Three bicycles placed at the bottom of a 'tree'. User cycles any of these to trigger the
       28 x ebike wheels on top of the tree to go faster to show a pov image on the
       4 led strips placed in a "+" pattern on the wheels.

       This code is a WiFi server reading from three hall sensors and sends out the max
       analog value to the 28 Espresso clients for the 28 ebike wheels.

       Reference for setting a static server IP address:
            https://github.com/esp8266/Arduino/issues/1959
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define hallPin1 14
#define hallPin2 12
#define hallPin3 13

#define MAXRPM 100 //max speed of use bicycle needed to trigger max speed for pov
#define MINRPM 20 //min speed below which will read as an off signal

int currVal[3], prevVal[3], counter[3];
int RPM[3], myMaxRPM, myPrevMaxRPM;
long isTriggeredTime[3], prevReadTime[3];
long oneRevTimeInterval[3], timeInterval[3];

const char* ssid = "IntoPulsar";
const char* password = "ryfzaini";
IPAddress ip(192, 168, 1, 177);
IPAddress ipMulti(239, 0, 0, 57);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
unsigned int portMulti = 12345;

WiFiUDP udp;

void setup() {

  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to");
  Serial.print(ssid);

  WiFi.config(ip, gateway, subnet); //set static IP address
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(hallPin1, INPUT_PULLUP);
  pinMode(hallPin2, INPUT_PULLUP);
  pinMode(hallPin3, INPUT_PULLUP);
}

void loop() {

  for (int i = 0; i < 3; i++) {
    read_sensor(i);
    calc_RPM(i);
  }

  get_myMaxRPM();

  do_udp_multicast();

  delay(5); //need high delay for serial monitor. Otherwise no need for actual operation
}

void read_sensor(int i) {

  currVal[i] = digitalRead(i + 12);

  if (currVal[i] == LOW && prevVal[i] == HIGH) { //positive hall detection

    counter[i]++;
    isTriggeredTime[i] = millis();

    //    Serial.print("counter ");
    //    Serial.print(i + 12);
    //    Serial.print(": ");
    //    Serial.println(counter[i]);
  }

  prevVal[i] = currVal[i];
}

void calc_RPM(int i) {

  timeInterval[i] = millis() - prevReadTime[i]; //independent of whether triggered or not

  oneRevTimeInterval[i] = isTriggeredTime[i] - prevReadTime[i];

  prevReadTime[i] = isTriggeredTime[i];

  if (oneRevTimeInterval[i] > 0) {

    RPM[i] = int( 60000.0 / oneRevTimeInterval[i]);

  }

  if (timeInterval[i] > 5000) {
    /*
       ideally the bike wheel should slow down instead of abruptly stopping,
       but just in case
    */
    RPM[i] = 0;
  }

}

void get_myMaxRPM() {

  //    Serial.print("RPM ");
  //    for (int i=0; i<3; i++) {
  //        Serial.print(RPM[i]);
  //        Serial.print(" ");
  //    }
  //    Serial.println();

  if ( RPM[0] > RPM[1] ) {
    myMaxRPM = RPM[0];
  } else {
    myMaxRPM = RPM[1];
  }

  if (myMaxRPM < RPM[2]) {
    myMaxRPM = RPM[2];
  }

  //    Serial.print("maxVal: ");
  //    Serial.println(maxVal);

  myMaxRPM = constrain(myMaxRPM, MINRPM, MAXRPM);
  myMaxRPM = map(myMaxRPM, MINRPM, MAXRPM, 600, 300); //600 is analog value for the motor controller for cruise speed, 300 for max speed
}

void do_udp_multicast() {
  if (myMaxRPM != myPrevMaxRPM) {
    for (int i = 0; i < 3; i++) { //send a few times in case of lost packets
      udp.beginPacketMulticast(ipMulti, portMulti, WiFi.localIP());
      udp.write((unsigned char *) &myMaxRPM, sizeof(myMaxRPM));
      udp.endPacket();
    }
  }
  myPrevMaxRPM = myMaxRPM;
}

