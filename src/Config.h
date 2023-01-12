#pragma once
#include <Preferences.h>
#include "Log.h"

static Preferences preferances;

struct PreferanceEntries{
    enum class Type {String, U8, U32, I32};
    static constexpr char Namespace[6] = "solar";
    static constexpr int TrustedAPsCount = 3;
    static constexpr int MaxEntriesCount = TrustedAPsCount*2 + 40;
    String names[MaxEntriesCount];
    Type   types[MaxEntriesCount];
    void*  dests[MaxEntriesCount];

    int usedEntries = 0;
    bool addEntry(String name, Type type, void* dest = nullptr){
        if(usedEntries >= MaxEntriesCount) {
            logln("Exceded Maximum Preferance Entries (%d) for key '%s'", usedEntries, name.c_str());
            return false;
        }
        names[usedEntries] = name;
        types[usedEntries] = type;
        dests[usedEntries] = dest;
        usedEntries++;
        return true;
    }

    String getPrefsString(){
        String res;
        for(int i=0; i<usedEntries; i++){
            if(i > 0) res += ',';
            res += names[i];
            res += '|';
            switch(types[i]){
                case Type::String:  res += "STR|"; res += *(reinterpret_cast<String*>     (dests[i])); break;
                case Type::U8:      res += "U8|";  res += *(reinterpret_cast<uint8_t*>    (dests[i])); break;
                case Type::U32:     res += "U32|"; res += *(reinterpret_cast<uint32_t*>   (dests[i])); break;
                case Type::I32:     res += "I32|"; res += *(reinterpret_cast<int32_t*>    (dests[i])); break;
            }
        }
        return res;
    }

    void updateFromString(String str){
        int stage = 0;
        String name = "";
        String value = "";
        for(size_t i = 0; i<=str.length(); i++){
            char c = str[i];
            if(stage == 0){
                if(c == '|') stage = 1;
                else         name += c;
            }else if(stage == 1){
                if(c == '|') stage = 2;
            }else if(stage == 2){
                if(c != ',' && c != '\0') {
                    value += c;
                    continue;
                }
                stage = 0;
                int e = 0;
                for(e = 0; e<usedEntries; e++){
                    if(names[e] == name) break;
                }
                if(e == usedEntries){
                    logln("Prefs Upload: name '%s' not found", name.c_str());
                    name = "";
                    value = "";
                    continue;
                }
                logln("Updating name '%s' of type %d with value %s (%d)", names[e].c_str(), types[e], value.c_str(), value.toInt());
                switch(types[e]){
                    case Type::String:  *(reinterpret_cast<String*>   (dests[e])) = value;          break;
                    case Type::U8:      *(reinterpret_cast<uint8_t*>  (dests[e])) = value.toInt();  break;
                    case Type::U32:     *(reinterpret_cast<uint32_t*> (dests[e])) = value.toInt();  break;
                    case Type::I32:     *(reinterpret_cast<int32_t*>  (dests[e])) = value.toInt();  break;
                }
                name = "";
                value = "";
            }
        }
    }

    bool load(){
        bool res = preferances.begin(Namespace, false);
        auto freeEntries = preferances.freeEntries();
        logln("Loading Configuration from Namespace: '%s', Free entries: %u, TrustedAPs: %d", Namespace, freeEntries, TrustedAPsCount);
        if(!res){
            logln("Failed Loading configuration.");
            return false;
        }
        for(int i=0; i<usedEntries; i++){
            if(!preferances.isKey(names[i].c_str())){
                logln("Missing Key: %s", names[i].c_str());
                continue;
            }
            if(dests[i] == nullptr){
                logln("Missing dest for Key: %s", names[i].c_str());
                continue;
            }

            if(types[i] == Type::String){
                auto dest = reinterpret_cast<String*>(dests[i]);
                *dest = preferances.getString(names[i].c_str());
            }else if(types[i] == Type::U8){
                auto dest = reinterpret_cast<uint8_t*>(dests[i]);
                *dest = preferances.getUChar(names[i].c_str());
            }else if(types[i] == Type::U32){
                auto dest = reinterpret_cast<uint32_t*>(dests[i]);
                *dest = preferances.getULong(names[i].c_str());
            }else if(types[i] == Type::I32){
                auto dest = reinterpret_cast<int32_t*>(dests[i]);
                *dest = preferances.getLong(names[i].c_str());
            }
        }
        logln("Loading Configurtation completed.");
        preferances.end();
        return true;
    }

