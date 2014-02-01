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

class CSwing
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


class CTicker
{
public:
  byte OutputPin;            // Arduino digital pin on which the pulse is sent
  byte Invert;               // output is LOW during tick if set (NB: output is electrically inverted at the buffer)
  byte PulseTime;            // milliseconds for the output pulse
  byte PulseRecoverTime;     // minimum milliseconds between pulses
  byte ActiveSteps;          // Total number of steps used before repeating sequence
//  int  StepTime[MAX_STEPS];   // 
  CSwing *pGroove;
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
    pGroove = new CSwing;
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




void loadSegments(byte d)
{
  byte m=0x80;
  while(m)
  {
    digitalWrite(P_SHCLK, LOW);
    digitalWrite(P_SHDAT, !!(m&d));
    digitalWrite(P_SHCLK, HIGH);
    m>>=1;
  }
  digitalWrite(P_SHCLK, LOW);
  digitalWrite(P_SHCLK, HIGH);
}
void enableDisplay(byte which, byte state)
{
    digitalWrite(P_DIGIT0, (which==0)? state:LOW);
    digitalWrite(P_DIGIT1, (which==1)? state:LOW);
    digitalWrite(P_DIGIT2, (which==2)? state:LOW);
    digitalWrite(P_DIGIT3, (which==3)? state:LOW);
    digitalWrite(P_DIGIT4, (which==4)? state:LOW);
}

byte segs[5];
byte sw[5];
void refresh()
{
  for(int i=0; i<5; ++i)
  {
    enableDisplay(i,LOW);
    loadSegments(segs[i]);
    enableDisplay(i,HIGH);
    delay(1);
    sw[i] = !!digitalRead(P_SWREAD);    
  }
}

//  aaa
// f   b
// f   b
//  ggg
// e   c
// e   c
//  ddd

#define SEG_B    0x01
#define SEG_G    0x02
#define SEG_C    0x04
#define SEG_DP    0x08
#define SEG_D    0x10
#define SEG_E    0x20
#define SEG_F    0x40
#define SEG_A   0x80



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

byte xlatDigit[10]  = {  DGT_0,  DGT_1,  DGT_2,  DGT_3,  DGT_4,  DGT_5,  DGT_6,  DGT_7,  DGT_8,  DGT_9 };
void showNumber(int n)
{
  segs[0] = xlatDigit[n/1000]; n%=1000;
  segs[1] = xlatDigit[n/100]; n%=100;
  segs[2] = xlatDigit[n/10]; n%=10;
  segs[3] = xlatDigit[n]; 
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
//  straightSwing(Ticker1.StepTime, 66);
  Ticker1.reset();
  
  setBPM(120);
  heartBeatInit();
//Serial.begin(9600);  
}



int n = 999;
void loop()
{
  unsigned long milliseconds = millis();
  runTickers(milliseconds);
//digitalWrite(13,HIGH);
//delay(200);
//digitalWrite(13,LOW);
    heartBeatRun(milliseconds);
//  Serial.println(milliseconds, DEC);
  showNumber(Ticker1.pGroove->SwingAmount);
  if(sw[0] && !(milliseconds%100) && Ticker1.pGroove->SwingAmount > 1) Ticker1.pGroove->SwingAmount--;
  if(sw[1] && !(milliseconds%100) && Ticker1.pGroove->SwingAmount < 99) Ticker1.pGroove->SwingAmount++;
  refresh();
}


