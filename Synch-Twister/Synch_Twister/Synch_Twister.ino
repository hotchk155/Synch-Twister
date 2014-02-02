#include "Arduino.h"
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
// USER INTERFACE HANDLING
//
////////////////////////////////////////////////////////

#define UI_MAXLEDARRAY  5         // number of LED arrays
#define UI_NUM_SWITCHES 6
#define UI_DEBOUNCE_MS 20         // key debounce
#define UI_AUTO_REPEAT_DELAY 500  // delay before key auto repeats
#define UI_AUTO_REPEAT_PERIOD 50  // delay between auto repeats
#define UI_DOUBLE_CLICK_TIME 200  // double click threshold

enum 
{
  UI_KEY_0      = 0x0001,
  UI_KEY_1      = 0x0002,
  UI_KEY_2      = 0x0004,
  UI_KEY_3      = 0x0008,
  UI_KEY_4      = 0x0010,
  UI_KEY_SELECT = 0x0020, // must be next bit after scanned keys
  
  UI_KEY_PRESS  = 0x0100,
  UI_KEY_HOLD   = 0x0200,
  UI_KEY_AUTO   = 0x0400,
  UI_KEY_DOUBLE = 0x0800
};

byte uiLEDState[UI_MAXLEDARRAY];       // the output bit patterns for the LEDs
byte uiDebounceCount[UI_NUM_SWITCHES];  // debounce registers for switches
unsigned int uiKeyStatus;              // input status for switches (The one other modules should read)
unsigned int uiLastKeyStatus;          
unsigned int uiLastKeypress;           
unsigned long uiDoubleClickTime; 
unsigned long uiAutoRepeatTime; 
byte uiLEDIndex;                   
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
void uiShowNumber(int n, int start=0)
{  
  byte xlatDigit[10]  = {  DGT_0,  DGT_1,  DGT_2,  DGT_3,  DGT_4,  DGT_5,  DGT_6,  DGT_7,  DGT_8,  DGT_9 };
  int div[4] = {1000, 100, 10, 1};
  if(start < 0 || start >= 4)
  {
    uiLEDState[0] = uiLEDState[1] = uiLEDState[2] = uiLEDState[3] = SEG_DP;
  }
  else
  {
    while(start < 4)
    {
      int divider = div[start];
      uiLEDState[start] = xlatDigit[n/divider]; 
      n%=divider;
      ++start;
    }
  }
}

////////////////////////////////////////////////////////
// Configure the LEDs to show a decimal number
void uiShowHex(unsigned int n)
{
  byte xlatDigit[16]  = {  DGT_0,  DGT_1,  DGT_2,  DGT_3,  DGT_4,  DGT_5,  DGT_6,  DGT_7,  DGT_8,  DGT_9, DGT_A, DGT_B, DGT_C, DGT_D, DGT_E, DGT_F };
  uiLEDState[0] = xlatDigit[n/0x1000]; n%=0x1000;
  uiLEDState[1] = xlatDigit[n/0x100]; n%=0x100;
  uiLEDState[2] = xlatDigit[n/0x10]; n%=0x10;
  uiLEDState[3] = xlatDigit[n]; 
}

