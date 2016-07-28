// --------------------------------------------
// Monitor incoming radio packets.
// Blinks on message or RX error.
// Sends message data to the serial port.


// --------------------------------------------

#include "stdmansos.h"
#include "../phaser_msg.h"
#include "../db_framework.h"

// Comment below for less output
// #define PRINT_PACKETS 1


#define RATE_DELAY 200

// Phaser control message(s)
MSG_NEW_WITH_ID(ctrl_msg, phaser_control_t, PH_MSG_Control);
phaser_control_t *ctrl_data_p = &(ctrl_msg.payload);

// Define a buffer for receiving messages
MSG_DEFINE_BUFFER_WITH_ID(radioBuffer, recv_data_p, RADIO_MAX_PACKET);


// --------------------------------------------

// Setup the default test run parameters
test_config_t test_config = {
    .platform_id=PH_PHASER,
    .start_delay=1000,
    .send_count=100,
    .send_delay=20,
    .angle_step=25,
    .angle_count=8,
    .power={31,23,15,0},
    .ant.phaseA={
        .start=0,
        .step=32,
        .count=0    // 0
    },
    .ant.phaseB={
        .start=0,
        .step=32,
        .count=8    // 8
    },
};





//===========================================
//  Experimental data
//===========================================
#define NUM_EXPS 128

static experiment_t experiment[NUM_EXPS];
experiment_t * curExp = NULL;

int lastExpIdx=0;

int rxIdx=0;

static bool flRestart=true;


// Prototypes
void send_ctrl_msg(msg_action_t act);


// --------------------------------------------
// Serial receive handler
// --------------------------------------------
#define SER_BUF_SIZE 64
static uint8_t serBuffer[SER_BUF_SIZE];

void onSerRecv(uint8_t bytes)
{
    // int i;
    // PRINTF("Pong: ");
    // for(i=0; i<bytes; i++)
    // {
    //     PRINTF("%c", (char)serBuffer[i]);
    // }
    // PRINTF("\n");

    if(bytes>=1 && serBuffer[0] == 'r'){
        PRINTF("Ser: Restart!\n");
        flRestart = true;
        send_ctrl_msg(MSG_ACT_RESTART);
    }

}


// --------------------------------------------
// --------------------------------------------
void send_ctrl_msg(msg_action_t act)
{
    ctrl_msg.payload.action = act;
    MSG_DO_CHECKSUM( ctrl_msg );
    MSG_RADIO_SEND( ctrl_msg );
}

// --------------------------------------------
// --------------------------------------------
// void sendTestResults(int expIdxFrom, int expIdxTo)
void sendTestResults()
{
    int rssi_mean, lqi_mean;
    int32_t rssi_devSq, lqi_devSq;
    experiment_t *exp = curExp;

    curExp = NULL;  // Avoid retransmission of the same experiment

    // int i;
    // for (i=expIdxFrom; i<=expIdxTo; i++)
    {
        // exp = &(experiment[i]);
        rssi_mean = STREAM_STAT_MEAN(exp->rssi_data);
        rssi_devSq = STREAM_STAT_DEVIATION_SQUARED(exp->rssi_data);
        lqi_mean = STREAM_STAT_MEAN(exp->lqi_data);
        lqi_devSq = STREAM_STAT_DEVIATION_SQUARED(exp->lqi_data);
        PRINTF("Test:"
            "\t%d"
            "\t%d\t%d\t%d"
            // "\t%d\t%d\t%d\t%d\t%ld"
            // "\t%d\t%d\t%d\t%d\t%ld"
            "\t%d\t%d\t%d"
            "\t%ld\t%ld"
            "\n",
            (int) lastExpIdx,

            (int) exp->power,
            (int) exp->angle,
            (int) exp->phase,

            // (int) exp->rssi_data.sum,
            // (int) exp->rssi_data.sum_squares,

            // (int) exp->lqi_data.num,
            // (int) exp->lqi_data.sum,
            // (int) exp->lqi_data.sum_squares,

            (int) exp->rssi_data.num,
            (int) rssi_mean,
            (int) lqi_mean,

            (long unsigned int) (rssi_devSq),
            (long unsigned int) (lqi_devSq)
            )
        // debugHexdump((uint8_t *) exp, sizeof(experiment_t));

        // Clear data
        exp->power = 0;
        exp->angle = 0;
        exp->phase = 0;
        STREAM_STAT_INIT(exp->rssi_data);
        STREAM_STAT_INIT(exp->lqi_data);
    }
    //...

}

// --------------------------------------------
// --------------------------------------------
inline void processTestMsg(phaser_ping_t * test, rssi_t rssi, lqi_t lqi)
{
    // x
    int expIdx;
    // expIdx = test->expIdx;
    // if( expIdx<0 || expIdx>= NUM_EXPS) return;

    //Override caching data in array. use only one record.
    expIdx =0;

    experiment_t *exp = &(experiment[expIdx]);

    // exp->expIdx = test->expIdx;

    exp->power = test->power;
    exp->angle = test->angle;
    exp->phase = test->ant.phaseA | test->ant.phaseB ;
    STREAM_STAT_ADD(exp->rssi_data, rssi);
    STREAM_STAT_ADD(exp->lqi_data, lqi);
 
    curExp = exp;
    lastExpIdx = test->expIdx;
}

// --------------------------------------------
// --------------------------------------------
void printAction(action)
{
    PRINTF("Rx: %s\n", MSG_ACT_NAME( action ) );
}

