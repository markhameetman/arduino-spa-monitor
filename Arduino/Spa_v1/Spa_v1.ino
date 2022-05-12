#include <SPI.h>
#include <Ethernet.h>
#include "ph_grav.h"             
Gravity_pH pH = Gravity_pH(A0);  
#include "rtd_grav.h"
Gravity_RTD RTD = Gravity_RTD(A1);
#include "orp_grav.h"
Gravity_ORP ORP = Gravity_ORP(A2);

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 1, 80, 200);
EthernetServer server(80);

                

uint8_t user_bytes_received = 0;                
const uint8_t bufferlen = 32;                   
char user_data[bufferlen];                     

void parse_cmd(char* string) {                   
  strupr(string);                                
  if (strcmp(string, "CAL,7") == 0) {       
    pH.cal_mid();                                
    Serial.println("MID CALIBRATED");
  }
  else if (strcmp(string, "CAL,4") == 0) {            
    pH.cal_low();                                
    Serial.println("LOW CALIBRATED");
  }
  else if (strcmp(string, "CAL,10") == 0) {      
    pH.cal_high();                               
    Serial.println("HIGH CALIBRATED");
  }
  else if (strcmp(string, "CAL,CLEAR") == 0) { 
    pH.cal_clear();                              
    Serial.println("CALIBRATION CLEARED");
  }
  String cmd = String(string);
  if(cmd.startsWith("ORP")){
    int index = cmd.indexOf(',');
    if(index != -1){
      String param = cmd.substring(index+1, cmd.length());
      if(param.equals("CLEAR")){
        ORP.cal_clear();
        Serial.println("ORP CALIBRATION CLEARED");
      }
      else{
        int cal_param = param.toInt();
        ORP.cal(cal_param);
        Serial.println("ORP CALIBRATED");
      }
    }
  }
  if(cmd.startsWith("RTD")){
    int index = cmd.indexOf(',');
    if(index != -1){
      String param = cmd.substring(index+1, cmd.length());
      if(param.equals("CLEAR")){
        RTD.cal_clear();
        Serial.println("RTD CALIBRATION CLEARED");
      }else {
        RTD.cal(param.toFloat());
        Serial.println("RTD CALIBRATED");
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {
  if (Serial.available() > 0) {
    user_bytes_received = Serial.readBytesUntil(13, user_data, sizeof(user_data));
  }

  if (user_bytes_received) {
    parse_cmd(user_data);
    user_bytes_received = 0;
    memset(user_data, 0, sizeof(user_data));
  }

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        String json = "{ \"ph\": " + String(pH.read_ph()) + ", \"temperature\":" + String(RTD.read_RTD_temp_C()) + ", \"orp\": " + String((int) ORP.read_orp()) + " }";
        
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
            Serial.println((int)ORP.read_orp());
            client.println(json);
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
    Serial.println("client disconnected");
  }
}
