////////////////////////////////////////////////////////
//
// SYNCH TWISTER
//
////////////////////////////////////////////////////////

#include "Arduino.h"
#include "TinyUI.h"
#include "Synch_Twister.h"
#include "Mutators.h"
#include "SynchChannel.h"

////////////////////////////////////////////////////////
// Create a mutator.. hook up new mutators here
CMutator *createMutator(byte which)
{
  switch(which)
  {
  case MUTATOR_NULL:    
    return new CNullMutator; 
  case MUTATOR_SHUFFLE: 
    return new CShuffleMutator;
  case MUTATOR_RANDOM:  
    return new CRandomMutator;
  }  
  return new CNullMutator; 
}





////////////////////////////////////////////////////////
//
// USER INTERFACE HANDLING
//
////////////////////////////////////////////////////////






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
enum {
  SYNCH_STOP,
  SYNCH_RUN,
  SYNCH_RESET
};
enum {
  SYNCH_SOURCE_INTERNAL,
  SYNCH_SOURCE_MIDI,
  SYNCH_SOURCE_CV,
  SYNCH_SOURCE_MAX
};
CSynchChannel synchChannels[NUM_CHANNELS];
double synchMillisPerTick; 
double synchNextTick;
double synchBPM;
unsigned long synchLastMilliseconds;
byte synchState;
byte synchSource;
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
  synchState = SYNCH_STOP;
  synchSource = SYNCH_SOURCE_INTERNAL; 

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
    for(i=0;i<NUM_CHANNELS;++i)
      synchChannels[i].timerRollover();
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

void synchReset()
{
}


////////////////////////////////////////////////////////
//
// MENU 
//
////////////////////////////////////////////////////////

enum 
{
  MENU_KEY_SELECT = TUI_KEY_A,
  MENU_KEY_PREV   = TUI_KEY_2,
  MENU_KEY_NEXT   = TUI_KEY_3,
  MENU_KEY_DEC    = TUI_KEY_4,
  MENU_KEY_INC    = TUI_KEY_1,
  MENU_KEY_ENTER  = TUI_KEY_0,

  MENU_KEY_A      = MENU_KEY_NEXT,
  MENU_KEY_B      = MENU_KEY_PREV,
  MENU_KEY_C      = MENU_KEY_INC,
  MENU_KEY_D      = MENU_KEY_DEC,
  MENU_KEY_GLOBAL = MENU_KEY_ENTER  
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
  MENU_GLOBAL_RUN = 0,
  MENU_GLOBAL_BPM,
  MENU_GLOBAL_SYNCH,
  MENU_GLOBAL_MAX  
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
  MENU_LED_CHAN1 = TUI_LED_0,
  MENU_LED_CHAN2 = TUI_LED_1,
  MENU_LED_CHAN3 = TUI_LED_2,
  MENU_LED_CHAN4 = TUI_LED_3,
  MENU_LED_GLOBAL = TUI_LED_4
};

byte menuParam;
byte menuContext;


void menuDisplayGlobalParam()
{
  switch(menuParam)
  {
  case MENU_GLOBAL_RUN:
    switch(synchState)
    {
    case SYNCH_STOP:
      TUI.show(DGT_S, DGT_T, DGT_O, DGT_P);
      break;
    case SYNCH_RUN:
      TUI.show(DGT_G, DGT_O, SEG_B|SEG_DP);
      break;
    case SYNCH_RESET:
      TUI.show(DGT_R, DGT_S, DGT_T);
      break;
    }
    break;
  case MENU_GLOBAL_BPM:
    TUI.show(DGT_T|SEG_DP);
    TUI.showNumber(synchBPM,1);
    break;
  case MENU_GLOBAL_SYNCH:
    switch(synchSource)
    {
    case SYNCH_SOURCE_INTERNAL:
      TUI.show(DGT_S|SEG_DP, DGT_I, DGT_N, DGT_T);
      break;
    case SYNCH_SOURCE_MIDI:
      TUI.show(DGT_S|SEG_DP, DGT_M, DGT_I, DGT_D);
      break;
    case SYNCH_SOURCE_CV:
      TUI.show(DGT_S|SEG_DP, DGT_E, DGT_X, DGT_T);
      break;
    }
    break;
  }
}