////////////////////////////////////////////////////////
// Manage UI functions
void uiRun(unsigned long milliseconds)
{
  // Check the select key, which has its own IO pin
  if(!digitalRead(P_SELECT))
    uiSwitchStates |= UI_KEY_SELECT;
  else  
    uiSwitchStates &= ~UI_KEY_SELECT;
    
  
  // Manage switch debouncing...
  byte newKeyPress = 0;
  for(int i=0; i<UI_NUM_SWITCHES; ++i)
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
        uiKeyStatus |= mask;
        uiDebounceCount[i] = UI_DEBOUNCE_MS;        // debounce
        newKeyPress = 1;
      }
      else if(!(uiSwitchStates & mask) && (uiKeyStatus & mask)) // key pressed before, not now
      {
        uiKeyStatus &= ~mask;
        uiDebounceCount[i] = UI_DEBOUNCE_MS;  // debounce
      }
    }
  }
  
  // Manage auto repeat etc
  if(!(uiKeyStatus & 0x00FF)) // no keys pressed
  {
    uiKeyStatus = 0;
    uiAutoRepeatTime = 0; 
  }
  else if(uiKeyStatus != uiLastKeyStatus) // change in keypress
  {
    uiKeyStatus &= 0x00FF; // clear flags    
    if(newKeyPress)
    {
      if(uiKeyStatus == uiLastKeypress && milliseconds < uiDoubleClickTime)
        uiKeyStatus |= UI_KEY_DOUBLE;      
      uiLastKeypress = uiKeyStatus;
      uiDoubleClickTime = milliseconds + UI_DOUBLE_CLICK_TIME;
      uiKeyStatus |= UI_KEY_PRESS; 
    }
    uiAutoRepeatTime = milliseconds + UI_AUTO_REPEAT_DELAY;
  }
  else 
  {
    // keys held - not a new press
    uiKeyStatus &= ~(UI_KEY_PRESS|UI_KEY_DOUBLE|UI_KEY_AUTO);
    if(uiAutoRepeatTime < milliseconds)
    {
      if(uiKeyStatus & UI_KEY_HOLD)  // key flagged as held?
        uiKeyStatus |= UI_KEY_AUTO;  // now it is an auto repeat
      else
        uiKeyStatus |= UI_KEY_HOLD;  // otherwise flag as held
      uiAutoRepeatTime = milliseconds + UI_AUTO_REPEAT_PERIOD;
    }
  }
  uiLastKeyStatus = uiKeyStatus;  
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
  
  synchChannels[0].setOutputPin(P_CLKOUT0);
  synchChannels[1].setOutputPin(P_CLKOUT1);
  synchChannels[2].setOutputPin(P_CLKOUT2);
  synchChannels[3].setOutputPin(P_CLKOUT3);  
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

enum 
{
  MENU_KEY_SELECT = UI_KEY_SELECT,
  MENU_KEY_PREV   = UI_KEY_2,
  MENU_KEY_NEXT   = UI_KEY_3,
  MENU_KEY_DEC    = UI_KEY_4,
  MENU_KEY_INC    = UI_KEY_1,
  MENU_KEY_ENTER  = UI_KEY_0,
    
  MENU_KEY_A      = MENU_KEY_NEXT,
  MENU_KEY_B      = MENU_KEY_PREV,
  MENU_KEY_C      = MENU_KEY_INC,
  MENU_KEY_D      = MENU_KEY_DEC  
};

enum {  
  MENU_CHAN_MUTATION = 0,
  MENU_CHAN_PARAM1,
  MENU_CHAN_PARAM2,
  MENU_CHAN_PARAM3,
  MENU_CHAN_PARAM4,
  MENU_CHAN_STEPS,
  MENU_CHAN_DIV,
  MENU_CHAN_PULSEMS,
  MENU_CHAN_RECOVERMS,
  MENU_CHAN_INVERT,  
  MENU_CHAN_MAX
};

enum {
  MENU_CONTEXT_CHAN1  = 0,
  MENU_CONTEXT_CHAN2  = 1,
  MENU_CONTEXT_CHAN3  = 2,
  MENU_CONTEXT_CHAN4  = 3,
  MENU_CONTEXT_GLOBAL = 4
};

enum 
{
  MENU_LED_CHAN1 = 0x01,
  MENU_LED_CHAN2 = 0x02,
  MENU_LED_CHAN3 = 0x04,
  MENU_LED_CHAN4 = 0x08,
  MENU_LED_GLOBAL = 0x10
};

byte menuChannelParam;
byte menuContext;

