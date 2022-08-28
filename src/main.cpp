#include <Arduino.h>

#include "WiFi.h"
#include "WiFiMulti.h"

#include "Commander.h"
#include "HTTP.h"

WiFiMulti wifiMutli;

void setup()
{
    Serial.begin(115200);
    delay(100);

    preferanceEntries.load();

    network.begin();
    if(network.isConnected()){
        network.printFullConnectionStatus();
    }
    if(!MDNS.begin("solar")){
        logln("[ERROR] MDNS failed to start!!");
    };

    TelnetPrint.begin();
    http.begin();
    wss.begin();
}

void loop()
{
    //config.print();

    // Reading commands from Serial
    if(Serial.available()){
        String cmd = Serial.readStringUntil('\n');
        if(Serial.peek() == '\n')
            Serial.read();
        commander.parseAndExecute(cmd);
    }
    // Reading commands from Telnet
    while(true){
        auto telnetClient = TelnetPrint.available();
        if(!telnetClient) break;
        while(telnetClient.available()){
            String cmd = telnetClient.readStringUntil('\n');
            if(telnetClient.peek() == '\n')
                telnetClient.read();
            commander.parseAndExecute(cmd);
        }
    }

    http.handleClient();
    wss.loop();

    delay(200);
}