// --------------------------------------------
// PhaserTx node: digitally controlled beam forming 
// The driver.
// Compile and upload target: phaser
//
// --------------------------------------------

#include "stdmansos.h"

#include "../phaser_msg.h"

#include "antenna_driver.h"


// -------------------------------------------------------------------------
// Driver name and ID
// -------------------------------------------------------------------------
char *ant_driver_name = "Phaser";
#define PLATFORM_ID  PH_PHASER


// -------------------------------------------------------------------------
// Set of test configurations that should be executed
// -------------------------------------------------------------------------
//
// Configuration parameters: 
//   Phase: 6bits, ~180deg, across Channel 1 and 2
//   Attenuation: 4 bits, 0dB - 7.5dB on channel 2

test_config_t testSet[] = {
    {
        .platform_id = PLATFORM_ID,     // Short test
        .start_delay = 1000,
        .send_delay  = 1,
        .send_count  = 100,
        .angle_step  = 5,
        .angle_count = 40,
        .ant.phase.start = 0,
        .ant.phase.step  = 8,
        .ant.phase.count = 8,
        .ant.attenuation.start = 0,
        .ant.attenuation.step  = 4,
        .ant.attenuation.count = 1,
        .power = {31, 0}
    },
    {
        .platform_id = PLATFORM_ID,     // Longer test
        .start_delay = 1000,
        .send_delay  = 5,
        .send_count  = 100,
        .angle_step  = 5,
        .angle_count = 40,
        .ant.phase.start = 0,
        .ant.phase.step  = 8,
        .ant.phase.count = 8,
        .ant.attenuation.start = 0,
        .ant.attenuation.step  = 4,
        .ant.attenuation.count = 4,
        .power = {31, 23, 15, 7, 3, 0}
    }
};
const size_t testSet_size = sizeof(testSet)/sizeof(testSet[0]);

// -------------------------------------------------------------------------
// Hardware definitions
// -------------------------------------------------------------------------

PIN_DEFINE(RFCLK_, 2, 3);
PIN_DEFINE(RFSDI_, 2, 6);
PIN_DEFINE(RFSDO_, 2, 0);
PIN_DEFINE(RFLE_,  2, 7);

// Serial map: 
// 14bit word: S0 S1 P0-P4 M0-M3 S2 S3 C0
// M0-M3 - attenuation:  4 + 2 + 1 + 0.5 dB
// P0-P4 - phase shift:  45 + 22.5 + 11.2 + 5.6 + 2.8 deg
// C0 - channel register select 
//      Channels: RFOUT1: phase only;  RFOUT2: attenuation and phase
// S* - spare bits = X

// Timing:
// CLK frequency: 32KHz - 26MHz
// CLK period min: 40ns
// CLK min high and min low times: 20ns
// LE min high width: 10ns
// SDI setup max: 5ns
// SDI hold max: 2ns
// CLK rising to LE rising (Tsettle) max = 27ns


PIN_DEFINE(PhaserBtn, 1, 2);
#define PHASER_BTN_DOWN()  (!PhaserBtnRead())
#define PHASER_BTN_UP()  (PhaserBtnRead())


// Delay at least 20ns per step (PE46120 serial comm, see datasheet)
#define STEP_DELAY()  nop()

// -------------------------------------------------------------------------
//  Setup PE46120 chip, one channel
// -------------------------------------------------------------------------
void pe46120_setup_channel(uint8_t phase, uint8_t attenuation, uint8_t channel)
{
    // Setup the 14-bit serial word
    uint16_t word = 0;
    uint16_t x = 0;
    x = phase & 0x1f;
    word |= (x << 2);
    x = attenuation & 0x0f;
    word |= (x << 7);
    x = (channel & 0x01);
    word |= (x << 13);

    // Send the serial word
    RFCLK_Low();
    RFLE_Low();
    STEP_DELAY();
    for(x=0; x<14; x++){
        RFSDI_Write(word & 0x01);
        STEP_DELAY();
        RFCLK_High();
        STEP_DELAY();
        RFCLK_Low();
        word = word >> 1;
    }
    STEP_DELAY();
    STEP_DELAY();
    RFLE_High();
    STEP_DELAY();
}

// -------------------------------------------------------------------------
//  Setup PE46120 chip, both channels
// -------------------------------------------------------------------------
void pe46120_setup(uint8_t phase1, uint8_t phase2, uint8_t attenuation2)
{
    pe46120_setup(phase1, 0, 0);
    pe46120_setup(phase2, attenuation2, 1);
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ant_driver_init()
{
    RFCLK_AsOutput();
    RFLE_AsOutput();
    RFSDI_AsOutput();
    RFSDO_AsInput();

    RFLE_High();
    RFCLK_Low();
    RFSDI_Low();

    //Send initial config
    pe46120_setup(0, 0, 0);
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
    testIdx->phaseA.limit = test_config->ant.phase.count;
    testIdx->phaseB.idx = 0;
    testIdx->phaseB.limit = test_config->ant.attenuation.count;

    // Init the antena configuration 
    ant_cfg_p->ant.phase = test_config->ant.phase.start;
    ant_cfg_p->ant.attenuation = test_config->ant.attenuation.start;
}


// -------------------------------------------------------------------------
// Setup the test run
// Return true when next iteration is ready
// Return false when done (no more iterations possible)
// -------------------------------------------------------------------------
bool ant_test_next_config(test_loop_t *testIdx, test_config_t *test_config, phaser_ping_t *ant_cfg_p)
{
    // Next phase
    if(test_config->ant.phase.count){
        testIdx->phaseA.idx++;
        if( testIdx->phaseA.idx >= testIdx->phaseA.limit ){
            testIdx->phaseA.idx = 0;
            ant_cfg_p->ant.phase = test_config->ant.phase.start;
        }
        else {
            ant_cfg_p->ant.phase += test_config->ant.phase.step;
            return true;
        }
    } 
    if(test_config->ant.attenuation.count){
        testIdx->phaseB.idx++;
        if( testIdx->phaseB.idx >= testIdx->phaseB.limit ){
            testIdx->phaseB.idx = 0;
            ant_cfg_p->ant.attenuation = test_config->ant.attenuation.start;
        }
        else {
            ant_cfg_p->ant.attenuation += test_config->ant.attenuation.step;
            return true;
        }
    }
    return false;
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ant_test_setup(phaser_ping_t *ant_cfg_p)
{
    // Build the config for the chip. Send over the serial port
    uint8_t p1, p2;

    // Version 1 of phase encoding by the configuration.
    // Use p1 for -90 or ~0=(-2.8) deg only. 
    // Thus the configuration phase may be used as a continious 6-bit number 
    // for phase range between ~0 - ~180 deg (2.8 - 177.2 deg to be exact).
    p1 = ((ant_cfg_p->ant.phase >> 5) & 0x01);  
    p1 = ( p1==0 ) ? 0x10 : 0x00;

    p2 = (ant_cfg_p->ant.phase & 0x1f);

    // Version 2 of phase encoding by the configuration.
    // Note, we loose LSB for each phase. Less resolution but wider range.
    // p1 = ((ant_cfg_p->ant.phase >> 4) & 0x0f) << 1;
    // p2 = (ant_cfg_p->ant.phase & 0x0f) << 1;


    pe46120_setup( p1, p2, ant_cfg_p->ant.attenuation);

    mdelay(1);
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
    if( PHASER_BTN_UP() )   return false;
    ledBtnDown();
    while( PHASER_BTN_DOWN() ){   // Wait untill button released
        ledBtnDown();
    } 

    return true;
}