void menuDisplayChannelParam()
{
  switch(menuChannelParam)
  {
    case MENU_CHAN_MUTATION:
      synchChannels[menuContext].getMutator()->getName(uiLEDState);
      break;
    case MENU_CHAN_PARAM1:
    case MENU_CHAN_PARAM2:
    case MENU_CHAN_PARAM3:
    case MENU_CHAN_PARAM4:
      {
        int whichParam = menuChannelParam - MENU_CHAN_PARAM1;
        byte prefix[4] = { DGT_1, DGT_2, DGT_3, DGT_4 };
        uiLEDState[0] = prefix[whichParam]|SEG_DP;
        if(whichParam >= synchChannels[menuContext].getMutator()->getNumParams())
        {
          uiLEDState[1] = uiLEDState[2] = uiLEDState[3] = DGT_DASH;
        }
        else
        {
          uiShowNumber(synchChannels[menuContext].getMutator()->getParam(whichParam), 1);
        }
      }
      break;    
    case MENU_CHAN_STEPS:      
          uiLEDState[0] = DGT_S;
          uiLEDState[1] = DGT_T|SEG_DP;
          uiShowNumber(synchChannels[menuContext].getParam(CSynchChannel::PARAM_STEPS), 2);
          break;                   
    case MENU_CHAN_DIV:
          uiLEDState[0] = DGT_D;
          uiLEDState[1] = DGT_I|SEG_DP;
          uiShowNumber(synchChannels[menuContext].getParam(CSynchChannel::PARAM_DIV), 2);
          break;                   
    case MENU_CHAN_PULSEMS:
          uiLEDState[0] = DGT_P;
          uiLEDState[1] = DGT_T|SEG_DP;
          uiShowNumber(synchChannels[menuContext].getParam(CSynchChannel::PARAM_PULSEMS), 2);
          break;                   
    case MENU_CHAN_RECOVERMS:
          uiLEDState[0] = DGT_R;
          uiLEDState[1] = DGT_T|SEG_DP;
          uiShowNumber(synchChannels[menuContext].getParam(CSynchChannel::PARAM_RECOVERMS), 2);
          break;                   
    case MENU_CHAN_INVERT:
          uiLEDState[0] = DGT_P;
          uiLEDState[1] = DGT_O|SEG_DP;
          if(synchChannels[menuContext].getParam(CSynchChannel::PARAM_INVERT))
          {
            uiLEDState[2] = DGT_L;
            uiLEDState[3] = DGT_O;
          }
          else
          {
            uiLEDState[2] = DGT_H;
            uiLEDState[3] = DGT_I;
          }
          break;                   
     default:
         uiLEDState[0] = uiLEDState[1] = uiLEDState[2] = uiLEDState[3] = DGT_0;
         break;
  }  
}

///////////////////////////////////////////////////////////////
// Select NEXT or PREV param
void menuSelectParam(byte next)
{
  switch(menuContext)
  {
      case MENU_CONTEXT_GLOBAL:
        break;
      case MENU_CONTEXT_CHAN1:
      case MENU_CONTEXT_CHAN2:
      case MENU_CONTEXT_CHAN3:
      case MENU_CONTEXT_CHAN4:
        if(next)
        { 
          if(menuChannelParam >= MENU_CHAN_MAX -1)
            menuChannelParam = 0;
          else
            ++menuChannelParam;
        }
        else
        {
          if(menuChannelParam <= 0)
            menuChannelParam = MENU_CHAN_MAX-1;
          else
            --menuChannelParam;
        }        
        menuDisplayChannelParam();        
        break;
  }        
}

