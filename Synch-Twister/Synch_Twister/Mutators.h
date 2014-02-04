/////////////////////////////////////////////////////////////
//
// M U T A T O R S
//
// These classes define the functions that apply mutations
// to the straightforward time sequence 
//
// Instances of the class contain a specific configuration
// of the mutation function, based on a set of integer
// parameters
//
/////////////////////////////////////////////////////////////

// Maximum number of parameters that can be associated with
// any type of muatator
#define MUTATOR_PARAMS_MAX 4

// Define the types of mutator
enum {
  MUTATOR_NULL,
  MUTATOR_SHUFFLE,
  MUTATOR_RANDOM,
  MUTATOR_MAX
};

/////////////////////////////////////////////////////////////
// "NULL MUTATOR"
// Straight beat
class CNullMutator : public CMutator
{
    void getName(byte *buf) 
    { 
      buf[0] = DGT_DASH;
      buf[1] = DGT_DASH;
      buf[2] = DGT_DASH;
  }
    
  int getStepTime(int s) 
  {
    return TICKS_PER_STEP * s;
  }
  int getNumParams() { return 0; }
  int getParam(int index) { return 0; }
  int setParam(int index, int value) { return 0; }
};

/////////////////////////////////////////////////////////////
// SHUFFLE
class CShuffleMutator : public CMutator
{
  int SwingAmount;  
public:  
  CShuffleMutator() 
  {
    SwingAmount = 50;
  }
  void getName(byte *buf) 
  { 
    // Shuf
    buf[0] = DGT_S;
    buf[1] = DGT_H;
    buf[2] = DGT_F;
  }
  
  int getStepTime(int s)
  {
    if(s%2) // odd
      return (TICKS_PER_STEP * (s-1)) + (((double)SwingAmount*TICKS_PER_STEP)/50.0); 
    else
      return TICKS_PER_STEP * s;
  }
  int getNumParams() { return 1; }
  int getParam(int index) { return SwingAmount; }
  int setParam(int index, int value) { SwingAmount = constrain(value, 1, 99); return SwingAmount; }
};




/////////////////////////////////////////////////////////////
// RANDOM
class CRandomMutator : public CMutator
{
public: 
  int Seed;
  int Intensity;
  CRandomMutator() 
  {
    Seed = 1;
    Intensity = 20;
  }
  void getName(byte *buf) 
  { 
      // rAnd
      buf[0] = DGT_R;
      buf[1] = DGT_N;
      buf[2] = DGT_D;
  }
  int getStepTime(int s)
  {
    if(Seed) randomSeed(Seed + s);  
    double z = (random(1000) - random(1000))/1000.0;
    return (TICKS_PER_STEP * s) + ((Intensity*z*TICKS_PER_STEP)/100.0); 
  }  
  int getNumParams() { return 2; }
  int getParam(int index) { if(index == 1) return Intensity; else return Seed; }
  int setParam(int index, int value) { 
      if(index == 1)
      {  
          Intensity = constrain(value, 0, 100); 
          return Intensity;
      }
      else
      {
          Seed = constrain(value, 0, 999); 
          return Seed;
      }
  }
};



