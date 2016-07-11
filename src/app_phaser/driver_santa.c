// --------------------------------------------
// Santa node: digitally controlled beam forming 
// The driver.
// Compile target: santa
// --------------------------------------------

#include "stdmansos.h"
#include "santa_hw.h"      // Depends on the platform "phaser"

#include "../phaser_msg.h"

#include "antenna_driver.h"


// -------------------------------------------------------------------------
// Driver name and ID
// -------------------------------------------------------------------------
char *ant_driver_name = "Santa";
#define PLATFORM_ID  PH_SANTA

uint8_t santa_pins_list[] = 
{
    0b00000000,
    0b00001001,
    0b00010010,
    0b00100100,
    0b00111111,

    0b00000001,
    0b00000010,
    0b00000100,
    0b00001000,
    0b00010000,
    0b00100000,

    0b00000011,
    0b00000110,
    0b00001100,
    0b00011000,
    0b00110000,
    0b00100001,

    0b00000101,
    0b00001010,
    0b00010100,
    0b00101000,
    0b00010001,
    0b00100010,

    0b00000111,
    0b00001110,
    0b00011100,
    0b00111000,
    0b00110001,
    0b00100011,
};

#define santa_pins_list_size (sizeof(santa_pins_list)/sizeof(santa_pins_list[0]))

// -------------------------------------------------------------------------
// Set of test configurations that should be executed
// -------------------------------------------------------------------------
test_config_t testSet[] = {
    {
        .platform_id = PLATFORM_ID,     // Short test
        .start_delay = 1000,
        .send_delay  = 1,
        .send_count  = 100,
        .angle_step  = 5,
        .angle_count = 40,
        .ant.santa_pins.start = 0,
        .ant.santa_pins.step = 1,
        .ant.santa_pins.count = 4,
        .ant.santa_extra  = 0,
        .power = {31, 0}
    },
    {
        .platform_id = PLATFORM_ID,     // Longer test
        .start_delay = 1000,
        .send_delay  = 5,
        .send_count  = 100,
        .angle_step  = 5,
        .angle_count = 40,
        .ant.santa_pins.start = 0,
        .ant.santa_pins.step = 1,
        .ant.santa_pins.count = 4,      // Up to 2^6
        .ant.santa_extra  = 0,
        .power = {31, 23, 15, 7, 3, 0}
    }
};
const size_t testSet_size = sizeof(testSet)/sizeof(testSet[0]);

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ant_driver_init()
{
    SantaPinInit();
    SantaPinSetCfg(0);
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
    // Init the iterators
    testIdx->phaseA.idx = 0;
    testIdx->phaseA.limit = test_config->ant.santa_pins.count;
    testIdx->phaseB.idx = 0;
    testIdx->phaseB.limit = 0;

    // Init the antena configuration 
    ant_cfg_p->ant.santa_pins = test_config->ant.santa_pins.start;
    ant_cfg_p->ant.santa_extra = 0;
}


// -------------------------------------------------------------------------
// Setup the test run
// Return true when next iteration is ready
// Return false when done (no more iterations possible)
// -------------------------------------------------------------------------
bool ant_test_next_config(test_loop_t *testIdx, test_config_t *test_config, phaser_ping_t *ant_cfg_p)
{
    // Next phase
    if(test_config->ant.santa_pins.count){
        testIdx->phaseA.idx++;
        if( testIdx->phaseA.idx >= testIdx->phaseA.limit ){
            testIdx->phaseA.idx = 0;
            ant_cfg_p->ant.santa_pins = test_config->ant.santa_pins.start;
        }
        else {
            // Calculate the next config for the antenna
            // For small config count use preset configs, for large just increment
            if(test_config->ant.santa_pins.count > santa_pins_list_size){
                ant_cfg_p->ant.santa_pins += test_config->ant.santa_pins.step;
            }
            else {
                ant_cfg_p->ant.santa_pins = santa_pins_list[testIdx->phaseA.idx];
            }
            return true;
        }
    }
    return false;
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ant_test_setup(phaser_ping_t *ant_cfg_p)
{
    SantaPinSetCfg(ant_cfg_p->ant.santa_pins);
    mdelay(1);  // Wait a bit for the config to settle
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
    // if( PHASER_BTN_UP() )   return false;
    // ledBtnDown();
    // while( PHASER_BTN_DOWN() ){   // Wait untill button released
    //     ledBtnDown();
    // } 
    // return true;
    return false;
}
