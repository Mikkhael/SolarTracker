#include <Arduino.h>

#include "WiFi.h"
#include "WiFiMulti.h"

#include "Commander.h"
#include "HTTP.h"

#include "Time.h"

void sendLogsToWSS(const char* buf, bool newline){
    wss.broadcastLogs(buf, newline);
}

WiFiMulti wifiMutli;


unsigned long long lastTimer = 0;

static bool PROFILE(char id, unsigned long long limit = 100){
    int diff = millis() - lastTimer;
    if(diff >= limit){
        logln("PROFILE %c %d", id, diff);
        lastTimer = millis();
        return false;
    }
    lastTimer = millis();
    return true;
}


bool blink = true;

void setup()
{
    pinMode(2, OUTPUT);
    digitalWrite(2, blink);
    Serial.begin(115200);
    delay(100);
    // SerialBT.begin("ESP32SOLAR");
    // delay(100);

    preferanceEntries.load();

    hmotor.initAsH();
    positionCalculatorH.initAsH();

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

    ntc.setUpdateInterval(config.ntcUpdateInterval);
    ntc.begin();

    lastTimer = millis();
}

bool firstNtcSet = false;

const int next_blink_interval = 3000;
int next_blink = next_blink_interval;

void loop()
{

    if(millis() > next_blink ) {
        blink = !blink;
        digitalWrite(2, blink);
        next_blink = millis() + next_blink_interval;
    }



    PROFILE('0');
    //config.print();

    // Reading commands from Serial
    if(Serial.available()){
        String cmd = Serial.readStringUntil('\n');
        if(Serial.peek() == '\n')
            Serial.read();
        commander.parseAndExecute(cmd);
    }
    
    PROFILE('c');
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
    PROFILE('t');

    if(controller.isSafeToInterrupt()){
        if(ntc.update() && !firstNtcSet){
            logln("Setting time from NTC to %d (%s)", ntc.getEpochTime(), ntc.getFormattedTime().c_str());
            firstNtcSet = true;
        }
    }
    PROFILE('n');

    controller.loop();
    PROFILE('C');

    http.handleClient();
    PROFILE('h');
    wss.loop();
    PROFILE('w');



    //delay(200);
}