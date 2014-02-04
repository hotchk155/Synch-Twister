
////////////////////////////////////////////////////////
//
// DEFINE IO PINS
//
////////////////////////////////////////////////////////
#define P_CLKOUT0 9
#define P_CLKOUT1 10
#define P_CLKOUT2 11
#define P_CLKOUT3 12


#define P_SELECT 2
#define DBIT_SELECT  (1<<2)

#define P_HEARTBEAT 13

#define MAX_STEPS 32
#define TICKS_PER_BEAT  96
#define TICKS_PER_STEP  24


class CMutator
{
public:  
  virtual void getName(byte *buf) = 0;
  virtual int getStepTime(int s) = 0;
  virtual int getNumParams() = 0;
  virtual int getParam(int index) = 0;
  virtual int setParam(int index, int value) = 0;
  int changeParam(int index, byte inc) 
  { 
    if(inc)
      return setParam(index, getParam(index) + 1);
    else
      return setParam(index, getParam(index) - 1);
  }
};
extern CMutator *createMutator(byte which);