void menuDisplayChannelParam()
{
  switch(menuParam)
  {
  case MENU_CHAN_MUTATION:
    {
      byte prefix[4] = { 
        DGT_A, DGT_B, DGT_C, DGT_D       };
      byte buf[3];
      synchChannels[menuContext].getMutator()->getName(buf);
      TUI.show(prefix[menuContext]|SEG_DP, buf[0], buf[1], buf[2]);
    }
    break;
  case MENU_CHAN_PARAM1:
  case MENU_CHAN_PARAM2:
  case MENU_CHAN_PARAM3:
  case MENU_CHAN_PARAM4:
    {
      int whichParam = menuParam - MENU_CHAN_PARAM1;
      byte prefix[4] = { 
        DGT_1, DGT_2, DGT_3, DGT_4       };
      TUI.show(prefix[whichParam]|SEG_DP);
      if(whichParam >= synchChannels[menuContext].getMutator()->getNumParams())      
        TUI.show(DGT_DASH, DGT_DASH, DGT_DASH, DGT_DASH);
      else
        TUI.showNumber(synchChannels[menuContext].getMutator()->getParam(whichParam), 1);
    }
    break;    
  case MENU_CHAN_STEPS:      
    TUI.show(DGT_S, DGT_T|SEG_DP);
    TUI.showNumber(synchChannels[menuContext].getParam(CSynchChannel::PARAM_STEPS), 2);
    break;                   
  case MENU_CHAN_DIV:
    TUI.show(DGT_D, DGT_I|SEG_DP);
    TUI.showNumber(synchChannels[menuContext].getParam(CSynchChannel::PARAM_DIV), 2);
    break;                   
  case MENU_CHAN_PULSEMS:
    TUI.show(DGT_P, DGT_T|SEG_DP);
    TUI.showNumber(synchChannels[menuContext].getParam(CSynchChannel::PARAM_PULSEMS), 2);
    break;                   
  case MENU_CHAN_RECOVERMS:
    TUI.show(DGT_R, DGT_T|SEG_DP);
    TUI.showNumber(synchChannels[menuContext].getParam(CSynchChannel::PARAM_RECOVERMS), 2);
    break;                   
  case MENU_CHAN_INVERT:
    if(synchChannels[menuContext].getParam(CSynchChannel::PARAM_INVERT))
      TUI.show(DGT_P, DGT_O|SEG_DP, DGT_L, DGT_O);
    else
      TUI.show(DGT_P, DGT_O|SEG_DP, DGT_H, DGT_I);
    break;                   
  default:
    TUI.show(DGT_0, DGT_0, DGT_0, DGT_0);
    break;
  }  
}

///////////////////////////////////////////////////////////////
void menuDisplayParam()
{
  switch(menuContext)
  {
  case MENU_CONTEXT_GLOBAL:
    menuDisplayGlobalParam();
    break;
  case MENU_CONTEXT_CHAN1:
  case MENU_CONTEXT_CHAN2:
  case MENU_CONTEXT_CHAN3:
  case MENU_CONTEXT_CHAN4:
    menuDisplayChannelParam();
    break;
  }
}