// --------------------------------------------
// --------------------------------------------
void print_test_config(test_config_t *test_config)
{
    int pid = test_config->platform_id;
    PRINTF("\nPlatform: %s\n", PH_PLATFORM_NAME(pid) );

    PRINTF("Start_delay=%d\tSend_delay=%d\t Send_count=%d\n",
        (int) test_config->start_delay,
        (int) test_config->send_delay,
        (int) test_config->send_count);

    PRINTF("Angle_step=%d\tAngle_count=%d\n",
        (int) test_config->angle_step,
        (int) test_config->angle_count);

    int pw, i=0;
    PRINTF("TX_power:")
    while( (pw=test_config->power[i++]) ){
        PRINTF("\t%d", pw);
    }
    PRINTF("\n");

    PRINTF("Ant_config:\t%d\t%d\t%d\t%d\t%d\t%d\n",
        (int) test_config->ant.phaseA.start,
        (int) test_config->ant.phaseA.step,
        (int) test_config->ant.phaseA.count,
        (int) test_config->ant.phaseB.start,
        (int) test_config->ant.phaseB.step,
        (int) test_config->ant.phaseB.count);

    PRINTF("\n");
}


// --------------------------------------------
// --------------------------------------------
void onRadioRecv(void)
{
    static bool flRxProcessing=false;
    if(flRxProcessing){
#ifdef PRINT_PACKETS
        PRINTF("RX Locked\n");
#endif
        return;
    }
    flRxProcessing=true;    // There is a chance for a small race condition

#ifdef PRINT_PACKETS
    uint32_t rxTime = getTimeMs();
#endif
    int16_t rxLen;
    rssi_t rssi;
    lqi_t lqi;

    rxIdx++;
    if( rxIdx < 0 ) rxIdx=0;


    led1Toggle();

    // rxLen = radioRecv( &(DB_REC(db)), DB_REC_SIZE(db));
    // DB_REC(db).recLen = rxLen;
    rxLen = radioRecv(&radioBuffer, sizeof(radioBuffer));
    rssi = radioGetLastRSSI();
    lqi = radioGetLastLQI();

#ifdef PRINT_PACKETS
    PRINTF("%d\t%d\t%d\t%d\t%ld\t", (int)rxIdx, (int)rxLen, (int)rssi, (int)lqi, (long)rxTime);
#endif
#ifdef PRINT_PACKETS
    if (rxLen < 0) {
        PRINTF("RX failed\n");
    }
    else if (rxLen > 0 ) {
        debugHexdump((uint8_t *) &radioBuffer, rxLen);
        // debugHexdump((uint8_t *) &(DB_REC(db)), rxLen);
    }
#endif
    if (rxLen < 0) {
        led2Toggle();
        flRxProcessing=false;
        return;
    }

    if( ! MSG_SIGNATURE_OK(radioBuffer) ) { flRxProcessing = false; return; }

    // Anticipated payload types.
    MSG_NEW_PAYLOAD_PTR(radioBuffer, phaser_ping_t, test_data_p);
    MSG_NEW_PAYLOAD_PTR(radioBuffer, phaser_control_t, ctrl_data_p);
    MSG_NEW_PAYLOAD_PTR(radioBuffer, msg_text_data_t, msg_text_p);
    MSG_NEW_PAYLOAD_PTR(radioBuffer, test_config_t, test_config_p);

    int act = MSG_ACT_CLEAR;
    bool flOK=true;

    switch( radioBuffer.id ){
    case PH_MSG_Test:
        MSG_CHECK_FOR_PAYLOAD(radioBuffer, phaser_ping_t, flOK=false );
        if( !flOK ){
            PRINTF("BadChk\n");
            break;
        }
        // Check if new experiment iteration started.
        if(lastExpIdx != test_data_p->expIdx && curExp){
            sendTestResults();
        }
        processTestMsg(test_data_p, rssi, lqi);
        break;
    
    case PH_MSG_Angle:
        if(curExp) sendTestResults();
        if( flRestart ){        // Best time to resend the restart message after the angle change
            send_ctrl_msg(MSG_ACT_RESTART);
            flRestart = false;
        }
        break;

    case PH_MSG_Control:
        MSG_CHECK_FOR_PAYLOAD(radioBuffer, phaser_control_t, break);
        if(curExp) sendTestResults();

        act = ctrl_data_p->action;
        if(act == MSG_ACT_START ){
            flRestart = false;  // Clear restart command attempt
        }
        printAction(act);
        break;

    case PH_MSG_Text:
        MSG_CHECK_FOR_PAYLOAD(radioBuffer, msg_text_data_t, break );
        PRINTF(msg_text_p->text);
        PRINTF("\n");
        break;

    case PH_MSG_Config:
        MSG_CHECK_FOR_PAYLOAD(radioBuffer, test_config_t, break );
        PRINTF("Config received:\n");
        // TODO: parse the config and print
        print_test_config(test_config_p);
    }


    flRxProcessing=false;
}

// --------------------------------------------
// --------------------------------------------
void appMain(void)
{
    serialEnableRX(PRINTF_SERIAL_ID);
    // serialSetReceiveHandle(PRINTF_SERIAL_ID, onSerRecv);
    serialSetPacketReceiveHandle(PRINTF_SERIAL_ID, onSerRecv, serBuffer, SER_BUF_SIZE);

    radioSetReceiveHandle(onRadioRecv);
    radioOn();
    mdelay(200);

    // Send restart message to the phaser
    send_ctrl_msg(MSG_ACT_RESTART);

    while (1) {
        mdelay(RATE_DELAY);
        led0Toggle();
    }
}
