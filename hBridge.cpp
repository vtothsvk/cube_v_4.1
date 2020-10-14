#include "hBridge.h"

pPump::pPump(I2C *boardI2C, uint8_t ADDR){
    this -> pumpAddr = ADDR;
    this -> pumpI2C = boardI2C;
    this -> currentState = false;
    this -> cal = defaultCalValue;
    this -> currentSpeed = defSpeed;
}

status_t pPump::pump(long vol, bool dir){
    char message[2];
    message[0] = writeReg;
    message[1] = ((this -> currentSpeed) << 2)|(0b1 + dir);

    if(this -> pumpI2C -> write(this -> pumpAddr, &message[0], 2, false)){
        return STATUS_TX_ERROR;
    }
    this -> currentState = true;
    ThisThread::sleep_for(vol*(this -> cal));
    this -> lastV_time = vol*(this -> cal);
    message[1] = STOP;

    if(this -> pumpI2C -> write(this -> pumpAddr, &message[0], 2, false)){
        return STATUS_TX_ERROR;
    }
    this -> currentState = false;
    ThisThread::sleep_for(10);
    return STATUS_OK;
}

status_t pPump::start(bool dir){
    char message[2];
    message[0] = writeReg;
    message[1] = ((this -> currentSpeed) << 2)|(0b1 + dir);

    if(this -> pumpI2C -> write(this -> pumpAddr, &message[0], 2, false)){
        return STATUS_TX_ERROR;
    }
    this -> currentState = true;

    ThisThread::sleep_for(10);
    return STATUS_OK;
}

status_t pPump::stop(){
    char message[2];
    message[0] = writeReg;
    message[1] = STOP;

    if(this -> pumpI2C -> write(this -> pumpAddr, &message[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    this -> currentState = false;
    ThisThread::sleep_for(10);
    return STATUS_OK;
}

void pPump::calibrate(){
    this -> start();
    ThisThread::sleep_for(9990);
    this -> stop();
}

void pPump::setCalibration(float val){
    this -> cal = 10000 / val;
}

void pPump::setSpeed(uint8_t speed){
    this -> currentSpeed = speed&0x3F;
}

bool pPump::state(){
    return this -> currentState;
}

long pPump::lastv(){
    return this -> lastV_time;
}