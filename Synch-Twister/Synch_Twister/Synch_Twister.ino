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

#define CBIT_DIGIT0  (1<<2) 
#define CBIT_DIGIT1  (1<<1)
#define CBIT_DIGIT2  (1<<0)
#define CBIT_DIGIT3  (1<<3)
#define DBIT_DIGIT4  (1<<7)
#define CBIT_SHCLK   (1<<5)
#define CBIT_SHDAT   (1<<4)
#define BBIT_SWREAD  (1<<0)






// Map LED segments to shift register bits
//
//  aaa
// f   b
//  ggg
// e   c
//  ddd
#define SEG_A    0x80
#define SEG_B    0x01
#define SEG_C    0x04
#define SEG_D    0x10
#define SEG_E    0x20
#define SEG_F    0x40
#define SEG_G    0x02
#define SEG_DP   0x08

// Define the LED segment patterns for digits
#define DGT_0 (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F)
#define DGT_1 (SEG_B|SEG_C)
#define DGT_2 (SEG_A|SEG_B|SEG_D|SEG_E|SEG_G)
#define DGT_3 (SEG_A|SEG_B|SEG_C|SEG_D|SEG_G)
#define DGT_4 (SEG_B|SEG_C|SEG_F|SEG_G)
#define DGT_5 (SEG_A|SEG_C|SEG_D|SEG_F|SEG_G)
#define DGT_6 (SEG_A|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_7 (SEG_A|SEG_B|SEG_C)
#define DGT_8 (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_9 (SEG_A|SEG_B|SEG_C|SEG_F|SEG_G)

#define UI_KEYPRESS  1
#define UI_KEYHELD   2
#define UI_KEYREPEAT 4
#define UI_NEW_KEY_PRESS (UI_KEYPRESS|UI_KEYREPEAT)

#define UI_DEBOUNCE_MS 20
#define UI_MAXLEDARRAY  5
byte uiLEDState[UI_MAXLEDARRAY];    // the output bit patterns for the LEDs
byte uiDebounceCount[UI_MAXLEDARRAY];  // input status for switches
byte uiKeyStatus[UI_MAXLEDARRAY];
byte uiLEDIndex = 0;
byte uiSwitchStates;
byte uiLastSwitchStates;
unsigned long uiAutoRepeatTime;
#define UI_AUTO_REPEAT_DELAY 500
#define UI_AUTO_REPEAT_PERIOD 50


