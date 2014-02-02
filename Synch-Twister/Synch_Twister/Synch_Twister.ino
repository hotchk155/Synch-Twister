#include "Synch_Twister.h"
#include "Mutators.h"
#include "SynchChannel.h"

////////////////////////////////////////////////////////
//
// DEFINE IO PINS
//
////////////////////////////////////////////////////////
#define P_CLKOUT0 9
#define P_CLKOUT1 10
#define P_CLKOUT2 11
#define P_CLKOUT3 12

#define P_DIGIT0  16
#define P_DIGIT1  15
#define P_DIGIT2  14
#define P_DIGIT3  17
#define P_DIGIT4  7

#define P_SHCLK  19
#define P_SHDAT  18
#define P_SWREAD 8
#define P_HEARTBEAT 13
#define P_SELECT 2

#define CBIT_DIGIT0  (1<<2) 
#define CBIT_DIGIT1  (1<<1)
#define CBIT_DIGIT2  (1<<0)
#define CBIT_DIGIT3  (1<<3)
#define DBIT_DIGIT4  (1<<7)
#define CBIT_SHCLK   (1<<5)
#define CBIT_SHDAT   (1<<4)
#define BBIT_SWREAD  (1<<0)
#define DBIT_SELECT  (1<<2)


////////////////////////////////////////////////////////
//
// SEVEN SEGMENT LED DISPLAY DEFINITIONS
//
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
//
// USER INTERFACE HANDLING
//
////////////////////////////////////////////////////////

#define UI_MAXLEDARRAY  5         // number of LED arrays
#define UI_DEBOUNCE_MS 20         // key debounce
#define UI_AUTO_REPEAT_DELAY 500  // delay before key auto repeats
#define UI_AUTO_REPEAT_PERIOD 50  // delay between auto repeats

/*
// Flags used in Key Status
enum {
  UI_KEYPRESS  = 0x01,
  UI_KEYHELD   = 0x02,
  UI_KEYREPEAT = 0x04,
  UI_NEW_KEY_PRESS = (UI_KEYPRESS|UI_KEYREPEAT)
};

enum {
  UI_KEY1 = 0,
  UI_KEY2 = 1,
  UI_KEY3 = 2,
  UI_KEY4 = 3,
  UI_KEY5 = 4
};
*/

enum 
{
  UI_KEY_0      = 0x01,
  UI_KEY_1      = 0x02,
  UI_KEY_2      = 0x04,
  UI_KEY_3      = 0x08,
  UI_KEY_4      = 0x10,
  UI_KEY_SELECT = 0x20,
  UI_KEY_PRESS  = 0x40,
  UI_KEY_AUTO   = 0x80
};

byte uiLEDState[UI_MAXLEDARRAY];       // the output bit patterns for the LEDs
byte uiDebounceCount[UI_MAXLEDARRAY];  // debounce registers for switches
byte uiKeyStatus;                      // input status for switches
unsigned long uiAutoRepeatTime; 
byte uiLEDIndex = 0;                   
byte uiSwitchStates;
byte uiLastSwitchStates;              

////////////////////////////////////////////////////////
// Initialise variables
void uiInit()
{
  memset(uiLEDState, 0, sizeof(uiLEDState));
  memset(uiDebounceCount, 0, sizeof(uiDebounceCount));
  uiKeyStatus = 0;
  uiLEDIndex = 0;
  uiSwitchStates = 0;
  uiLastSwitchStates = 0;
  uiAutoRepeatTime = 0;
  
  // start the interrupt to service the UI   
  TCCR2A = 0;
  TCCR2B = 1<<CS22 | 0<<CS21 | 1<<CS20; // control refresh rate of display
  TIMSK2 = 1<<TOIE2;
  TCNT2 = 0; 
}

////////////////////////////////////////////////////////
// Interrupt service routing that refreshes the LEDs
ISR(TIMER2_OVF_vect) 
{
  // Read the switch status (do it now, rather than on previous
  // tick so we can ensure adequate setting time)
  if(PINB & BBIT_SWREAD)
    uiSwitchStates |= (1<<uiLEDIndex);
  else
    uiSwitchStates &= ~(1<<uiLEDIndex);
    
//  uiInputState[uiLEDIndex] = !!
  
  // Turn off all displays  
  PORTC &= ~(CBIT_DIGIT0|CBIT_DIGIT1|CBIT_DIGIT2|CBIT_DIGIT3);
  PORTD &= ~(DBIT_DIGIT4);

  // Unrolled loop to load shift register
  byte d = uiLEDState[uiLEDIndex];
  // bit 7    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x80) PORTC |= CBIT_SHDAT; else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 6
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x40) PORTC |= CBIT_SHDAT; else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 5    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x20) PORTC |= CBIT_SHDAT; else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 4    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x10) PORTC |= CBIT_SHDAT; else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 3    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x08) PORTC |= CBIT_SHDAT; else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 2    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x04) PORTC |= CBIT_SHDAT; else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 1    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x02) PORTC |= CBIT_SHDAT; else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 0
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x01) PORTC |= CBIT_SHDAT; else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // flush    
  PORTC &= ~CBIT_SHCLK;
  PORTC |= CBIT_SHCLK;
  
  // Turn on the appropriate display
  switch(uiLEDIndex)
  {
    case 0: PORTC |= CBIT_DIGIT0; break;
    case 1: PORTC |= CBIT_DIGIT1; break;
    case 2: PORTC |= CBIT_DIGIT2; break;
    case 3: PORTC |= CBIT_DIGIT3; break;
    case 4: PORTD |= DBIT_DIGIT4; break;
  }
  
  // Next pass we'll check the next display
  if(++uiLEDIndex >= UI_MAXLEDARRAY)
    uiLEDIndex = 0;
}