///////////////////////////////////////////////////////////////
void menuChangeParam(byte inc)
{
  switch(menuChannelParam)
  {
    case MENU_CHAN_MUTATION:
      break;
    case MENU_CHAN_PARAM1:   synchChannels[menuContext].getMutator()->changeParam(0, inc); break;
    case MENU_CHAN_PARAM2:   synchChannels[menuContext].getMutator()->changeParam(1, inc); break;
    case MENU_CHAN_PARAM3:   synchChannels[menuContext].getMutator()->changeParam(2, inc); break;
    case MENU_CHAN_PARAM4:   synchChannels[menuContext].getMutator()->changeParam(3, inc); break;
    case MENU_CHAN_STEPS:    synchChannels[menuContext].changeParam(CSynchChannel::PARAM_STEPS, inc); break;
    case MENU_CHAN_DIV:      synchChannels[menuContext].changeParam(CSynchChannel::PARAM_DIV, inc); break;
    case MENU_CHAN_PULSEMS:  synchChannels[menuContext].changeParam(CSynchChannel::PARAM_PULSEMS, inc); break;
    case MENU_CHAN_RECOVERMS:synchChannels[menuContext].changeParam(CSynchChannel::PARAM_RECOVERMS, inc); break;
    case MENU_CHAN_INVERT:   synchChannels[menuContext].changeParam(CSynchChannel::PARAM_INVERT, inc); break;
  }
  menuDisplayChannelParam();
};
    
///////////////////////////////////////////////////////////////
// Select a different meny
void menuSetContext(byte context)
{
  menuContext = context;

  byte leds = uiLEDState[4] & 
    ~(MENU_LED_CHAN1|MENU_LED_CHAN2|MENU_LED_CHAN3|MENU_LED_CHAN4|MENU_LED_GLOBAL);
  switch(menuContext)
  {
      case MENU_CONTEXT_GLOBAL: leds |= MENU_LED_GLOBAL; break;
      case MENU_CONTEXT_CHAN1:  leds |= MENU_LED_CHAN1; break;
      case MENU_CONTEXT_CHAN2:  leds |= MENU_LED_CHAN2; break;
      case MENU_CONTEXT_CHAN3:  leds |= MENU_LED_CHAN3; break;
      case MENU_CONTEXT_CHAN4:  leds |= MENU_LED_CHAN4; break;
  }
  uiLEDState[4] = leds;
  menuDisplayChannelParam();
}


///////////////////////////////////////////////////////////////
// Run the menu
void menuRun(unsigned long milliseconds)
{
  switch(uiKeyStatus)
  {
    case UI_KEY_PRESS|MENU_KEY_SELECT|MENU_KEY_A: // Select + A
      menuSetContext(MENU_CONTEXT_CHAN1);
      break;
    case UI_KEY_PRESS|MENU_KEY_SELECT|MENU_KEY_B: // Select + B
      menuSetContext(MENU_CONTEXT_CHAN2);
      break;
    case UI_KEY_PRESS|MENU_KEY_SELECT|MENU_KEY_C: // Select + C
      menuSetContext(MENU_CONTEXT_CHAN3);
      break;
    case UI_KEY_PRESS|MENU_KEY_SELECT|MENU_KEY_D: // Select + D
      menuSetContext(MENU_CONTEXT_CHAN4);
      break;
      
    //////
    
    case UI_KEY_PRESS|MENU_KEY_PREV:
      menuSelectParam(0);
      break;
    case UI_KEY_PRESS|MENU_KEY_NEXT:
      menuSelectParam(1);
      break;
      
    case UI_KEY_PRESS|MENU_KEY_DEC:
    case UI_KEY_PRESS|UI_KEY_DOUBLE|MENU_KEY_DEC:
    case UI_KEY_HOLD|UI_KEY_AUTO|MENU_KEY_DEC:
      menuChangeParam(0);
      break;
    case UI_KEY_PRESS|MENU_KEY_INC:
    case UI_KEY_PRESS|UI_KEY_DOUBLE|MENU_KEY_INC:
    case UI_KEY_HOLD|UI_KEY_AUTO|MENU_KEY_INC:
      menuChangeParam(1);
      break;
  }
}
void menuInit()
{
  menuChannelParam = 0;
  menuSetContext(MENU_CONTEXT_CHAN1);
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
  uiInit();     
  menuInit();
  sei();  
}



unsigned long prevMilliseconds = 0;
void loop()
{
  unsigned long milliseconds = millis();
  if(prevMilliseconds != milliseconds)
  {
    prevMilliseconds = milliseconds;
    synchRun(milliseconds);
    heartBeatRun(milliseconds);
    uiRun(milliseconds);
    menuRun(milliseconds);
  }
}


