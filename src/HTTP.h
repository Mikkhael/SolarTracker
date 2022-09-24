#pragma once

#include "Log.h"
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Update.h>
#include "html.h"
#include "secrets.h"
#include "Commander.h"
#include "Config.h"


static const char* update_html = "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='update'><input type='checkbox' name='s' value='1'>SPIFFS<input type='submit' value='Update'></form>"; 

struct HTTP : public WebServer{
    static constexpr char OTA_USER[6] = "admin";

    size_t upload_bytes_received = 0;

    HTTP() : WebServer(80)
    {
        this->on("/", [this](){
            //this->send(202, "text/html", "<h1>HELLO WORLD !! OTA</h1>");
            this->send(200, "text/html", index_html);
        });

        this->on("/update", HTTP_GET, [this]{
            if(!this->authenticate(OTA_USER, OTA_PASS)){
                this->requestAuthentication(DIGEST_AUTH, "ESP32 Solar Tracker");
                return;
            }
            this->send(200, "text/html", update_html);
        });
        this->on("/upload", HTTP_POST, []{}, [this]{handleUpdateUpload();});
    }

    void handleUpdateUpload(){
        if(!this->authenticate(OTA_USER, OTA_PASS)){
            this->requestAuthentication(DIGEST_AUTH, "ESP32 Solar Tracker");
            return;
        }
        bool isSpiffs = this->hasArg("s");
        HTTPUpload& upload = this->upload();
        if (upload.status == UPLOAD_FILE_START) {
            upload_bytes_received = 0;
            if(!Update.begin(UPDATE_SIZE_UNKNOWN, isSpiffs ? U_SPIFFS : U_FLASH)){
                logln("[OTA] Failed to Start. (SPIFFS=%d)", isSpiffs);
                Update.printError(Serial);
            }else{
                logln("[OTA] Starting... (SPIFFS=%d)", isSpiffs);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            upload_bytes_received += upload.currentSize;
            logln("[OTA] Progress: %u/%u", upload_bytes_received, upload.totalSize);
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                logln("[OTA] Error during transmission.");
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { //true to set the size to the current progress
                logln("[OTA] Completed: %u. Rebooting...", upload.totalSize);
            } else {
                logln("[OTA] Failed to End.");
                Update.printError(Serial);
            }
        } else {
            logln("[OTA] Aborted (%u/%u).", upload_bytes_received, upload.totalSize);
            Update.end(true);
        }
    }
};

inline HTTP http;

struct WSS : public WebSocketsServer{

    bool requestedLogs[WEBSOCKETS_SERVER_CLIENT_MAX] {};
    int requestedLogsCount = 0;

    void setRequestedLogs(uint8_t num, bool val){
        if(requestedLogs[num] == val) return;
        requestedLogsCount += val ? 1 : -1;
        requestedLogs[num] = val;
    }

    void broadcastLogs(const char* buf, bool newline){
        static char newbuf[202 + 14];
        if(requestedLogsCount == 0){
            return;
        }
        int len = snprintf(newbuf + 14, 202, newline ? "l%s\n" : "l%s", buf);
        for(int num=0; num<WEBSOCKETS_SERVER_CLIENT_MAX; num++){
            if(requestedLogs[num]){
                this->sendTXT(num, newbuf, len, true);
            }
        }
    }

    void handle(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

    WSS() : WebSocketsServer(81){
        this->onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t length){

            auto ip = this->remoteIP(num);
            switch(type){
                case WStype_CONNECTED:{
                    logln("[WSS] [%u] Connection from %s.", num, ip.toString().c_str());
                    setRequestedLogs(num, false);
                    break;
                }
                case WStype_DISCONNECTED:{
                    logln("[WSS] [%u] Disconnect.", num);
                    setRequestedLogs(num, false);
                    break;
                }
                case WStype_ERROR:{
                    logln("[WSS] [%u] ERROR from %s: %s", num, ip.toString().c_str(), payload);
                    setRequestedLogs(num, false);
                    break;
                }
                case WStype_TEXT:
                case WStype_BIN:
                {
                    handle(num, type, payload, length);
                    break;
                }
                case WStype_PING:{
                    logln("[WSS] [%u] PING from %s.", num, ip.toString().c_str());
                    break;
                }
                case WStype_PONG:{
                    logln("[WSS] [%u] PONG from %s.", num, ip.toString().c_str());
                    break;
                }
                default:{
                    logln("[WSS] [%u] Unrecognized Event Type from %s: %d", num, ip.toString().c_str(), static_cast<int>(type));
                    break;
                }
            }

        });
    }
};



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
    }else if(length == 1 && payload[0] == 'l'){
        logln("[WSS] Num %u Registering for logs", num);
        setRequestedLogs(num, true);
    }else if(length == 1 && payload[0] == 'r'){
        String p = "r";

        (p += '|') += static_cast<int>(controller.mode);
        (p += '|') += static_cast<int>(hmotor.accualState);
        (p += '|') += hmotor.calibrated;
        (p += '|') += config.controlMaxExtensionTime;
        (p += '|') += hmotor.position;
        (p += '|') += controller.targetPositionH;
        (p += '|') += controller.targetPositionH_direction;
        (p += '|') += controller.val_h1;
        (p += '|') += controller.val_h2;
        (p += '|') += controller.sunTargetAltitude;
        (p += '|') += controller.sunTargetAzimuth;
        (p += '|') += controller.sunTargetPositonH;
        (p += '|') += ntc.getEpochTime();
        (p += '|') += controller.getAzimuthFromPosition(hmotor.position);

        this->sendTXT(num, p);
    }else{
        logln("[WSS] [%d] Unrecognized Request Format (ID: %u, Len: %u).", num, length > 0 ? payload[0] : 0, length);
    }

}

inline WSS wss;
