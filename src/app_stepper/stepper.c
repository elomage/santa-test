/* 
 *  Stepper control driver
 */
  
#include "stdmansos.h"
#include "stepper.h"

// =========================================================================
// Stepper Hardware driver
// =========================================================================
// Comment LOWPOWER define to enable the stepper controller all the time
// LOWPOWER will disable breaking/holding the position when not moving.
// Otherwise there is high current consumprtion when idle.
#define LOWPOWER

#define DELAY_STEP 10          // mdelay between steps (100)

#define DELAY_START 10      // wait (brake) before moving (10)
#define DELAY_BRAKE 200     // wait (brake) after moving (200)


static int lastAngle = 1;    // Last known position

// Stepper port and pins for MCU

PIN_DEFINE(StPinEn, 6, 0);
PIN_DEFINE(StPinStep, 6, 1);
PIN_DEFINE(StPinDir, 6, 2);

// Hall sensor pin
PIN_DEFINE(StPinSense, 6, 6);

// -------------------------------------------------------------------------
void stepperBrake()
{
    StPinEnLow();
    mdelay(DELAY_START);
}

// -------------------------------------------------------------------------
void stepperRelease()
{
    mdelay(DELAY_BRAKE);
    StPinEnHigh();
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void stepperInit()
{
    StPinDirAsOutput();
    StPinStepAsOutput();
    StPinEnAsOutput();

    StPinDirLow();
    StPinStepLow();
    StPinEnLow();

    StPinSenseAsInput();

    stepperRelease();
}

// -------------------------------------------------------------------------
// Rotate the stepper N steps in whatever direction was set before
// -------------------------------------------------------------------------
void step(int steps)
{
    int i;

    // Enable brakes while stepping
#ifdef LOWPOWER
    stepperBrake();
#endif
  
    for(i=0; i<steps; i++){
        StPinStepHigh();
        mdelay(DELAY_STEP);
        StPinStepLow();
        mdelay(DELAY_STEP);
    }
    // Disable brakes, to save power
#ifdef LOWPOWER
    stepperRelease();
#endif
}

// -------------------------------------------------------------------------
// Check the position sensor and return true if at zero point
// -------------------------------------------------------------------------
bool stepperSenseZero()
{
    bool fl = (StPinSenseRead() == 0);
    return fl;
}

// -------------------------------------------------------------------------
// Rotate the stepper until it reashes zero angle
// Return true if succeeded
// -------------------------------------------------------------------------
bool stepperZero()
{
    int i;

    // If close to sensor, move away first for better position.
    // if( stepperSenseZero() ) step(StepperAngleMax/2);
    if( stepperSenseZero() ) stepRelative(-20);


    // Enable brakes while stepping
#ifdef LOWPOWER
    stepperBrake();
#endif
    for(i=0; i<StepperAngleMax; i++)
    {
        StPinStepHigh();
        mdelay(DELAY_STEP);
        StPinStepLow();
        mdelay(DELAY_STEP);
        // Check the zero point sensor. Stop if found.

        // fl = stepperSenseZero();
        // if( fl ) led2On();
        // else led2Off();

        if(stepperSenseZero()){
            lastAngle = 0;
#ifdef LOWPOWER
            stepperRelease();
#endif
            return true;
        }
    }
    // Disable brakes, to save power
#ifdef LOWPOWER
    stepperRelease();
#endif

    // Could not locate the zero point sensor
    return false;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
void stepRelative(int steps) {

    // Check the direction
    if( steps<0 ){
        StPinDirHigh();
        steps = -steps;
    } else if( steps>0) {
        StPinDirLow();
    }

    // Rotate
    if (steps>0){
        step(steps);
    }
    // restore the direction to forward
    StPinDirLow();
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
bool stepAbsolute(int angle)
{
    int steps=0;

    if( angle == lastAngle ){
        return false;  
    } 

    if( angle >= StepperAngleMax ||  angle <= -StepperAngleMax ){
        return false;
    }
    
    if( angle == 0 ){
        return stepperZero();
    }

    steps = angle - lastAngle;
    if( steps >= StepperAngleMax ) steps -= StepperAngleMax;
    if( steps <= -StepperAngleMax ) steps += StepperAngleMax;
    stepRelative( steps );
    
    // Convert to positive angle
    if( angle < 0 ) angle += StepperAngleMax;
    lastAngle = angle;
    return true;
}


// =========================================================================
// =========================================================================