    bool save(){
        bool res = preferances.begin(Namespace, false);
        auto freeEntries = preferances.freeEntries();
        logln("Saving Configuration from Namespace: '%s', Free entries: %u, TrustedAPs: %d", Namespace, freeEntries, TrustedAPsCount);
        if(!res){
            logln("Failed Saving configuration.");
            return false;
        }
        for(int i=0; i<usedEntries; i++){
            if(dests[i] == nullptr){
                logln("Missing dest for Key: %s", names[i].c_str());
                continue;
            }

            if(types[i] == Type::String){
                auto dest = reinterpret_cast<String*>(dests[i]);
                if(preferances.getString(names[i].c_str()) != *dest)
                    preferances.putString(names[i].c_str(), *dest);
            }else if(types[i] == Type::U8){
                auto dest = reinterpret_cast<uint8_t*>(dests[i]);
                if(preferances.getUChar(names[i].c_str()) != *dest)
                    preferances.putUChar(names[i].c_str(), *dest);
            }else if(types[i] == Type::U32){
                auto dest = reinterpret_cast<uint32_t*>(dests[i]);
                if(preferances.getULong(names[i].c_str()) != *dest)
                    preferances.putULong(names[i].c_str(), *dest);
            }else if(types[i] == Type::I32){
                auto dest = reinterpret_cast<int32_t*>(dests[i]);
                if(preferances.getLong(names[i].c_str()) != *dest)
                    preferances.putLong(names[i].c_str(), *dest);
            }
        }
        logln("Saving Configurtation completed.");
        preferances.end();
        return true;
    }
};

inline PreferanceEntries preferanceEntries;

struct Config{
    static constexpr int TrustedAPsCount = PreferanceEntries::TrustedAPsCount;
    String SSIDs[TrustedAPsCount];
    String PASSs[TrustedAPsCount];
    uint32_t dimHRadius         = 200; // mm
    uint32_t dimHAnchorDistance = 600; // mm
    uint32_t dimHMinLength      = 420; // mm
    uint32_t dimHMaxLength      = 720; // mm

    uint32_t wifiConnectionTimeout = 5000;
    uint32_t ntcUpdateInterval = 30*60*1000;
    uint32_t controlResistorValue    = 40000; // Ohm
    uint32_t controlTriggerDiff      = 20000; // Ohm
    uint32_t controlSunPosUpdateInterval = 600; // s
    int32_t  controlMinAzimuth = 30000; // cdeg
    int32_t  controlLenAzimuth = 60000; // cdeg
    uint32_t controlMaxExtensionTime = 1000; // ms
    uint32_t controlCalibrationTime  = 2000; // ms
    uint32_t controlPWMFrequency     = 40000;// Hz
    uint8_t  controlPWMDutyCycle     = 25;   // 1/256
    uint8_t  controlInverted         = 0;
    uint8_t  startWithManualControl  = 1;
    int32_t coordW = 18624; // cdeg
    int32_t coordN = 50286; // cdeg

    uint8_t  pinEnA = 13;
    uint8_t  pinIA1 = 12;
    uint8_t  pinIA2 = 14;

    
    uint8_t  pinSensH1 = 34;
    uint8_t  pinSensH2 = 35;

    uint8_t  test = 0;


