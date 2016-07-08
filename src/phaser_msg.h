/* 
 * Phaser Message protocol
 *
 *
 */
  
#ifndef _phaser_msg_h_
#define _phaser_msg_h_

#include "stdint.h"
#include "msg_framework.h"
#include "stream_stat.h"





//===========================================
// Phaser message types
//===========================================
enum {
    PH_MSG_Phase = 'P',
    PH_MSG_Angle = 'A',
    PH_MSG_Control = 'C',
    PH_MSG_Config = 'G',
    PH_MSG_Test = 'T',      // Test message, like ping, but with configuration
    PH_MSG_Text = 'X',
};


//===========================================
// Phaser platform types
//===========================================
typedef enum {
    PH_TELOSB,
    PH_PHASER,
    PH_PHASERTX,
    PH_SANTA,
} platform_id_t;

// Platform name from ID
#define PH_PLATFORM_NAME( platform_id )  ( \
    (platform_id)==PH_TELOSB ? "TelosB" :  \
    (platform_id)==PH_PHASER ? "Phaser" :  \
    (platform_id)==PH_PHASERTX ? "PhaserTX" :  \
    (platform_id)==PH_SANTA ? "Santa" :  \
    "?")

//===========================================
// Configuration parameters
//===========================================

// RSSI and TX power types
typedef int8_t rssi_t;
typedef int8_t lqi_t;
typedef uint8_t tx_power_t;

// structure for the iterator
typedef struct
{
    int idx;
    int limit;
} loop_int_t;

// Default value (0). Use to init variables of this type.
#define LOOP_INT_0 { 0,0 }


// Phase test setup
typedef struct 
{
    uint8_t start;    
    uint8_t step;    
    uint16_t count;    
} iter8_config_t;



// Angle configuration type
typedef uint16_t angle_t;
enum { ANGLE_NOT_SET_VALUE= 0xffff };

// Phase configuration type
typedef uint8_t phase_t;
enum { phase_max_c = 0xff };


// Test iterators
typedef struct
{
    loop_int_t power;
    loop_int_t phaseA;
    loop_int_t phaseB;
    loop_int_t angle;
} test_loop_t;

// Default value (0). Use to init variables of this type.
#define TEST_LOOP_INIT_VAL { LOOP_INT_0,LOOP_INT_0,LOOP_INT_0,LOOP_INT_0 }

#define TEST_CONFIG_POWER_LIST_SIZE  8

//--------------------------------------------
// Antenna state variables, union by platform
// This defines the state and is updated during the each test run
typedef union 
{
    struct {
        uint16_t i16;
    };
    struct {        // Phaser
        uint8_t phaseA;
        uint8_t phaseB;
    };
    struct {        // PhaserTX
        uint8_t phase;
        uint8_t attenuation;
    };
    struct {        // Santa
        uint8_t santa_pins;
        uint8_t santa_extra;
    };
} ant_state_t;

// Antena configuration parameters, union by platform
// These define the test parameters, limits, iteration count, etc.
typedef union 
{
    struct {        // Phaser
        iter8_config_t phaseA;
        iter8_config_t phaseB;
    };
    struct {        // PhaserTX
        iter8_config_t phase;
        iter8_config_t attenuation;
    };
    struct {        // Santa
        iter8_config_t santa_pins;
        uint32_t santa_extra;
    };
} ant_test_config_t;


// Test run setup
typedef struct 
{
    uint8_t platform_id;    
    uint16_t start_delay;    // delay before the test starts in ms
    uint16_t send_count;    // Number of experiments per each configuration
    uint16_t send_delay;    // delay between test message sends in ms
    uint16_t angle_step;
    uint16_t angle_count;
    tx_power_t power[TEST_CONFIG_POWER_LIST_SIZE];
    ant_test_config_t ant;
} test_config_t;


//===========================================
// Messages
//===========================================

typedef struct 
{
    msg_timestamp_t timestamp;
    uint16_t msgCounter;
    uint16_t expIdx;     // Experiment index/counter
    angle_t angle;

    ant_state_t ant;
    // phase_t phaseA;
    // phase_t phaseB;

    uint8_t power;       // cc2420: 0(min) - 31(max)

} __attribute__((packed)) 
phaser_ping_t;
// phaser_config_t;

typedef struct
{
    angle_t angle;
    msg_action_t action;
} __attribute__((packed)) 
phaser_angle_t;

typedef struct
{
    msg_action_t action;
} __attribute__((packed)) 
phaser_control_t;


//===========================================
// Experimental data
//===========================================

STREAM_STAT_DECLARE(rssi_data_t, int32_t);
STREAM_STAT_DECLARE(lqi_data_t, int32_t);

typedef struct 
{
    // int expIdx;
    tx_power_t power;
    angle_t angle;
    phase_t phase;
    rssi_data_t rssi_data;
    lqi_data_t lqi_data;
} experiment_t;


#endif // _phaser_msg_h_
