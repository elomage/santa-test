/* 
 *  Stepper control driver
 */

#ifndef _STEPPER_H_
#define _STEPPER_H_
  
#include "stdmansos.h"

// =========================================================================
// Stepper Hardware driver
// =========================================================================

#define StepperAngleMax 200     // number of steps for a full circle


void stepperInit();

// Check the position sensor and return true if at zero point
bool stepperSenseZero();

// Rotate the stepper until it reashes zero angle
// Return true if succeeded
bool stepperZero();

// Rotate the stepper N steps in whatever direction was set before
void step(int steps);

// Rotate the stepper N steps relative to the current position. Can be negative.
void stepRelative(int steps);

// Rotate to the absolute angle from the zero point, in steps.
// Return false if already there or invalid input.
bool stepAbsolute(int angle);

#endif // _STEPPER_H_
