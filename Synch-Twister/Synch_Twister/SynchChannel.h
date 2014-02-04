////////////////////////////////////////////////////////
//
// SYNCH CHANNEL
//
////////////////////////////////////////////////////////

class CSynchChannel
{

  byte outputPin;            // Arduino digital pin on which the pulse is sent
  byte activeSteps;          // Total number of steps used before repeating sequence
  byte divider;
  byte pulseTime;            // milliseconds for the output pulse
  byte pulseRecoverTime;     // minimum milliseconds between pulses
  byte invert;               // output is LOW during tick if set (NB: output is electrically inverted at the buffer)
  byte mutator;
  CMutator *pMutator[MUTATOR_MAX];
  
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
  enum 
  {
    PARAM_MUTATION,
    PARAM_STEPS,        
    PARAM_DIV,        
    PARAM_PULSEMS,        
    PARAM_RECOVERMS,        
    PARAM_INVERT        
  };
  
  ////////////////////////////////////////////////////////
  CSynchChannel()
  {
    invert = 0;
    pulseTime = 15;           
    pulseRecoverTime = 10;
    activeSteps = 16;    

    // create the mutators
    // TODO: Load the config    
    for(int i = 0; i < MUTATOR_MAX; ++i)
      pMutator[i] = createMutator(i);
    reset();
  }

  ////////////////////////////////////////////////////////
  void setOutputPin(byte p)
  {
    outputPin = p;
  }
  
  ////////////////////////////////////////////////////////
  CMutator *getMutator() 
  {
    return pMutator[mutator];
  }
  
  ////////////////////////////////////////////////////////  
  int setParam(int which, int value)
  {
    switch(which)
    {
    case PARAM_MUTATION:
      mutator = constrain(value,0,MUTATOR_MAX-1);
      return mutator;
    case PARAM_STEPS:
      activeSteps = constrain(value,1,99);
      return activeSteps;
    case PARAM_DIV:
      divider = constrain(value,1,99);
      return divider;
    case PARAM_PULSEMS:
      pulseTime = constrain(value,1,99);
      return pulseTime;
    case PARAM_RECOVERMS:
      pulseRecoverTime = constrain(value,1,99);
      return pulseRecoverTime;    
    case PARAM_INVERT:        
      invert = constrain(value,0,1);
      return invert;    
    default:
      return 0;    
    }
  }
  
  ////////////////////////////////////////////////////////
  int getParam(int which)
  {
    switch(which)
    {
    case PARAM_MUTATION:
      return mutator;
    case PARAM_STEPS:
      return activeSteps;
    case PARAM_DIV:
      return divider;
    case PARAM_PULSEMS:
      return pulseTime;
    case PARAM_RECOVERMS:
      return pulseRecoverTime;    
    case PARAM_INVERT:        
      return invert;    
    default:
      return 0;    
    }
  }

  ////////////////////////////////////////////////////////
  int changeParam(int which, byte inc)
  {
    if(inc)
      return setParam(which, getParam(which)+1);
    else
      return setParam(which, getParam(which)-1);
  }

  ////////////////////////////////////////////////////////
  void reset()
  {
    currentStep = 0;
    tickCount = 0;
    nextStepTime = 0;
    stateEndTime = 0;
    state = STATE_READY;
  }      
  
  ////////////////////////////////////////////////////////
  void timerRollover()
  {
    // called in the unlikely event of a millisecond timer rollover
    // prevents lockup of the state machine
    stateEndTime = 0;
  }
  
  // *** The first step can be delayed but never done early

  ////////////////////////////////////////////////////////
  void run(unsigned long milliseconds)
  {
    switch(state)
    {
      case STATE_PULSE:
          digitalWrite(outputPin, !invert); // signal the tick
          stateEndTime = milliseconds + pulseTime;
          state = STATE_PULSING;
          break;
      case STATE_PULSING:
        if(milliseconds > stateEndTime)
        {
          digitalWrite(outputPin, !!invert); // end the tick
          stateEndTime = milliseconds + pulseRecoverTime;
          state = STATE_RECOVER;
        }
        break;
      case STATE_RECOVER:
        if(milliseconds > stateEndTime)
          state = STATE_READY;
        break;
    }          
  }
  
  ////////////////////////////////////////////////////////  
  void tick()
  {
    if(STATE_READY == state)
    {
        // After the final step of the loop we are waiting for the last tick
        // of the loop to pass before returning to the first step
        if(currentStep < activeSteps && tickCount >= nextStepTime)
        {
          state = STATE_PULSE;
          // skip to next step
          if(++currentStep < activeSteps)     
          {
            nextStepTime = pMutator[mutator]->getStepTime(currentStep);
          }
          else
          {
            // the next step will be the first of the loop
            nextStepTime = pMutator[mutator]->getStepTime(0);
          }
        }
    }      
    
    // Count the tick
    if(++tickCount >= (TICKS_PER_STEP * activeSteps))
    {
      // we only return to step 0 at the correct
      // end time of the loop      
      tickCount = 0;
      currentStep = 0;
    } 
  }
};