////////////////////////////////////////////////////////
// Configure the LEDs to show a decimal number
void uiShowNumber(int n)
{
  byte xlatDigit[10]  = {  DGT_0,  DGT_1,  DGT_2,  DGT_3,  DGT_4,  DGT_5,  DGT_6,  DGT_7,  DGT_8,  DGT_9 };
  uiLEDState[0] = xlatDigit[n/1000]; n%=1000;
  uiLEDState[1] = xlatDigit[n/100]; n%=100;
  uiLEDState[2] = xlatDigit[n/10]; n%=10;
  uiLEDState[3] = xlatDigit[n]; 
}

////////////////////////////////////////////////////////
// Manage UI functions
void uiRun(unsigned long milliseconds)
{
  // Check for auto repeats
  byte autoRepeat = 0;
  if(!uiSwitchStates)
  {
    uiAutoRepeatTime = 0; 
    uiLastSwitchStates = 0;
  }
  else if(uiLastSwitchStates != uiSwitchStates)
  {
    uiAutoRepeatTime = milliseconds + UI_AUTO_REPEAT_DELAY;
    uiLastSwitchStates = uiSwitchStates;
  }
  else if(uiAutoRepeatTime < milliseconds)
  {
    uiAutoRepeatTime = milliseconds + UI_AUTO_REPEAT_PERIOD;
    autoRepeat = 1;
  }
  
  // manage each switch
  for(int i=0; i<UI_MAXLEDARRAY; ++i)
  {    
    byte mask = 1<<i;
    
    // are we debouncing?
    if(uiDebounceCount[i])
    {
      --uiDebounceCount[i];
    }
    else
    {      
      if((uiSwitchStates & mask) && !(uiKeyStatus & mask)) // key pressed now, was not before
      {
        uiKeyStatus |= mask | UI_KEY_PRESS;   // new key press
        uiKeyStatus &= ~UI_KEY_AUTO;          // not auto repeating
        uiDebounceCount[i] = UI_DEBOUNCE_MS;  // debounce
      }
      else if(!(uiSwitchStates & mask) && (uiKeyStatus & mask)) // key pressed before, not now
      {
        uiKeyStatus &= ~(mask | UI_KEY_PRESS | UI_KEY_AUTO);   // unpress
        uiDebounceCount[i] = UI_DEBOUNCE_MS;  // debounce
      }
      else if(autoRepeat && (uiKeyStatus & mask))
      {
        uiKeyStatus |= UI_KEY_AUTO; // auto repeat
      }
    }
  }
}




/*
// swing 0-100
void straightSwing(int *steps, double swing)
{
  int t = 0;
  for(int i=0;i<MAX_STEPS;)
  {    
    steps[i++] = t;
    t += TICKS_PER_STEP;
    steps[i++] = t + (swing * TICKS_PER_STEP)/100;
    t += TICKS_PER_STEP;
  }
}
*/




/*
  we maintain a "tick count" which is a division of musical time (fraction of a beat)
  rather than absolute time, so it remains valid after changes to the BPM
  
  Each tick is 1/96 of a quarter note
  1/24 of a step
  at 120bpm
  96 ticks per beat
  2 beats per second
  500ms / 96
  ~5ms
*/
#define NUM_CHANNELS 4
CSynchChannel synchChannels[NUM_CHANNELS];
double synchMillisPerTick; 
double synchNextTick;
double synchBPM;
unsigned long synchLastMilliseconds;

void synchSetBPM(double b)
{
  synchBPM = b;
  double millisecondsPerBeat = 60000.0/synchBPM;
  synchMillisPerTick = millisecondsPerBeat/96.0;
}

void synchInit()
{
  synchSetBPM(120);
  synchNextTick = 0;
  synchLastMilliseconds = 0;
  
  synchChannels[0].OutputPin = P_CLKOUT0;
  synchChannels[1].OutputPin = P_CLKOUT1;
  synchChannels[2].OutputPin = P_CLKOUT2;
  synchChannels[3].OutputPin = P_CLKOUT3;  
}

