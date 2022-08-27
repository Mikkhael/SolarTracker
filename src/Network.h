#pragma once

#include "Config.h"
#include "WiFiMulti.h"
#include "ESPmDNS.h"

struct Network{
    
    WiFiMulti wifiMulti;

    bool isInited = false;

    void init(){
        logln("Initializing trusted APs:");
        for(int i=0; i<config.TrustedAPsCount; i++){
            if(config.SSIDs[i] != ""){
                logln(config.SSIDs[i].c_str());
                if(config.PASSs[i] != ""){
                    wifiMulti.addAP(config.SSIDs[i].c_str(), config.PASSs[i].c_str());
                }else{
                    wifiMulti.addAP(config.SSIDs[i].c_str());
                }
            }
        }
        isInited = true;
    }

    auto getStatus(){
        return WiFi.status();
    }
    bool isConnected(){
        return WiFi.status() == WL_CONNECTED;
    }

    bool begin(){
        if(!isInited) init();
        logln("Connecting to WiFi (t=%d)...", config.wifiConnectionTimeout);
        auto status = wifiMulti.run(config.wifiConnectionTimeout);
        if(status == WL_CONNECTED){
            logln("WiFi Connected.");
            return true;
        }
        logln("WiFi Not Connected.");
        return false;
    }

    bool end(){
        return WiFi.disconnect();
    }

    void printFullConnectionStatus(){
        logln("STATUS: %d", WiFi.status());
        logln("SSID: %s", WiFi.SSID().c_str());
        logln("IP: %s", WiFi.localIP().toString().c_str());
        logln("SUBNET: %s", WiFi.subnetMask().toString().c_str());
        logln("GATEWAY: %s", WiFi.gatewayIP().toString().c_str());
    }

};


inline Network network;