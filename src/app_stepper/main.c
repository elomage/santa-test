/* 
 *  Santa stepper control for the rotating antenna mount
 */
  
#include "stdmansos.h"
#include "stepper.h"
#include "../phaser_msg.h"


//#define DELAY_RATE 500      // mdelay between global loop iterations
#define DELAY_RATE 10      // mdelay between global loop iterations


// -------------------------------------------------------------------------
// Types and global data
// -------------------------------------------------------------------------


// Define a buffer for receiving messages
MSG_DEFINE_BUFFER_WITH_ID(radioBuffer, recv_data_p, RADIO_MAX_PACKET);

// Flag for the request to change the angle 
static bool fl_AngleProcessing = false;
static angle_t newAngle = 0;

// Phaser angle setting message
MSG_NEW_WITH_ID(ack_msg, phaser_angle_t, PH_MSG_Angle);


// -------------------------------------------------------------------------
// Communications statistics during runtime
// -------------------------------------------------------------------------
typedef struct {
    int recvPktDropped;   // receiving while processing another packet
    int recvPktFailed;    // radioRecv failed, length <0
    int recvPktInvalid;   // packet decoding failed
    int recvPktCount;     // packets received
} ComStat_t;

// Global comm.statistics object
ComStat_t stat = {0,0,0,0};


// =========================================================================
// =========================================================================

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
bool setAngle(int angle)
{
    bool ret = stepAbsolute(angle);

    // Send ACK
    ack_msg.payload.angle = angle;
    ack_msg.payload.action = MSG_ACT_ACK;
    MSG_DO_CHECKSUM( ack_msg );
    MSG_RADIO_SEND( ack_msg );

    fl_AngleProcessing = false; // angle request done
    return ret;
}

// -------------------------------------------------------------------------
// Incoming radio message handler
// -------------------------------------------------------------------------
static bool flReceiving=false;
// - - - - - - - - - - - - - - - - - - - - - - 
void onRadioRecv(void)
{
    bool flOK=true;
    int16_t len;

    if( flReceiving ){
        stat.recvPktDropped ++;
        return;
    }
    flReceiving = true;

    stat.recvPktCount ++;

    len = MSG_RADIO_RECV(radioBuffer);
    if (len < 0) {
        stat.recvPktFailed ++;
        flReceiving = false; 
        return; 
    }

    //PRINTF("Len=%d\t", (int)len);
    // debugHexdump((uint8_t *) &radioBuffer, len);

    led1Toggle();

    if( ! MSG_SIGNATURE_OK(radioBuffer) ) { flReceiving = false; return; }

    led2Toggle();

    // Anticipated payload types.
    MSG_NEW_PAYLOAD_PTR(radioBuffer, phaser_angle_t, angle_data_p);


    switch( radioBuffer.id ){
    case PH_MSG_Angle:
        MSG_CHECK_FOR_PAYLOAD(radioBuffer, phaser_angle_t, flOK=false );
        if( !flOK ){ flReceiving = false; return; }

        if( angle_data_p->action == MSG_ACT_SET ){
            newAngle = angle_data_p->angle;
            fl_AngleProcessing = true; // set the angle at first convenience
        }
        break;
    }
    flReceiving = false;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void appMain(void)
{
    stepperInit();

    radioSetReceiveHandle(onRadioRecv);
    radioOn();

    stepperZero();

    while (1) {
        if( fl_AngleProcessing ){
            setAngle( newAngle );
        }

        // Test: Check stepper zero position
        // led2Set( stepperSenseZero() );

        mdelay(DELAY_RATE);
        led0Toggle();

        // Test: move stepper
        // step( 50 );
        // mdelay(500);

    }
}
