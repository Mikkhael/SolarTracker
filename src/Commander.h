#pragma once
#include "Network.h"


struct Commander{
    static constexpr int MaxArgs = 5;

    void parseAndExecute(String str){
        String args[MaxArgs];
        int current_arg = 0;
        for(int i=0; i<str.length(); i++){
            char c = str[i];
            if(c == ' '){
                current_arg++;
                continue;
            }
            args[current_arg] += c;
        }
        execute(args);
    }


    void execute(String args[MaxArgs]){
        if(args[0] == "help"){
            logln("HELP!!");
        }
        else if(args[0] == "cfg"){
            config.print();
        }
        else if(args[0] == "cfgsave"){
            preferanceEntries.save();
        }
        else if(args[0] == "cfgload"){
            preferanceEntries.load();
        }
        else if(args[0] == "ssid"){
            int i = args[1].toInt();
            logln("Setting Trusted SSID %d to %s", i, args[2].c_str());
            config.SSIDs[i] = args[2];
        }
        else if(args[0] == "pass"){
            int i = args[1].toInt();
            logln("Setting Trusted PASS %d to %s", i, args[2].c_str());
            config.PASSs[i] = args[2];
        }
        else if(args[0] == "test"){
            int i = args[1].toInt();
            logln("Setting TEST to %d", i);
            config.test = i;
        }
        else if(args[0] == "wifibegin"){
            network.begin();
        }
        else if(args[0] == "wifiend"){
            logln("Disconnecting from WiFi.");
            network.end();
        }
        else if(args[0] == "wifistatus"){
            network.printFullConnectionStatus();
        }
        else if(args[0] == "reboot"){
            ESP.restart();
        }
        else{
            logln("Unknown command: %s", args[0].c_str());
        }
    }
};

inline Commander commander;