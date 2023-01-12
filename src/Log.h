#pragma once
#include <Arduino.h>
// #include <BluetoothSerial.h>
#include <WiFiServer.h>

// inline BluetoothSerial SerialBT;
inline WiFiServer TelnetPrint(23);



void sendLogsToWSS(const char* buf, bool newline);

inline constexpr size_t logBufSize = 1000;
inline char logBuf[logBufSize];

template<typename ...Args>
void logp(const char* format, Args&& ...args){
    snprintf(logBuf, logBufSize, format, std::forward<Args>(args)...);
    Serial.print(logBuf);
    //SerialBT.print(logBuf);
    TelnetPrint.print(logBuf);
    sendLogsToWSS(logBuf, false);
}
template<typename ...Args>
void logln(const char* format, Args&& ...args){
    snprintf(logBuf, logBufSize, format, std::forward<Args>(args)...);
    Serial.print(logBuf);
    Serial.println();
    //SerialBT.print(logBuf);
    //SerialBT.println();
    TelnetPrint.print(logBuf);
    TelnetPrint.println();
    sendLogsToWSS(logBuf, true);
}