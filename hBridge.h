#ifndef hBridge_h
#define hBridge_h

#include "mbed.h"
#include "drivers/I2C.h"

#define defaultCalValue 1000
#define defSpeed        0x3F
#define writeReg        0x00
#define STOP            0b11

#ifndef status_type
#define status_type
typedef enum status{
    STATUS_OK = 0,
    STATUS_NO_DATA = -1000,
    STATUS_TX_ERROR = -1001,
    STATUS_RX_ERROR = -1002,
    STATUS_TX_TIMEOUT = -1003,
    STATUS_RX_TIMEOUT = -1004,
    STATUS_UNKNOWN = -1005
}status_t;
#endif

class pPump{
public:
    explicit pPump(I2C *boardI2C, uint8_t ADDR);
    status_t pump(long vol, bool dir = true);
    status_t start(bool dir = true);
    status_t stop();
    void calibrate();
    void setCalibration(float val);
    void setSpeed(uint8_t speed);
    bool state();
    long lastv();

protected:
    uint8_t pumpAddr;
    bool currentState;  
    long cal;
    long lastV_time;
    uint8_t currentSpeed;
    I2C *pumpI2C;
};

#endif