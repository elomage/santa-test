// --------------------------------------------
// Monitor incoming radio packets (buffered version).
// Blinks on message or RX error.
// Sends message data to the serial port.
// --------------------------------------------

#include "stdmansos.h"
#include "../phaser_msg.h"

#define PRINT_CSV 1     // Comment this to print labels as well

#define RATE_DELAY 200

// #define RADIO_BUF_LEN RADIO_MAX_PACKET
// static uint8_t radioBuffer[RADIO_BUF_LEN];


typedef struct {
    uint8_t data[RADIO_MAX_PACKET];
} __attribute__((packed)) max_msg_buffer_t;


//===========================================
// Database
//===========================================

#define MAX_MSG_TYPE  max_msg_buffer_t

#define DB_RECORD_NUM 32

#define DB_REC_LEN sizeof( MAX_MSG_TYPE )

typedef struct 
{
    msg_timestamp_t timestamp;
    int recLen;     //received packet length
    rssi_t rssi;
    MAX_MSG_TYPE msg;    
} __attribute__((packed)) 
db_record_t;

typedef struct
{
    int curIdx;
    db_record_t data[DB_RECORD_NUM];
} database_t;

#define DB_NEW(dbname)  \
    database_t dbname = { .curIdx=0 };

#define DB_TOP(dbname)  \
    dbname.curIdx=0;    \

#define DB_NEXT(dbname)     \
    dbname.curIdx++;        \
    if(dbname.curIdx > DB_LAST_IDX ) dbname.curIdx=0;  \

#define DB_REC(dbname)  (dbname.data[dbname.curIdx])

//===========================================


DB_NEW(db);


int rxIdx=0;
bool flRxProcessing=false;

// --------------------------------------------
void onRadioRecv(void)
{
    uint32_t rxTime = getTimeMs();
    int16_t rxLen;
    int8_t rssi;

    rxIdx++;
    if(rxIdx<0) rxIdx=0;

    if(flRxProcessing){
        PRINTF("RX Locked\n");
        return;
    }
    flRxProcessing=true;    // There might be a small race condition

    led1Toggle();

    rxLen = radioRecv( &(DB_REC(db)), DB_REC_LEN);
    DB_REC(db).recLen = rxLen;
    // rxLen = radioRecv(radioBuffer, sizeof(radioBuffer));

    rssi = radioGetLastRSSI();

    PRINTF("%d\t%d\t%d\t%ld\t", (int)rxIdx, (int)rxLen, (int)rssi, (long)rxTime);

    if (rxLen < 0) {
        led2Toggle();
        PRINTF("RX failed\n");
    }
    else if (rxLen > 0 ) {
        // debugHexdump((uint8_t *) radioBuffer, rxLen);
        debugHexdump((uint8_t *) &(DB_REC(db)), rxLen);
    }
    flRxProcessing=false;
}

// --------------------------------------------
void appMain(void)
{
    DB_TOP(db);

    radioSetReceiveHandle(onRadioRecv);
    radioOn();

    while (1) {
        mdelay(RATE_DELAY);
        led0Toggle();
    }
}
