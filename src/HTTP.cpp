#include "HTTP.h"

void WSS::handle(uint8_t num, WStype_t type, uint8_t * payload, size_t length){

    if(length == 1 && payload[0] == 100){
        this->sendTXT(num, "HELLO SOCKET WORLD!");
    }
    else{
        logln("[WSS] [%d] Unrecognized Request Format (ID: %u, Len: %u).", num, length > 0 ? payload[0] : 0, length);
    }

}