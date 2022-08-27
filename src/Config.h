#pragma once
#include <Preferences.h>
#include "Log.h"

static Preferences preferances;

struct PreferanceEntries{
    enum class Type {String, U8, U32};
    static constexpr char Namespace[6] = "solar";
    static constexpr int TrustedAPsCount = 3;
    static constexpr int MaxEntriesCount = TrustedAPsCount*2 + 2;
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
    uint32_t wifiConnectionTimeout = 5000;
    uint8_t  test = 0;


    // Methods
    void bindToPreferanceEntries(){
        for(int i=0; i<TrustedAPsCount; i++){
            preferanceEntries.addEntry(String("ssid") + i, PreferanceEntries::Type::String, &SSIDs[i]);
            preferanceEntries.addEntry(String("pass") + i, PreferanceEntries::Type::String, &PASSs[i]);
        }
        preferanceEntries.addEntry("wifiCTO",    PreferanceEntries::Type::U32,  &wifiConnectionTimeout);
        preferanceEntries.addEntry("test",       PreferanceEntries::Type::U8,   &test);
    }

    void print(){
        logln("Printing Loaded Configuration: ");
        
        for(int i=0; i<TrustedAPsCount; i++){
            logln("AP%d: SSID:[%s], PASS:[%s]", i, SSIDs[i].c_str(), PASSs[i].c_str());
        }
        logln("WiFi Connection Timeout: %u", wifiConnectionTimeout);
        logln("test: %u", test);
    }

    Config(bool autoBind = false){
        if(autoBind){
            bindToPreferanceEntries();
        }
    }
};

inline Config config(true);