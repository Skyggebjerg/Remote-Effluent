
#include <Arduino.h>
#include "M5Atom.h"

#include "M5UnitHbridge.h"

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>


int n = 0; 
int ontime;
int forsink;

//int save_forsink; // forsink saved in EEPROM as int (forsink divided by 100)
uint64_t tempus;
int mstatus = 0; // defines which state the system is in

const char* ssid = "Effluent Pump 1"; //SSID of your network
const char* password = "12345678"; //Password of your WPA Network

WebServer server(80); //Server on port 80

M5UnitHbridge driver; // Create a driver object

void handleRoot() {
    String html = "<html><body style=\"font-size: 18px;\">";
    html += "<meta name=\"viewport\" content=\"width=390, initial-scale=1\"/>"; // Set the viewport to the width of the device and set the initial scale to 1
    html += "<h1 style=\"font-size: 24px;\">Motor Control Settings</h1>";
    html += "<form action=\"/update\" method=\"POST\">";
    html += "Ontime: <input type=\"text\" name=\"ontime\" value=\"" + String(ontime) + "\" style=\"font-size: 18px;\"><br>";
    html += "Forsink: <input type=\"text\" name=\"forsink\" value=\"" + String(forsink) + "\" style=\"font-size: 18px;\"><br>";
    html += "<input type=\"submit\" value=\"Save\" style=\"font-size: 18px;\">";
    html += "</form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleUpdate() {
    if (server.hasArg("ontime") && server.hasArg("forsink")) {
        ontime = server.arg("ontime").toInt();
        forsink = server.arg("forsink").toInt();

        EEPROM.put(0, ontime);
        EEPROM.put(sizeof(ontime), forsink);
        EEPROM.commit();

        server.send(200, "text/html", "<html><body><h1>Settings Saved</h1><a href=\"/\">Go Back</a></body></html>");
    } else {
        server.send(400, "text/html", "<html><body><h1>Invalid Input</h1><a href=\"/\">Go Back</a></body></html>");
    }
}


void setup() {
    Serial.begin(115200);
    EEPROM.begin(512);
    EEPROM.get(0, ontime);
    EEPROM.get(sizeof(ontime), forsink);
    WiFi.softAP(ssid, password);
    Serial.println("Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/update", HTTP_POST, handleUpdate);
    server.begin();
    Serial.println("HTTP server started");
    Wire.begin(26,32);
    M5.begin();
    driver.begin(&Wire, HBRIDGE_I2C_ADDR, 26, 32, 100000L);
    tempus = millis();
}

void loop() {
        server.handleClient(); //Handle client requests

        M5.update();
        if(M5.Btn.wasPressed())
        {
            mstatus = mstatus + 1;
            if (mstatus == 2) mstatus = 0;
        }
        
        if(n == 60) // defines number of runs before reverse (500)
        {
            mstatus = 3; // go to "reverse motor" case 3
            n = 0; // reset counter
        }

        // This loop runs the pump in 30ms bursts for i iterations with delays that are dependent on button press that toggles fast/slow
        
        switch (mstatus) {

        case 0: //run motor in experiment
        {
            if (millis() - tempus >= forsink) // to be set by adjustment (100)
            {
            n = n + 1; // increment counter and check if it is time to reverse
            driver.setDriverDirection(HBRIDGE_FORWARD); // Set peristaltic pump in forward to take out BR content
            driver.setDriverSpeed8Bits(128); //Run pump in half speed
            //Serial.print("Forward pump for 30 msecs #: ");
            //Serial.println(i);
            delay(ontime);
            driver.setDriverDirection(HBRIDGE_STOP);
            driver.setDriverSpeed8Bits(0);  //Stop pump 
            tempus = millis(); // Reset the timer
            }
        break;
        }

        
        case 1: //run motor fast
        {
            if (millis() - tempus >= 200) // to be set by adjustment (100)
            {
            //n = n + 1;
            driver.setDriverDirection(HBRIDGE_FORWARD); // Set peristaltic pump in forward to take out BR content
            driver.setDriverSpeed8Bits(128); //Run pump in half speed
            //Serial.print("Forward pump for 30 msecs #: ");
            //Serial.println(i);
            delay(ontime);
            driver.setDriverDirection(HBRIDGE_STOP);
            driver.setDriverSpeed8Bits(0);  //Stop pump 
            //tempus = millis(); // Reset the timer
            }
        break;
        }        

        case 3: //reverse motor
        {
        for(int i = 0; i < 16; i++){
                      //server.handleClient();
        driver.setDriverDirection(HBRIDGE_BACKWARD); // Set peristaltic pump in reverse to rinse 
        driver.setDriverSpeed8Bits(128);
        //Serial.println("Reverse pump for 30 msecs #: ");
        delay(30);
        driver.setDriverDirection(HBRIDGE_STOP);
        driver.setDriverSpeed8Bits(0);             
        delay(200); 
        }
            mstatus = 0;
            break;
        }
}
}