    // Methods
    void bindToPreferanceEntries(){
        for(int i=0; i<TrustedAPsCount; i++){
            preferanceEntries.addEntry(String("ssid") + i, PreferanceEntries::Type::String, &SSIDs[i]);
            preferanceEntries.addEntry(String("pass") + i, PreferanceEntries::Type::String, &PASSs[i]);
        }

        preferanceEntries.addEntry("dhr",        PreferanceEntries::Type::U32,  &dimHRadius);
        preferanceEntries.addEntry("dhanc",      PreferanceEntries::Type::U32,  &dimHAnchorDistance);
        preferanceEntries.addEntry("dhdmin",     PreferanceEntries::Type::U32,  &dimHMinLength);
        preferanceEntries.addEntry("dhdmax",     PreferanceEntries::Type::U32,  &dimHMaxLength);

        preferanceEntries.addEntry("wifiCTO",    PreferanceEntries::Type::U32,  &wifiConnectionTimeout);
        preferanceEntries.addEntry("ntcint",     PreferanceEntries::Type::U32,  &ntcUpdateInterval);

        preferanceEntries.addEntry("res",        PreferanceEntries::Type::U32,  &controlResistorValue);
        preferanceEntries.addEntry("trg",        PreferanceEntries::Type::U32,  &controlTriggerDiff);

        preferanceEntries.addEntry("sunint",      PreferanceEntries::Type::U32,  &controlSunPosUpdateInterval);
        preferanceEntries.addEntry("minA",        PreferanceEntries::Type::I32,  &controlMinAzimuth);
        preferanceEntries.addEntry("lenA",        PreferanceEntries::Type::I32,  &controlLenAzimuth);

        preferanceEntries.addEntry("maxe",       PreferanceEntries::Type::U32,  &controlMaxExtensionTime);
        preferanceEntries.addEntry("cal",        PreferanceEntries::Type::U32,  &controlCalibrationTime);
        preferanceEntries.addEntry("frq",        PreferanceEntries::Type::U32,  &controlPWMFrequency);
        preferanceEntries.addEntry("pwm",        PreferanceEntries::Type::U8,   &controlPWMDutyCycle);
        preferanceEntries.addEntry("inv",        PreferanceEntries::Type::U8,   &controlInverted);
        preferanceEntries.addEntry("man",        PreferanceEntries::Type::U8,   &startWithManualControl);

        preferanceEntries.addEntry("ena",        PreferanceEntries::Type::U8,   &pinEnA);
        preferanceEntries.addEntry("ia1",        PreferanceEntries::Type::U8,   &pinIA1);
        preferanceEntries.addEntry("ia2",        PreferanceEntries::Type::U8,   &pinIA2);
        preferanceEntries.addEntry("sh1",        PreferanceEntries::Type::U8,   &pinSensH1);
        preferanceEntries.addEntry("sh2",        PreferanceEntries::Type::U8,   &pinSensH2);
        
        preferanceEntries.addEntry("coordW",     PreferanceEntries::Type::I32,   &coordW);
        preferanceEntries.addEntry("coordN",     PreferanceEntries::Type::I32,   &coordN);


        preferanceEntries.addEntry("test",       PreferanceEntries::Type::U8,   &test);
    }

    void print(){
        logln("Printing Loaded Configuration: ");
        logln("NETWORK:");
        
        for(int i=0; i<TrustedAPsCount; i++){
            logln("  AP%d: SSID:[%s], PASS:[%s]", i, SSIDs[i].c_str(), PASSs[i].c_str());
        }
        logln("  WiFi Connection Timeout: %u", wifiConnectionTimeout);
        logln("  NTC Update Interval: %u", ntcUpdateInterval);
        logln("DIMENSIONS HORIZONTAL: ");
        logln("  Turntable Radius: %umm", dimHRadius);
        logln("  Anchor Distance from center: %umm", dimHAnchorDistance);
        logln("  Minimum Accuator Length: %umm", dimHMinLength);
        logln("  Maximum Accuator Length: %umm", dimHMaxLength);
        logln("CONTROL:");
        logln("  Res (Ohm): %u", controlResistorValue);
        logln("  Trigger Diff (Ohm): %u", controlTriggerDiff);
        logln("  Sun Position Update Interval (s): %u", controlSunPosUpdateInterval);
        logln("  Azimuth anchor (deg): %.3f", controlMinAzimuth / 1000.f);
        logln("  Azimuth range  (deg): %.3f", controlLenAzimuth / 1000.f);
        logln("  Max Extension Time (ms): %u", controlMaxExtensionTime);
        logln("  Calibration Time (ms): %u", controlCalibrationTime);
        logln("  PWM Freq (Hz): %u", controlPWMFrequency);
        logln("  PWM Duty (%%): %.2f", controlPWMDutyCycle / 2.56f);
        logln("  Inverted: %u", controlInverted);
        logln("  Start Manual: %u", startWithManualControl);
        logln("  Coords: %.3f W, %.3f N", coordW / 1000.f, coordN / 1000.f);
        logln("PINS:");
        logln("  EnA: %u", pinEnA);
        logln("  IA1: %u", pinIA1);
        logln("  IA2: %u", pinIA2);
        logln("  SensH1: %u", pinSensH1);
        logln("  SensH2: %u", pinSensH2);
        

        //logln("test: %u", test);
    }

    Config(bool autoBind = false){
        if(autoBind){
            bindToPreferanceEntries();
        }
    }
};

inline Config config(true);