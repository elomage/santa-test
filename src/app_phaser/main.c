// --------------------------------------------
// Phaser node: digitally controlled beam forming 
// --------------------------------------------

#include "stdmansos.h"
#include "cc2420/cc2420.h"

#include "../phaser_msg.h"
#include "../msg_framework.h"
#include "antenna_driver.h"

// #define PH_COMMENT ""

// Comment to disable debug messages over serial port
#define DEBUG_PHASER 1

// Comment to disable debug messages over serial port
#define FL_TEST_RESTART_ON_END 1

#define RADIO_MAX_TX_POWER 31
#define RADIO_BUF_PAYLOAD_LEN RADIO_MAX_PACKET


//--- Test setup ----------------------
test_config_t test_config;

// Test iterator
test_loop_t testIdx = TEST_LOOP_INIT_VAL;

// Global configuration counter. Each config is defined in the testSet[] array.
static int config_counter=0;


//--- Global data -----------------------------------------------------------

bool fl_test_restart=true;
bool fl_test_stop=true;


angle_t lastAngle = ANGLE_NOT_SET_VALUE;
bool fl_AngleSet=false;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Define a buffer for receiving messages
MSG_DEFINE_BUFFER_WITH_ID(radioBuffer, recv_data_p, RADIO_BUF_PAYLOAD_LEN);

// Phaser configuration message
MSG_NEW_WITH_ID(ant_msg, phaser_ping_t, PH_MSG_Test);
phaser_ping_t *ant_cfg_p = &(ant_msg.payload);

// Phaser angle setting message
MSG_NEW_WITH_ID(angle_msg, phaser_angle_t, PH_MSG_Angle);

// Phaser control/report message
MSG_NEW_WITH_ID(ctrl_msg, phaser_control_t, PH_MSG_Control);

// Phaser test configuration message
MSG_NEW_WITH_ID(config_msg, test_config_t, PH_MSG_Config);

// Phaser test configuration message
MSG_NEW_WITH_ID(text_msg, msg_text_data_t, PH_MSG_Text);


// -------------------------------------------------------------------------
// Delay in ms, using a variable instead of constant.
// -------------------------------------------------------------------------
void mdelay_var(int ms)
{
    uint16_t k;
    for(k=ms; k>0; k--){
        mdelay(1);
    }
}


