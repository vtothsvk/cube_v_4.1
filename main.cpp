#include "mbed.h"
#include "hBridge.h"
#include "SHTC3.h"
#include "INA219.hpp"
#include <iostream>

#define CAN_ADDR    0x0002
#define CAN_devType 0x0011
#define CAN_Idmask  0x3FF800

#define DEAD_TIME   300 //s

#define HBR_ADDR    0xC0

#define relays      2

PinName relayPin[relays] = {A6, A0};

//can id macros//
#define pumpStateId     0x00
#define relayStateId    0x08
#define relay1mask      0x02
#define relay2mask      0x04
#define vipId           0x10
#define shtcId          0x11
#define RW              0x01

#define pumpStateMask   0x06
#define pumpDirMask     0x02

void testLoop(void);

//Booting//
void pumpInit(void);
void CAN_init(void);
void LED(void);

//CAN callback//
void CAN_cb(void);
void CAN_incoming_data_cb(CANMessage *msg);
status_t CAN_pub(CANMessage msg);

void blink(void);

//CAN Interface//
CAN can(PA_11, PA_12);
CANMessage msgBuffer;

//I2C Interface//
static I2C i2c(D4, D5);

//SHTC3 temp&hum//
SHTC shtc(&i2c);

//INA219//
INA219 ina(D4, D5);

//Hbridge - Peristaltic pump//
pPump pump(&i2c, HBR_ADDR);

//GPIO//
DigitalOut led(LED1);
DigitalOut relay[relays]{
    DigitalOut(relayPin[0]),
    DigitalOut(relayPin[1])
};

//Timers//
Ticker LEDtim;

//CAN Therad//
Thread CANThread;
EventQueue CAN_cb_queue;

Thread test;

//Cube//
float v, i, p;
unsigned long elapsed = 0;
bool rState[2] = {false, false};

int main()
{   //booting sequence
    cout << "Booting Cube v4..." << endl;
    LEDtim.attach(&LED, 1);
    pumpInit();
    CAN_init();
    ina.calibrate_16v_400mA();
    //end of booting sequence

    if(shtc.init()){
        cout << "Error: SHTC3 failed to initialise..." << endl; 
    }

    //test.start(&testLoop);

    while (elapsed <= DEAD_TIME) {
        i = ina.read_current_mA();
        cout << "Papam " << i + 0 << "mA" << endl;
        ThisThread::sleep_for(5000);
        elapsed += 5;
    }

    NVIC_SystemReset();
}

void pumpInit(){
    if(pump.stop()){
        cout << "H-bridge failed to initialise..." << endl;
    }
}

void CAN_init(){
    can.mode(CAN::Reset);
    can.mode(CAN::Normal);
    int canID = (CAN_devType << 17)|(CAN_ADDR << 11);
    can.frequency(250000);
    CANThread.start(callback(&CAN_cb_queue, &EventQueue::dispatch_forever));
    can.filter(canID, CAN_Idmask, CANExtended);
    can.attach(CAN_cb_queue.event(&CAN_cb), CAN::RxIrq); 
    //can.mode(CAN::Reset);
    //can.mode(CAN::Normal);
}

void CAN_cb(){
    if(can.read(msgBuffer, 0)){
        elapsed = 0;
        CAN_cb_queue.call(&CAN_incoming_data_cb, &msgBuffer);
    }
    blink();
}

void CAN_incoming_data_cb(CANMessage* msg){
    int commandId = msg -> id;

    if((commandId&pumpStateId) == pumpStateId){
        if(commandId&RW){
            //Read
            char buffer;
            buffer = (char)pump.state();

            CAN_cb_queue.call(&CAN_pub, CANMessage(pumpStateId, &buffer, 1, CANData, CANExtended));
        }
        else{
            //Write
            if((commandId&pumpStateMask) == 0x00){
                cout << "stop" << endl;
                cout << pump.stop() << endl;
            }else{
                cout << "start dir: " << (commandId&pumpDirMask) + 0 << endl;
                cout << pump.start(commandId&pumpDirMask) << endl;
            }
        }
    }

    else if((commandId&relayStateId) == relayStateId){
        if(commandId&RW){
            //Read
            char buffer[2];
            for(uint8_t i = 0; i < 2; i++){
                memcpy(&buffer[i], &rState[i], 1);
            }

            CAN_cb_queue.call(&CAN_pub, CANMessage(relayStateId, &buffer[0], 2, CANData, CANExtended));
        }
        else{
            //Write
            rState[0] = relay[0] = commandId&relay1mask;
            rState[1] = relay[1] = commandId&relay2mask;
        }
    }

    else if((commandId&vipId) == vipId){
        char buffer[8];
        memcpy(&buffer[0], &v, 4);
        memcpy(&buffer[3], &i, 4);

        CAN_cb_queue.call(&CAN_pub, CANMessage(vipId + 1, &buffer[0], 8, CANData, CANExtended));
    }

    else if((commandId&shtcId) == shtcId){
        shtc.read();
        char buffer[8];
        float ambientTemp = shtc.lastTemp();
        float ambientHum = shtc.lastHum();

        memcpy(&buffer[0], &ambientTemp, 4);
        memcpy(&buffer[3], &ambientHum, 4);

        CAN_cb_queue.call(&CAN_pub, CANMessage(shtcId + 1, &buffer[0], 8, CANData, CANExtended));
    }
}

status_t CAN_pub(CANMessage msg){
    if(can.write(msg)){
        return STATUS_TX_ERROR;
    }

    return STATUS_OK;
}

void LED(void){
    led = !led;
}

void blink(void){
    led = 1;
    ThisThread::sleep_for(25);
    led = 0;
    ThisThread::sleep_for(25);
    led = 1;
    ThisThread::sleep_for(25);
    led = 0;
}

void testLoop(){
    cout << "starting test thread" << endl;
    cout << "pump start: " << (pump.start(true) ? "error" : "success" ) << endl;
    char buffer[10] = "siz";
    ThisThread::sleep_for(5000);
    cout << pump.stop() + 0 << endl;
}