// Run the ticker outputs
void synchRun(unsigned long milliseconds)
{
  int i;
  // has the millisecond timer rolled over
  if(synchLastMilliseconds > milliseconds)
  {
    // make sure the tickers don't lock up
    synchChannels[0].timerRollover();
    synchChannels[1].timerRollover();
    synchChannels[2].timerRollover();
    synchChannels[3].timerRollover();
    synchNextTick = 0;
  }
  synchLastMilliseconds = milliseconds;
  
  // Time for the next tick?
  if(synchNextTick < milliseconds)
  {
    synchNextTick += synchMillisPerTick;
    if(synchNextTick < milliseconds)
      synchNextTick = milliseconds + synchMillisPerTick;     
    for(i=0;i<NUM_CHANNELS;++i)
    {
      synchChannels[i].tick();
      synchChannels[i].run(milliseconds);
    }
  }
  else
  {
    for(i=0;i<NUM_CHANNELS;++i)
      synchChannels[i].run(milliseconds);
  }
}



////////////////////////////////////////////////////////
//
// MENU 
//
////////////////////////////////////////////////////////

#define MENU_GLOBAL (-1)
int menuChannel = MENU_GLOBAL;



void menuInit()
{
}

void menuRun(unsigned long milliseconds)
{
  if(!digitalRead(P_SELECT))
  {
  }
}



#define HEARTBEAT_PERIOD 200
byte heartBeatState;
unsigned long nextHeartBeat;
void heartBeatInit()
{
  heartBeatState = 0;
  nextHeartBeat = 0;
  pinMode(P_HEARTBEAT,OUTPUT);
}

void heartBeatRun(unsigned long milliseconds)
{
  if(milliseconds > nextHeartBeat)
  {
    nextHeartBeat = milliseconds + 200;
    heartBeatState = !heartBeatState;
    digitalWrite(P_HEARTBEAT,heartBeatState);
  }
}

void setup()
{
  pinMode(P_DIGIT0, OUTPUT);
  pinMode(P_DIGIT1, OUTPUT);
  pinMode(P_DIGIT2, OUTPUT);
  pinMode(P_DIGIT3, OUTPUT);
  pinMode(P_DIGIT4, OUTPUT);
  pinMode(P_SHCLK, OUTPUT);
  pinMode(P_SHDAT, OUTPUT);
  pinMode(P_SWREAD, INPUT);
  pinMode(P_SELECT, INPUT);

  digitalWrite(P_DIGIT0, LOW);
  digitalWrite(P_DIGIT1, LOW);
  digitalWrite(P_DIGIT2, LOW);
  digitalWrite(P_DIGIT3, LOW);
  digitalWrite(P_DIGIT4, LOW);
  digitalWrite(P_SHCLK, LOW);
  digitalWrite(P_SHDAT, LOW);
  digitalWrite(P_SELECT, HIGH); //Weak pull-up

  pinMode(P_CLKOUT0,OUTPUT);
  pinMode(P_CLKOUT1,OUTPUT);
  pinMode(P_CLKOUT2,OUTPUT);
  pinMode(P_CLKOUT3,OUTPUT);
  
  digitalWrite(P_CLKOUT0,HIGH);
  digitalWrite(P_CLKOUT1,HIGH);
  digitalWrite(P_CLKOUT2,HIGH);
  digitalWrite(P_CLKOUT3,HIGH);      

  cli();
  synchInit();  
  heartBeatInit();
  menuInit();
  uiInit();     
  sei();  
}



int n = 999;
unsigned long prevMilliseconds = 0;
void loop()
{
  unsigned long milliseconds = millis();
  if(prevMilliseconds != milliseconds)
  {
    prevMilliseconds = milliseconds;
    synchRun(milliseconds);
    heartBeatRun(milliseconds);
    //uiShowNumber(W.Intensity);
    uiRun(milliseconds);
    menuRun(milliseconds);
/*    if(uiKeyStatus[0] & UI_NEW_KEY_PRESS)
    {
      uiKeyStatus[0]&=~UI_NEW_KEY_PRESS;
      if(W.Intensity > 1) W.Intensity--;
    }
    if(uiKeyStatus[1] & UI_NEW_KEY_PRESS)
    {
      uiKeyStatus[1]&=~UI_NEW_KEY_PRESS;
      if(W.Intensity < 99) W.Intensity++;
    }*/
  }
}

/*
  Each channel 
    Mutation Type     Shuf      rAnd    Strt   CUrv
    Mutation Param 1  1.000
    Mutation Param 2  2.000
    Mutation Param 3  3.000
    Mutation Param 4  4.000
    Step Count before pattern repeats  St.99
    Step Divisor  dIv.1
    Pulse Time      t.100
    Recovery Time   r.100
    Invert          Inv.0
  
*/


