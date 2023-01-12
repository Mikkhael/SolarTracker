#pragma once
#include "Network.h"
#include "Control.h"


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
            logln("https://github.com/Mikkhael/SolarTracker/blob/master/src/Commander.h");
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
        else if(args[0] == "hmotor"){
            int state_int = args[1].toInt();
            logln("Setting H Motor state to %d (%s)", state_int, motorStateNames[state_int]);
            controller.setMode(Controller::Mode::Manual);
            hmotor.setState(static_cast<Motor::State>(state_int));
        }
        else if(args[0] == "gotopos"){
            int posh = args[1].toInt();
            float poshp = float(posh) / config.controlMaxExtensionTime * 100;
            logln("Setting H Motor Target Position to %d (%.3f %%)", posh, poshp);
            controller.startTask_MoveToPosition(posh);
            controller.setMode(Controller::Mode::CustomPosition);
        }
        else if(args[0] == "gotoposp"){
            float poshp = args[1].toFloat();
            int posh = config.controlMaxExtensionTime * poshp / 100;
            logln("Setting H Motor Target Position to %d (%.3f %%)", posh, poshp);
            controller.startTask_MoveToPosition(posh);
            controller.setMode(Controller::Mode::CustomPosition);
        }
        else if(args[0] == "gotoazim"){
            float azim = args[1].toFloat();
            auto posh = controller.getPositionFromAzimuth(azim);
            logln("Setting H Motor Target Azimuth to %f (pos = %d)", azim, posh);
            controller.startTask_MoveToPosition(posh);
            controller.setMode(Controller::Mode::CustomPosition);
        }
        else if(args[0] == "ctrlsensor"){
            logln("Reasuming automatic control - Light Sensors.");
            controller.setMode(Controller::Mode::LightSensors);
        }
        else if(args[0] == "ctrlsun"){
            logln("Reasuming automatic control - Sun Position.");
            controller.setMode(Controller::Mode::SunPosition);
        }
        else if(args[0] == "ctrlstatus"){
            controller.printState();
        }
        else if(args[0] == "ctrlforcecalib"){
            logln("Forcing Calibration of motors.");
            hmotor.position = 0;
            hmotor.calibrated = true;
        }
        else if(args[0] == "reinit"){
            hmotor.initAsH();
            positionCalculatorH.initAsH();
            ntc.setUpdateInterval(config.ntcUpdateInterval);
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