#include "HTTP.h"
#include "Commander.h"
#include "Config.h"

void WSS::handle(uint8_t num, WStype_t type, uint8_t * payload, size_t length){

    if(length == 1 && payload[0] == 't'){
        this->sendTXT(num, "HELLO SOCKET WORLD!");
    }else if(length == 1 && payload[0] == 'd'){
        String p = String("d") + preferanceEntries.getPrefsString();
        logln("[WSS] Downloading Config string: '%s'", p.c_str());
        this->sendTXT(num, p);
    }else if(length > 1 && payload[0] == 'u'){
        String s = String(payload+1, length-1);
        logln("[WSS] Uploading Config from string: '%s'", s.c_str());
        preferanceEntries.updateFromString(s);
    }else if(length > 1 && payload[0] == 'c'){
        String s = String(payload+1, length -1);
        logln("[WSS] Executing remote command '%s'", s);
        commander.parseAndExecute(s);
    }else{
        logln("[WSS] [%d] Unrecognized Request Format (ID: %u, Len: %u).", num, length > 0 ? payload[0] : 0, length);
    }

}