void uiInit()
{
  memset(uiLEDState, 0, sizeof(uiLEDState));
  memset(uiDebounceCount, 0, sizeof(uiDebounceCount));
  memset(uiKeyStatus, 0, sizeof(uiKeyStatus));
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

// Configure the LEDs to show a number
void uiShowNumber(int n)
{
  byte xlatDigit[10]  = {  DGT_0,  DGT_1,  DGT_2,  DGT_3,  DGT_4,  DGT_5,  DGT_6,  DGT_7,  DGT_8,  DGT_9 };
  uiLEDState[0] = xlatDigit[n/1000]; n%=1000;
  uiLEDState[1] = xlatDigit[n/100]; n%=100;
  uiLEDState[2] = xlatDigit[n/10]; n%=10;
  uiLEDState[3] = xlatDigit[n]; 
}

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

void uiRun(unsigned long milliseconds)
{
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
  
  for(int i=0; i<UI_MAXLEDARRAY; ++i)
  {
    // are we debouncing?
    if(uiDebounceCount[i])
    {
      --uiDebounceCount[i];
    }
    else
    {      
      if((uiSwitchStates & (1<<i)) && !uiKeyStatus[i])
      {
        uiKeyStatus[i] = UI_KEYPRESS|UI_KEYHELD;
        uiDebounceCount[i] = UI_DEBOUNCE_MS;
      }
      else if(!(uiSwitchStates & (1<<i)) && uiKeyStatus[i])
      {
        uiKeyStatus[i] = 0;
        uiDebounceCount[i] = UI_DEBOUNCE_MS;
      }
      else if(autoRepeat && uiKeyStatus[i])
      {
        uiKeyStatus[i] |= UI_KEYREPEAT;
      }
    }
  }
}



/*
  MIDI pulse rate is 24 PPQN
  96 pulses per bar (16 steps)
  max 4 bar ticker sequence
  
  
  SWING - even numbered 16th notes delayed
  
  . .
  1---+---2---+---3---+---4---+---
  ^   ^
  
  4 chan


*/
#define MAX_STEPS 32
#define TICKS_PER_BEAT  96
#define TICKS_PER_STEP  24

class IGroove
{
public:  
  virtual int getStepTime(int s) = 0;
};

class CSwing : public IGroove
{
public:  
  CSwing() : SwingAmount(66.0) {}
  double SwingAmount;
  
  int getStepTime(int s)
  {
    if(s%2) // odd
    {
      return (TICKS_PER_STEP * (s-1)) + (((double)SwingAmount*TICKS_PER_STEP)/50.0); 
    }    
    else
    {
      return TICKS_PER_STEP * s;
    }
  }
};

#define PI 3.1415926536
class CWave : public IGroove
{
public: 
  int Seed;
  int Intensity;
  CWave() 
  {
    Seed = 0;
    Intensity = 100;
  }
  int getStepTime(int s)
  {
    if(Seed) randomSeed(Seed + s);  
    double z = (random(1000) - random(1000))/1000.0;
    return (TICKS_PER_STEP * s) + ((Intensity*z*TICKS_PER_STEP)/100.0); 
    
    //double an = (PI/2.0) * ((double)s/16);
//    return (int)(16.0 * TICKS_PER_STEP * cos(an));
  }  
};

class CTicker
{
public:
  byte OutputPin;            // Arduino digital pin on which the pulse is sent
  byte Invert;               // output is LOW during tick if set (NB: output is electrically inverted at the buffer)
  byte PulseTime;            // milliseconds for the output pulse
  byte PulseRecoverTime;     // minimum milliseconds between pulses
  byte ActiveSteps;          // Total number of steps used before repeating sequence
//  int  StepTime[MAX_STEPS];   // 
  IGroove *pGroove;
private:
  enum {
    STATE_READY,   
    STATE_PULSE,
    STATE_PULSING,
    STATE_RECOVER    
  };
  
  byte currentStep;
  int tickCount;
  int nextStepTime;
  unsigned long stateEndTime;
  byte state;
  
public: 
  CTicker()
  {
//    pGroove = new CSwing;
    pGroove = new CWave;
    reset();
  }
  
  void reset()
  {
    currentStep = 0;
    tickCount = 0;
    nextStepTime = 0;
    stateEndTime = 0;
    state = STATE_READY;
  }
    
  
  // called in the unlikely event of a millisecond timer rollover
  // prevents lockup of the state machine
  void timerRollover()
  {
    stateEndTime = 0;
  }
  
  // *** The first step can be delayed but never done early

  void run(unsigned long milliseconds)
  {
    switch(state)
    {
      case STATE_PULSE:
          digitalWrite(OutputPin, !Invert); // signal the tick
          stateEndTime = milliseconds + PulseTime;
          state = STATE_PULSING;
          break;
      case STATE_PULSING:
        if(milliseconds > stateEndTime)
        {
          digitalWrite(OutputPin, !!Invert); // end the tick
          stateEndTime = milliseconds + PulseRecoverTime;
          state = STATE_RECOVER;
        }
        break;
      case STATE_RECOVER:
        if(milliseconds > stateEndTime)
          state = STATE_READY;
        break;
    }          
  }
  void tick()
  {
    if(STATE_READY == state)
    {
        // After the final step of the loop we are waiting for the last tick
        // of the loop to pass before returning to the first step
        if(currentStep < ActiveSteps && tickCount >= nextStepTime)
        {
          state = STATE_PULSE;
          // skip to next step
          if(++currentStep < ActiveSteps)     
          {
            nextStepTime = pGroove->getStepTime(currentStep);
          }
          else
          {
            // the next step will be the first of the loop
            nextStepTime = pGroove->getStepTime(0);
          }
        }
    }      
    
    // Count the tick
    if(++tickCount >= (TICKS_PER_STEP * ActiveSteps))
    {
      // we only return to step 0 at the correct
      // end time of the loop      
      tickCount = 0;
      currentStep = 0;
    } 
  }
};
CTicker Ticker1;



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
double millisecondsPerTick; 
double nextTickTime;
double bpm;
unsigned long lastMilliseconds;
void setBPM(double b)
{
  bpm = b;
  double millisecondsPerBeat = 60000.0/bpm;
  millisecondsPerTick = millisecondsPerBeat/96.0;
}



// Run the ticker outputs
void runTickers(unsigned long milliseconds)
{
  // has the millisecond timer rolled over
  if(lastMilliseconds > milliseconds)
  {
    // make sure the tickers don't lock up
    Ticker1.timerRollover();
    nextTickTime = 0;
  }
  
  // Time for the next tick?
  if(nextTickTime < milliseconds)
  {
    nextTickTime += millisecondsPerTick;
    if(nextTickTime < milliseconds)
      nextTickTime = milliseconds;
    Ticker1.tick();
  }
  Ticker1.run(milliseconds);
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

CWave W;

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

  digitalWrite(P_DIGIT0, LOW);
  digitalWrite(P_DIGIT1, LOW);
  digitalWrite(P_DIGIT2, LOW);
  digitalWrite(P_DIGIT3, LOW);
  digitalWrite(P_DIGIT4, LOW);
  digitalWrite(P_SHCLK, LOW);
  digitalWrite(P_SHDAT, LOW);

/*
  memset(sw,0,sizeof(sw));
  loadSegments(0);  
  segs[0] = DGT_0;
  segs[1] = DGT_1;
  segs[2] = DGT_2;
  segs[3] = DGT_3;
  segs[4] = 0;
  */
  pinMode(P_CLKOUT0,OUTPUT);
  pinMode(P_CLKOUT1,OUTPUT);
  pinMode(P_CLKOUT2,OUTPUT);
  pinMode(P_CLKOUT3,OUTPUT);

  
  digitalWrite(P_CLKOUT0,HIGH);
  digitalWrite(P_CLKOUT1,HIGH);
  digitalWrite(P_CLKOUT2,HIGH);
  digitalWrite(P_CLKOUT3,HIGH);    
  Ticker1.OutputPin = P_CLKOUT0;
  Ticker1.Invert = 1;
  Ticker1.PulseTime = 15;           
  Ticker1.PulseRecoverTime = 10;
  Ticker1.ActiveSteps = 16;
  Ticker1.pGroove = &W;
  Ticker1.reset();
  
  setBPM(120);
  heartBeatInit();
//Serial.begin(9600);  

  cli();
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
    runTickers(milliseconds);
    heartBeatRun(milliseconds);
    uiShowNumber(W.Intensity);
    uiRun(milliseconds);
    if(uiKeyStatus[0] & UI_NEW_KEY_PRESS)
    {
      uiKeyStatus[0]&=~UI_NEW_KEY_PRESS;
      if(W.Intensity > 1) W.Intensity--;
    }
    if(uiKeyStatus[1] & UI_NEW_KEY_PRESS)
    {
      uiKeyStatus[1]&=~UI_NEW_KEY_PRESS;
      if(W.Intensity < 99) W.Intensity++;
    }
  }
}


