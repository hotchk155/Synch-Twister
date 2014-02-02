////////////////////////////////////////////////////////
//
// SYNCH CHANNEL
//
////////////////////////////////////////////////////////

class CSynchChannel
{
public:
  byte OutputPin;            // Arduino digital pin on which the pulse is sent
  byte Invert;               // output is LOW during tick if set (NB: output is electrically inverted at the buffer)
  byte PulseTime;            // milliseconds for the output pulse
  byte PulseRecoverTime;     // minimum milliseconds between pulses
  byte ActiveSteps;          // Total number of steps used before repeating sequence
  CMutator *pMutator;
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
  CSynchChannel()
  {
    Invert = 0;
    PulseTime = 15;           
    PulseRecoverTime = 10;
    ActiveSteps = 16;    
    pMutator = new CNullMutator;
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
            nextStepTime = pMutator->getStepTime(currentStep);
          }
          else
          {
            // the next step will be the first of the loop
            nextStepTime = pMutator->getStepTime(0);
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

