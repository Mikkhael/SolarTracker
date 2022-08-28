#pragma once

#include "Log.h"
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Update.h>
#include "html.h"
#include "secrets.h"


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

    void handle(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

    WSS() : WebSocketsServer(81){
        this->onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t length){

            auto ip = this->remoteIP(num);
            switch(type){
                case WStype_CONNECTED:{
                    logln("[WSS] [%u] Connection from %s.", num, ip.toString().c_str());
                    break;
                }
                case WStype_DISCONNECTED:{
                    logln("[WSS] [%u] Disconnect.", num);
                    break;
                }
                case WStype_ERROR:{
                    logln("[WSS] [%u] ERROR from %s: %s", num, ip.toString().c_str(), payload);
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

inline WSS wss;
