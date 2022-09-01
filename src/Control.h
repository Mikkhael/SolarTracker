#pragma once
#include "Config.h"


inline const char* motorStateNames[] = {"Free", "FailStop", "SoftStop", "MoveIn", "MoveOut", "Calibration"};

struct Motor{

    int pinEnable = 0;
    int pinIn1 = 0;
    int pinIn2 = 0;
    int pwmch = 0;

    bool pinIn1Value = false;
    bool pinIn2Value = false;
    int  pinEnableValue = 0;

    void init(int pinEnable, int pinIn1, int pinIn2, int pwmch){
        this->pinEnable = pinEnable;
        this->pinIn1 = pinIn1;
        this->pinIn2 = pinIn2;
        this->pwmch = pwmch;
        
        setState(State::Free);
    }
    void initAsH(){
        init(config.pinEnA, config.pinIA1, config.pinIA2, 1);
    }

    // Current State
    enum State {Free, FastStop, SoftStop, MoveIn, MoveOut, Calibration}; 
    State state = State::Free;
    State accualState = State::Free;
    bool calibrated = false;
    uint32_t position = 0;
    uint32_t calibrationStartTime = 0;

    float getPositionProcentage(){ return (float)position / (float)config.controlMaxExtensionTime;}

    void setState(State new_state){
        state = new_state;
        if(state == State::MoveIn && position == 0 && calibrated){
            state = State::SoftStop;
        }else if(state == State::MoveOut && position >= config.controlMaxExtensionTime && calibrated){
            state = State::SoftStop;
        }
        if(state == State::MoveIn || state == State::MoveOut){
            pinIn1Value = (state == State::MoveIn)  != config.controlInverted;
            pinIn2Value = (state == State::MoveOut) != config.controlInverted;
            pinEnableValue = config.controlPWMDutyCycle;
        }else if(state == State::Calibration){
            calibrationStartTime = millis();
            pinIn1Value = true  != config.controlInverted;
            pinIn2Value = false != config.controlInverted;
            pinEnableValue = config.controlPWMDutyCycle;
        }else{
            pinIn1Value = pinIn2Value = (state == State::SoftStop);
            pinEnableValue = state == State::FastStop ? 255 : 0;
        }
        update();
    }
    void outputControl(){
        if(state == accualState)
            return;
        accualState = state;
        digitalWrite(pinIn1, pinIn1Value);
        digitalWrite(pinIn2, pinIn2Value);
        ledcWrite(pwmch, pinEnableValue);
    }

    void setDirection(int direction, bool fastStop = false){
        setState(direction == 0 ? 
            (fastStop ? State::FastStop : State::SoftStop) : 
            (direction == 1 ? State::MoveOut : State::MoveIn));
    }
    void forceStop(bool fast = false){
       state = (fast ? State::FastStop : State::SoftStop);
       pinIn2Value = pinIn1Value = !fast;
       pinEnableValue = fast ? 255 : 0;
       outputControl();
    }

    unsigned long lastTime = 0;
    void update(){
        auto now = millis();
        if(lastTime == 0) lastTime = now;
        auto diff = now - lastTime;
        lastTime = now;

        if(state == State::MoveOut){
            position += diff;
            if(calibrated && position >= config.controlMaxExtensionTime){
                forceStop();
            }
        }else if(state == State::MoveIn){
            if(calibrated && diff >= position) {
                position = 0;
                forceStop();
            } else {
                position -= diff;
            }
        }else if(state == State::Calibration){
            if(now - calibrationStartTime > config.controlCalibrationTime){
                calibrated = true;
                position = 0;
                forceStop();
            }
        }
        outputControl();
    }


};

inline Motor hmotor;


struct Controller{

    bool manualControl = false;

    int raw_h1 = 0;
    int raw_h2 = 0;
    uint32_t val_h1 = 0; // Ohm
    uint32_t val_h2 = 0; // Ohm

    int gradient_h = 0;

    void readSensors(){
        raw_h1 = analogRead(config.pinSensH1);
        raw_h2 = analogRead(config.pinSensH2);
        val_h1 = (config.controlResistorValue * raw_h1) / (4096 - raw_h1);
        val_h2 = (config.controlResistorValue * raw_h2) / (4096 - raw_h2);
        
        if(val_h1 > val_h2){
            if(val_h1 - val_h2 >= config.controlTriggerDiff){
                gradient_h = 1;
            }else if(gradient_h == -1){
                gradient_h = 0;
            }
        }else if(val_h1 < val_h2){
            if(val_h2 - val_h1 >= config.controlTriggerDiff){
                gradient_h = -1;
            }else if(gradient_h == 1){
                gradient_h = 0;
            }
        }else{
            gradient_h = 0;
        }
    }

    void loop(){
        readSensors();
        if(manualControl){
            hmotor.update();
        }else{
            hmotor.setDirection(gradient_h);
        }
    }

    void printState(){
        logln("Manual: %d", manualControl);
        logln("HORIZONTAL");
        logln("  Sensor 1 Raw: %d", raw_h1);
        logln("  Sensor 2 Raw: %d", raw_h2);
        logln("  Sensor 1 Ohm: %u", val_h1);
        logln("  Sensor 2 Ohm: %u", val_h2);
        logln("  Gradient: %d", gradient_h);
        logln("  Position: %f (%u)", hmotor.getPositionProcentage(), hmotor.position);
        logln("  Calibrated: %d", hmotor.calibrated);
        int hmotor_state_int = static_cast<int>(hmotor.state);
        logln("  Motor State: %d (%s)", hmotor_state_int, motorStateNames[hmotor_state_int]);
    }

};

inline Controller controller;