///////////////////////////////////////////////////////////////
// Select NEXT or PREV param
void menuSelectParam(byte next)
{
  switch(menuContext)
  {
    /////////////////////////////
  case MENU_CONTEXT_GLOBAL:
    for(;;)
    {
      if(next)
      {
        if(menuParam >= MENU_GLOBAL_MAX - 1)
          break;
        else
          ++menuParam;
      }
      else
      {
        if(menuParam <= 0)
          break;
        else
          --menuParam;
      }
      if(menuParam != MENU_GLOBAL_BPM || synchSource == SYNCH_SOURCE_INTERNAL) // skip BPM option when not using internal synch
        break;
    }
    break;

    /////////////////////////////
  case MENU_CONTEXT_CHAN1:
  case MENU_CONTEXT_CHAN2:
  case MENU_CONTEXT_CHAN3:
  case MENU_CONTEXT_CHAN4:
    {
      bool paramOK;
      byte numMutatorParams = synchChannels[menuContext].getMutator()->getNumParams();
      do {
        if(next)
        { 
          if(menuParam >= MENU_CHAN_MAX -1)
            break;
          else
            ++menuParam;
        }
        else
        {
          if(menuParam <= 0)
            break;
          else
            --menuParam;
        }

        switch(menuParam)
        {
        case MENU_CHAN_PARAM1:   
          paramOK = (numMutatorParams > 0); 
          break;
        case MENU_CHAN_PARAM2:   
          paramOK = (numMutatorParams > 1); 
          break;
        case MENU_CHAN_PARAM3:   
          paramOK = (numMutatorParams > 2); 
          break;
        case MENU_CHAN_PARAM4:   
          paramOK = (numMutatorParams > 3); 
          break;
        default:
          paramOK = 1;
        }          
      } 
      while (!paramOK);      
      break;
    }
  }        
  menuDisplayParam();        
}

///////////////////////////////////////////////////////////////
void menuChangeParam(byte inc)
{
  switch(menuContext)
  {
  case MENU_CONTEXT_GLOBAL:
    switch(menuParam)
    {
    case MENU_GLOBAL_RUN:
      switch(synchState)
      {
      case SYNCH_STOP:
        if(inc) 
          synchState = SYNCH_RUN;
        else 
        {
          synchReset();
          synchState = SYNCH_RESET;
        }
        break;
      case SYNCH_RUN:
        if(!inc) synchState = SYNCH_STOP;
        break;
      case SYNCH_RESET:
        if(inc) synchState = SYNCH_RUN;
        break;
      }
    case MENU_GLOBAL_BPM:
      if(inc && synchBPM < 350) synchSetBPM(synchBPM+1);
      else if(!inc && synchBPM > 1) synchSetBPM(synchBPM-1);
      break;
    case MENU_GLOBAL_SYNCH:
      break;
    }
    break;
  case MENU_CONTEXT_CHAN1:
  case MENU_CONTEXT_CHAN2:
  case MENU_CONTEXT_CHAN3:
  case MENU_CONTEXT_CHAN4:
    switch(menuParam)
    {
    case MENU_CHAN_MUTATION: 
      synchChannels[menuContext].changeParam(CSynchChannel::PARAM_MUTATION, inc); 
      break;
    case MENU_CHAN_PARAM1:   
      synchChannels[menuContext].getMutator()->changeParam(0, inc); 
      break;
    case MENU_CHAN_PARAM2:   
      synchChannels[menuContext].getMutator()->changeParam(1, inc); 
      break;
    case MENU_CHAN_PARAM3:   
      synchChannels[menuContext].getMutator()->changeParam(2, inc); 
      break;
    case MENU_CHAN_PARAM4:   
      synchChannels[menuContext].getMutator()->changeParam(3, inc); 
      break;
    case MENU_CHAN_STEPS:    
      synchChannels[menuContext].changeParam(CSynchChannel::PARAM_STEPS, inc); 
      break;
    case MENU_CHAN_DIV:      
      synchChannels[menuContext].changeParam(CSynchChannel::PARAM_DIV, inc); 
      break;
    case MENU_CHAN_PULSEMS:  
      synchChannels[menuContext].changeParam(CSynchChannel::PARAM_PULSEMS, inc); 
      break;
    case MENU_CHAN_RECOVERMS:
      synchChannels[menuContext].changeParam(CSynchChannel::PARAM_RECOVERMS, inc); 
      break;
    case MENU_CHAN_INVERT:   
      synchChannels[menuContext].changeParam(CSynchChannel::PARAM_INVERT, inc); 
      break;
    }
    break;
  }
  menuDisplayParam();        
};

