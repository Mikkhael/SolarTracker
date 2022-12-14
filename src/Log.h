#pragma once
#include <Arduino.h>
#include <WiFiServer.h>

inline WiFiServer TelnetPrint(23);


void sendLogsToWSS(const char* buf, bool newline);

template<typename ...Args>
void logp(const char* format, Args&& ...args){
    static char buf[200];
    snprintf(buf, 200, format, std::forward<Args>(args)...);
    Serial.print(buf);
    TelnetPrint.print(buf);
    sendLogsToWSS(buf, false);
}
template<typename ...Args>
void logln(const char* format, Args&& ...args){
    static char buf[200];
    snprintf(buf, 200, format, std::forward<Args>(args)...);
    Serial.print(buf);
    Serial.println();
    TelnetPrint.print(buf);
    TelnetPrint.println();
    sendLogsToWSS(buf, true);
}