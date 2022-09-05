#pragma once
#include <NTPClient.h>
#include <WiFiUdp.h>

inline WiFiUDP ntc_client;
inline NTPClient ntc(ntc_client);