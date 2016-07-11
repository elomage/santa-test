// --------------------------------------------
// Phaser node: digitally controlled beam forming 
// The driver.
// Compile target: telosb
// --------------------------------------------

#include "stdmansos.h"
#include "digital.h"

#include "../phaser_msg.h"

#include "antenna_driver.h"

// Hardware definitions
PIN_DEFINE(TmoteBtn, 2, 7);

// -------------------------------------------------------------------------
// Driver name and ID
// -------------------------------------------------------------------------
char *ant_driver_name = "TelosB";
#define PLATFORM_ID  PH_TELOSB


// -------------------------------------------------------------------------
// Set of test configurations that should be executed
// -------------------------------------------------------------------------
test_config_t testSet[] = {
    {
        .platform_id = PLATFORM_ID,     // Short test
        .start_delay = 1000,
        .send_delay  = 5,
        .send_count  = 32,
        .angle_step  = 5,
        .angle_count = 40,
        .ant.phaseA.count = 0,
        .ant.phaseB.count = 0,
        .power = {31, 0}
    },
    {
        .platform_id = PLATFORM_ID,     // Longer test
        .start_delay = 1000,
        .send_delay  = 5,
        .send_count  = 100,
        .angle_step  = 5,
        .angle_count = 40,
        .ant.phaseA.count = 0,
        .ant.phaseB.count = 0,
        .power = {31, 23, 15, 7, 3, 0}
    }
};
const size_t testSet_size = sizeof(testSet)/sizeof(testSet[0]);


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ant_driver_init()
{
    //Init Button
    TmoteBtnAsInput();      \
    return;
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
bool ant_test_sanity_check(test_config_t *newTest)
{    
    return true;
}


// -------------------------------------------------------------------------
// Setup the test run (Antena parameters)
// -------------------------------------------------------------------------
void ant_test_init(test_loop_t *testIdx, test_config_t *test_config, phaser_ping_t *ant_cfg_p)
{
    //Nothing to do
    return;
}



// -------------------------------------------------------------------------
// Setup the test run
// Return true when next iteration is ready
// Return false when done (no more iterations possible)
// -------------------------------------------------------------------------
bool ant_test_next_config(test_loop_t *testIdx, test_config_t *test_config, phaser_ping_t *ant_cfg_p)
{
    //Nothing to do
    return false;
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ant_test_setup(phaser_ping_t *ant_cfg_p)
{
    //Nothing to do
    return;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ledBtnDown()
{
    int i;
    for (i=0; i<3; i++)
    {
        ledOn();
        mdelay(100);
        ledOff();
        mdelay(100);
    }    
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
bool ant_check_button()
{
    if( TmoteBtnRead() )   return false;
    ledBtnDown();
    while( !TmoteBtnRead() ){   // Wait untill button released
        ledBtnDown();
    } 

    return true;
}
