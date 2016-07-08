/**
 * Copyright (c) 2008-2014 the MansOS team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of  conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// --------------------------------------------
// Phaser node: digitally controlled beam forming 
// The driver.
// --------------------------------------------

#include "stdmansos.h"
#include "phaser_hw.h"      // Depends on the platform "phaser"

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
test_config_t testSet[] = {
    {
        .platform_id = PLATFORM_ID,     // Short test
        .start_delay = 1000,
        .send_delay  = 1,
        .send_count  = 100,
        .angle_step  = 5,
        .angle_count = 40,
        .ant.phaseA.start = 0,
        .ant.phaseA.step  = 32,
        .ant.phaseA.count = 0,
        .ant.phaseB.start = 0,
        .ant.phaseB.step  = 32,
        .ant.phaseB.count = 8,
        .power = {31, 0}
    },
    {
        .platform_id = PLATFORM_ID,     // Longer test
        .start_delay = 1000,
        .send_delay  = 5,
        .send_count  = 100,
        .angle_step  = 5,
        .angle_count = 40,
        .ant.phaseA.start = 0,
        .ant.phaseA.step  = 32,
        .ant.phaseA.count = 0,
        .ant.phaseB.start = 0,
        .ant.phaseB.step  = 32,
        .ant.phaseB.count = 8,
        .power = {31, 23, 15, 7, 3, 0}
    }
};
const size_t testSet_size = sizeof(testSet)/sizeof(testSet[0]);

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ant_driver_init()
{
    PHASER_INIT();
    PHASER_SET_PARALLEL();
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
    testIdx->phaseA.limit = test_config->ant.phaseA.count;
    testIdx->phaseB.idx = 0;
    testIdx->phaseB.limit = test_config->ant.phaseB.count;

    // Init the antena configuration 
    ant_cfg_p->ant.phaseA = test_config->ant.phaseA.start;
    ant_cfg_p->ant.phaseB = test_config->ant.phaseB.start;
}



// -------------------------------------------------------------------------
// Setup the test run
// Return true when next iteration is ready
// Return false when done (no more iterations possible)
// -------------------------------------------------------------------------
bool ant_test_next_config(test_loop_t *testIdx, test_config_t *test_config, phaser_ping_t *ant_cfg_p)
{
    // Next phase
    if(test_config->ant.phaseA.count){
        testIdx->phaseA.idx++;
        if( testIdx->phaseA.idx >= testIdx->phaseA.limit ){
            testIdx->phaseA.idx = 0;
            ant_cfg_p->ant.phaseA = test_config->ant.phaseA.start;
        }
        else {
            ant_cfg_p->ant.phaseA += test_config->ant.phaseA.step;
            return true;
        }
    } 
    if(test_config->ant.phaseB.count){
        testIdx->phaseB.idx++;
        if( testIdx->phaseB.idx >= testIdx->phaseB.limit ){
            testIdx->phaseB.idx = 0;
            ant_cfg_p->ant.phaseB = test_config->ant.phaseB.start;
        }
        else {
            ant_cfg_p->ant.phaseB += test_config->ant.phaseB.step;
            return true;
        }
    }
    return false;
}


// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void ant_test_setup(phaser_ping_t *ant_cfg_p)
{
    PHASER_A_SET(ant_cfg_p->ant.phaseA);
    PHASER_B_SET(ant_cfg_p->ant.phaseB);
    PHASER_WAIT_SETTLE();
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