///////////////////////////////////////////////////////////////
// Select a different meny
void menuSetContext(byte context)
{
  menuContext = context;

  TUI.clearLEDs(MENU_LED_CHAN1|MENU_LED_CHAN2|MENU_LED_CHAN3|MENU_LED_CHAN4|MENU_LED_GLOBAL);
  switch(menuContext)
  {
  case MENU_CONTEXT_GLOBAL: 
    TUI.setLEDs(MENU_LED_GLOBAL,MENU_LED_GLOBAL); 
    break;
  case MENU_CONTEXT_CHAN1:  
    TUI.setLEDs(MENU_LED_CHAN1,MENU_LED_CHAN1); 
    break;
  case MENU_CONTEXT_CHAN2:  
    TUI.setLEDs(MENU_LED_CHAN2,MENU_LED_CHAN2); 
    break;
  case MENU_CONTEXT_CHAN3:  
    TUI.setLEDs(MENU_LED_CHAN3,MENU_LED_CHAN3); 
    break;
  case MENU_CONTEXT_CHAN4:  
    TUI.setLEDs(MENU_LED_CHAN4,MENU_LED_CHAN4); 
    break;
  }
  menuParam = 0;
  menuDisplayParam();
}




///////////////////////////////////////////////////////////////
// Run the menu
void menuKeyPressHandler(unsigned int keyStatus)
{
  switch(keyStatus)
  {
  case TUI_PRESS|MENU_KEY_SELECT|MENU_KEY_A: // Select + A
    menuSetContext(MENU_CONTEXT_CHAN1);
    break;
  case TUI_PRESS|MENU_KEY_SELECT|MENU_KEY_B: // Select + B
    menuSetContext(MENU_CONTEXT_CHAN2);
    break;
  case TUI_PRESS|MENU_KEY_SELECT|MENU_KEY_C: // Select + C
    menuSetContext(MENU_CONTEXT_CHAN3);
    break;
  case TUI_PRESS|MENU_KEY_SELECT|MENU_KEY_D: // Select + D
    menuSetContext(MENU_CONTEXT_CHAN4);
    break;
  case TUI_PRESS|MENU_KEY_SELECT|MENU_KEY_GLOBAL: // Select + GLOBAL
    menuSetContext(MENU_CONTEXT_GLOBAL);
    break;

    //////

  case TUI_PRESS|MENU_KEY_PREV:
    menuSelectParam(0);
    break;
  case TUI_PRESS|MENU_KEY_NEXT:
    menuSelectParam(1);
    break;

  case TUI_PRESS|MENU_KEY_DEC:
  case TUI_DOUBLE|MENU_KEY_DEC:
  case TUI_AUTO|MENU_KEY_DEC:
    menuChangeParam(0);
    break;
  case TUI_PRESS|MENU_KEY_INC:
  case TUI_DOUBLE|MENU_KEY_INC:
  case TUI_AUTO|MENU_KEY_INC:
    menuChangeParam(1);
    break;
  }
}

///////////////////////////////////////////////////////////////
void menuInit()
{
  menuSetContext(MENU_CONTEXT_CHAN1);
}

///////////////////////////////////////////////////////////////
//
//                H E A R T B E A T
//
///////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////
//
//                E N T R Y    P O I N T S
//
///////////////////////////////////////////////////////////////
void setup()
{
  pinMode(P_SELECT, INPUT);
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
  TUI.init();     
  TUI.setExtraKey(TUI_KEY_A, P_SELECT);
  TUI.setKeypressHandler(menuKeyPressHandler);
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
    TUI.run(milliseconds);
  }
}




