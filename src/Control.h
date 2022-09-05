#pragma once
#include "Config.h"
#include "Time.h"
#include "Astronomy.h"


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

        if(this->pinEnable != 0){
            ledcDetachPin(this->pinEnable);
        }

        this->pinEnable = pinEnable;
        this->pinIn1 = pinIn1;
        this->pinIn2 = pinIn2;
        this->pwmch = pwmch;

        ledcAttachPin(this->pinEnable, this->pwmch);
        ledcChangeFrequency(pwmch, config.controlPWMFrequency, 8);
        pinMode(this->pinIn1, OUTPUT);
        pinMode(this->pinIn2, OUTPUT);
        
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

    int getDirection(){
        return state == State::MoveIn ? -1 : (state == State::MoveOut ? 1 : 0);
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


template <typename T, size_t N>
struct Meanator{
    T data[N];

    T sum = 0;
    T mean = 0;
    
    size_t size = 0;
    size_t index = 0;

    void add(T val){
        if(size > 0){
            sum -= data[index];
        }
        data[index] = val;
        sum += val;
        if(size < N){
            size++;
        }
        index = (index + 1) % N;
        mean = sum / size;
    }

    T get(){
        return mean;
    }
};

static const char* ControllerModeNames[] = {"Manual", "CustomPosition", "LightSensors", "SunPositioin"};

struct Controller{


    enum Mode {Manual, CustomPosition, LightSensors, SunPosition} mode = Mode::Manual;
    void setMode(Mode mode){
        this->mode = mode;
        if(mode == Mode::SunPosition){
            lastSolarUpdate = 0;
        }
    }

    // Light Sensors
    Meanator<int, 10> mean_h1;
    Meanator<int, 10> mean_h2;
    int raw_h1 = 0;
    int raw_h2 = 0;
    uint32_t val_h1 = 0; // Ohm
    uint32_t val_h2 = 0; // Ohm

    int gradient_h = 0;

    void readSensors(){
        mean_h1.add( analogRead(config.pinSensH1) );
        mean_h2.add( analogRead(config.pinSensH2) );
        raw_h1 = mean_h1.get();
        raw_h2 = mean_h2.get();
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

    bool isSafeToInterrupt(){
        return hmotor.state == hmotor.accualState &&
               hmotor.state != Motor::State::MoveOut;
    }

    // Astronomy Controlls
    uint64_t lastSolarUpdate = 0;
    double  sunTargetAzimuth  = 0;
    double  sunTargetAltitude = 0;
    uint64_t sunTargetPositonH = 0;
    bool sunPositionUpdated = false;

    uint64_t getPositionFromAzimuth(double azimuth){
        double azimuth_min = config.controlMinAzimuth / 1000.f;
        double azimuth_len = config.controlLenAzimuth / 1000.f;
        auto azimuth_delta = getAzimuthDelta(azimuth, azimuth_min, azimuth_len);

        //TODO Measure And Improve
        return (azimuth_delta / azimuth_len) * config.controlMaxExtensionTime;
    }

    bool updateCurrentSunPos(){
        if(!ntc.isTimeSet()){
            return false;
        }
        if(lastSolarUpdate != 0 && ntc.getEpochTime() - lastSolarUpdate < config.controlSunPosUpdateInterval){
            return false;
        }

        lastSolarUpdate = ntc.getEpochTime();
        auto pos = getSunPosition(lastSolarUpdate, config.coordW / 1000.f, config.coordN / 1000.f);
        sunTargetAltitude = pos.altitude;
        sunTargetAzimuth  = pos.azimuth;
        sunTargetPositonH = getPositionFromAzimuth(sunTargetAzimuth);
        sunPositionUpdated = true;
        return true;
    }


    uint64_t targetPositionH = 0;
    int targetPositionH_direction = 0;

    void startTask_MoveToPosition(uint64_t posh){
        targetPositionH = posh;
        if(hmotor.position < posh){
            targetPositionH_direction = 1;
        }else if(hmotor.position > posh){
            targetPositionH_direction = -1;
        }else{
            targetPositionH_direction = 0;
        }
    }

    bool updateMotors_MoveToPosition(){
        if(      targetPositionH_direction == 1  && hmotor.position >= targetPositionH){
            targetPositionH_direction = 0;
        }else if(targetPositionH_direction == -1 && hmotor.position <= targetPositionH){
            targetPositionH_direction = 0;
        }
        hmotor.setDirection(targetPositionH_direction);
        return targetPositionH_direction == 0;
    }

    void updateMotors(){
        hmotor.update();
    }

    void loop(){
        readSensors();
        updateCurrentSunPos();

        if(mode == Mode::Manual){
            updateMotors();
        }else if(mode == Mode::CustomPosition){
            updateMotors_MoveToPosition();
        }else if(mode == Mode::LightSensors){
            hmotor.setDirection(gradient_h);
        }else if(mode == Mode::SunPosition){
            if(sunPositionUpdated){
                startTask_MoveToPosition(sunTargetPositonH);
                sunPositionUpdated = false;
            }
            updateMotors_MoveToPosition();
        }
    }

    void printState(){
        logln("State: %s", ControllerModeNames[static_cast<int>(mode)]);
        logln("HORIZONTAL");
        int hmotor_state_int = static_cast<int>(hmotor.state);
        logln(" Motor State: %d (%s)", hmotor_state_int, motorStateNames[hmotor_state_int]);
        logln(" Position: %f (%u)", hmotor.getPositionProcentage(), hmotor.position);
        logln(" Target Position: %u", targetPositionH);
        logln(" Target Position dir: %u", targetPositionH_direction);
        logln(" Calibrated: %d", hmotor.calibrated);
        logln(" LIGHT SENSORS");
        logln("  Sensor 1 Raw: %d", raw_h1);
        logln("  Sensor 2 Raw: %d", raw_h2);
        logln("  Sensor 1 Ohm: %u", val_h1);
        logln("  Sensor 2 Ohm: %u", val_h2);
        logln("  Gradient: %d", gradient_h);
        logln(" SUN POSITION");
        logln("  Azimuth: %f", sunTargetAzimuth);
        logln("  Altitude: %f", sunTargetAltitude);
    }

};

inline Controller controller;