// -------------------------------------------------------------------------
// Setup the test run parameters
// -------------------------------------------------------------------------
void config_init()
{
    config_counter=0;   // Restart from the first stored configuration
    memcpy(&test_config, &(testSet[0]), sizeof(test_config_t));
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void send_test_config()
{
    MSG_COPY_AND_SEND(config_msg, &test_config);
}

// -------------------------------------------------------------------------
// Set up new configuration.
// Return "true" on success.
// -------------------------------------------------------------------------
bool config_new(test_config_t *newTest)
{
    int i;

    // Check test sanity
    if(newTest->start_delay > 20000) return false;
    if(newTest->send_delay > 10000) return false;

    for(i=0; i<TEST_CONFIG_POWER_LIST_SIZE; i++){
        if( newTest->power[i] > RADIO_MAX_TX_POWER ) return false;
    }

    if( !ant_test_sanity_check(newTest) ){
        return false;
    }

    memcpy(&test_config, newTest, sizeof(test_config));

    // Send the received config back, for verification.
    // send_test_config();

    return true;
}


// -------------------------------------------------------------------------
// Report phaser status: battery voltage etc.
// -------------------------------------------------------------------------
void reportStatus()
{
    send_test_config();
}

// -------------------------------------------------------------------------
// Send control message about the test.
// -------------------------------------------------------------------------
void send_ctrl_msg(msg_action_t act)
{
    int i;
    ctrl_msg.payload.action = act;
    MSG_DO_CHECKSUM( ctrl_msg );

    // Send 3 times for reliability.
    radioSetTxPower(RADIO_MAX_TX_POWER);
    mdelay(20);
    for(i=0; i<3; i++){
        MSG_RADIO_SEND( ctrl_msg );
        mdelay(100);
    }
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void send_string(char *str)
{
    int len = strlen(str);
    if(len>0 && len<MSG_TEXT_SIZE_MAX-1){
        memcpy(text_msg.payload.text, str, len);

        radioSetTxPower(RADIO_MAX_TX_POWER);
        MSG_RADIO_SEND( text_msg );
    }
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void test_start()
{
    send_string(ant_driver_name);

#ifdef PH_COMMENT
    send_string(PH_COMMENT);
#endif

    //Send config for the test to be run
    send_test_config();
    mdelay(100);    //give the receiver time to parse the config

    send_ctrl_msg(MSG_ACT_START);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void test_stop()
{
    // Send stop and set the stop flag
    fl_test_stop = true;
    fl_test_restart = true;

    send_ctrl_msg(MSG_ACT_IDLE);
}

// -------------------------------------------------------------------------
//  Radio reveive handler
// -------------------------------------------------------------------------
void onRadioRecv(void)
{
    static bool flRxProcessing=false;
    if( flRxProcessing ) return;
    flRxProcessing = true;

    static int rxLen;
    rxLen = MSG_RADIO_RECV(radioBuffer);
    if (rxLen < 0) {
        // led2Toggle();
        flRxProcessing=false;
        return;
    }

    if( ! MSG_SIGNATURE_OK(radioBuffer) ) {
        flRxProcessing=false;
        return;
    }

    // Anticipated payload types.
    MSG_NEW_PAYLOAD_PTR(radioBuffer, phaser_angle_t, angle_p);
    MSG_NEW_PAYLOAD_PTR(radioBuffer, phaser_control_t, control_p);
    MSG_NEW_PAYLOAD_PTR(radioBuffer, test_config_t, test_p);


    switch( radioBuffer.id ){
    case PH_MSG_Angle:
        // Eventually this should become >REQUEST <ACK <DONE
        if( angle_p->action == MSG_ACT_ACK ){
            fl_AngleSet = true;
        }
        break;

    case PH_MSG_Control:
        switch( control_p->action ){
        case MSG_ACT_RESTART:
            fl_test_restart = true;
            send_ctrl_msg(MSG_ACT_IDLE);
            break;
        case MSG_ACT_STOP:
            test_stop();
            break;
        case MSG_ACT_STATUS:
            reportStatus();
            break;
        }
        break;
    
    case PH_MSG_Config:
        config_new(test_p);
        fl_test_restart = true;
        break;
    }
    // Rx processing done
    flRxProcessing=false;
}

// -------------------------------------------------------------------------
// Set the physical angle of the antena module
// Return true if the angle was changed.
// -------------------------------------------------------------------------
bool set_angle( angle_t newAngle )
{
    if( newAngle==lastAngle && newAngle != ANGLE_NOT_SET_VALUE) return false;

    angle_msg.payload.angle = newAngle;
    angle_msg.payload.action = MSG_ACT_SET;
    MSG_DO_CHECKSUM( angle_msg );

    radioSetTxPower(RADIO_MAX_TX_POWER);

    fl_AngleSet=false;
    MSG_RADIO_SEND_FOR_ACK( angle_msg, fl_AngleSet );

    return( fl_AngleSet );
}

// -------------------------------------------------------------------------
// Setup the test run
// -------------------------------------------------------------------------
void test_init()
{
    // Init the test infrastructure
    lastAngle = ANGLE_NOT_SET_VALUE;    // Force stepper angle recalibration
    set_angle( -20 );
    set_angle( 0 );

    // Init the iterators
    testIdx.power.idx = 0;
    testIdx.angle.idx = 0;
    testIdx.angle.limit = test_config.angle_count;

    // Init the antena configuration 
    ant_cfg_p->expIdx = 0;
    ant_cfg_p->msgCounter = 0;
    ant_cfg_p->angle = 0;
    ant_cfg_p->power = test_config.power[0];

    ant_test_init(&testIdx, &test_config, ant_cfg_p);
}

// -------------------------------------------------------------------------
// Set up the next configuration
// -------------------------------------------------------------------------
bool next_config()
{
    test_config_t *cfg;

    if( ++config_counter >= testSet_size ){
        config_counter=0;
        return false;   // all configurations done
    }
    cfg = &(testSet[config_counter]);

    config_new(cfg);
    test_init();
    test_start();

    return true;
}

// -------------------------------------------------------------------------
// Calculate the next test step
// Return true when next iteration is ready
// Return false when done (no more iterations possible)
// -------------------------------------------------------------------------
bool test_next()
{
    ant_cfg_p->expIdx ++;

    // Next power
    testIdx.power.idx++;
    if( testIdx.power.idx < TEST_CONFIG_POWER_LIST_SIZE ){
        ant_cfg_p->power = test_config.power[testIdx.power.idx];
        if( ant_cfg_p->power > 0 ){
            return true; 
        }  
    }
    testIdx.power.idx = 0;
    ant_cfg_p->power = test_config.power[testIdx.power.idx];

    // Next hardware configuration
    if( ant_test_next_config(&testIdx, &test_config, ant_cfg_p) ){
        return true;
    }

    // Next angle
    testIdx.angle.idx++;
    if( testIdx.angle.idx >= testIdx.angle.limit ){
        testIdx.angle.idx = 0;
        ant_cfg_p->angle = 0;
    }
    else {
        ant_cfg_p->angle += test_config.angle_step;
        return true;
    }

    // Next test setup configuration
    send_ctrl_msg(MSG_ACT_DONE);    // Previous configuration done
    if( next_config() ) return true;

    return false;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void test_step()
{
    int i;
    uint8_t err;

#ifdef DEBUG_PHASER
    PRINTF("Do Send %d\n", (int)ant_cfg_p->expIdx);
#endif

    set_angle(ant_cfg_p->angle);

    ant_test_setup(ant_cfg_p);
    radioSetTxPower(ant_cfg_p->power);

    for(i=0; i<test_config.send_count; i++)
    {
        ant_cfg_p->timestamp = getTimeMs();
        ant_cfg_p->msgCounter ++;

        MSG_DO_CHECKSUM( ant_msg );
        err = MSG_RADIO_SEND(ant_msg);

#ifdef DEBUG_PHASER
        if(err<0){
            PRINTF("Idx=%d TX Error=%d\n", (int)ant_cfg_p->msgCounter, (int)err);
        }
#endif

        // Wait till send done
        mdelay(1);
        while( cc2420IsTxBusy() );

        mdelay_var(test_config.send_delay);
    }
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ledTestFinished()
{
    int i;
    for (i=0; i<3; i++)
    {
        ledOn();
        mdelay(100);
        ledOff();
        mdelay(100);
    }    
    mdelay(500);
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void appMain(void)
{
#ifdef DEBUG_PHASER
    PRINTF("Phaser started\n");
#endif
    // Blink on beginning, same sequence as on finish.
    ledTestFinished();

    ant_driver_init();

    radioSetReceiveHandle(onRadioRecv);
    radioOn();

    fl_test_stop = false;  

    while(1) 
    {
        config_init();  // Init the global configuration list
    
        test_init();
        test_start();
        
        mdelay_var( test_config.start_delay );
        fl_test_restart = false;
        
        while( !fl_test_restart && !fl_test_stop ) 
        {
            ledToggle();

            test_step();
            if( ! test_next() ){
                send_ctrl_msg(MSG_ACT_DONE);
                break;
            }

            if( ant_check_button() ) fl_test_restart = true;
        }
        // Test done!

#ifdef FL_TEST_RESTART_ON_END
        fl_test_restart = true;
#endif

        while( !fl_test_restart || fl_test_stop ) 
        {
            ledTestFinished();
            if( ant_check_button() ){
                fl_test_restart = true;
                fl_test_stop = false;  
            } 
        }
    }
}
