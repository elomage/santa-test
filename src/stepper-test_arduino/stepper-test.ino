/* LED Blink, Teensyduino Tutorial #1
   http://www.pjrc.com/teensy/tutorial.html
 
   This example code is in the public domain.
*/

//===== Prototypes =====
void myDelay();

//===== Data =====
// Comment below to enable the stepper controller all the time
#define LOWPOWER

#define BLINKRATE 200
#define DELAY_STEP 3

#define DELAY_START 10
#define DELAY_BRAKE 200

#define PINDIR  14
#define PINSTEP 15
#define PINM1   16
#define PINM2   17
#define PINM3   18
#define PINEN   19

// Teensy 3.0 has the LED on pin 13
const int ledPin = 13;

int cnt=0;

//-------------------------------------------
void stepperInit()
{
  pinMode(PINDIR, OUTPUT);
  pinMode(PINSTEP, OUTPUT);
  pinMode(PINEN, OUTPUT);
  pinMode(PINM1, OUTPUT);
  pinMode(PINM2, OUTPUT);
  pinMode(PINM3, OUTPUT);

//  digitalWrite(PINM1, HIGH);
//  digitalWrite(PINM2, HIGH);
//  digitalWrite(PINM3, HIGH);
  digitalWrite(PINM1, LOW);
  digitalWrite(PINM2, LOW);
  digitalWrite(PINM3, LOW);

  digitalWrite(PINDIR, LOW);
  digitalWrite(PINSTEP, LOW);
  digitalWrite(PINEN, LOW);
}

//--------------------
void stepperBrake()
{
    digitalWrite(PINEN, LOW);
    delay(DELAY_START);
}

//--------------------
void stepperRelease()
{
    delay(DELAY_BRAKE);
    digitalWrite(PINEN, HIGH);
}

//--------------------
// Rotate the stepper N steps
//--------------------
void step(int steps)
{
    // Enable
#ifdef LOWPOWER
    stepperBrake();
#endif
  
    for(int i=0; i<steps; i++){
      digitalWrite(PINSTEP, HIGH);
      delay(DELAY_STEP);
      digitalWrite(PINSTEP, LOW);
      delay(DELAY_STEP);
    }
    // Disable, to save power
#ifdef LOWPOWER
    stepperRelease();
#endif
}


//===========================================
// the setup() method runs once, when the sketch starts
//===========================================
void setup() {
  // initialize the digital pin as an output.
  pinMode(ledPin, OUTPUT);

  stepperInit();
  stepperRelease();
    
  Serial.begin(9600); // USB is always 12 Mbit/sec
}

//===========================================
// the loop() methor runs over and over again,
// as long as the board has power
//===========================================
void loop() {
  int ch;
  
  digitalWrite(ledPin, HIGH);   // set the LED on
  myDelay();             // wait 
  digitalWrite(ledPin, LOW);    // set the LED off
  myDelay();             // wait 


// /*  
//  Serial.print("Hello World1...");
  Serial.print(cnt);
  //ch = Serial.read();
  ch = Serial.parseInt();
  Serial.print("    InByte=");
  Serial.println(ch, DEC);

  // Check the direction
  if( ch<0 ){
    digitalWrite(PINDIR, HIGH);
    ch = -ch;
  } else if( ch>0) {
    digitalWrite(PINDIR, LOW);
  }

  // Rotate
  if (ch>0){
    step(ch);
  }

  
/* === */
}

//--------------------
void myDelay()
{
  delay(BLINKRATE);             // wait for a second
  cnt += BLINKRATE; 
}

