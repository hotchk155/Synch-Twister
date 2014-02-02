/*
  virtual byte *getName() = 0;
  virtual int getNumParams() = 0;
  virtual int getParam(int index) = 0;
  virtual int setParam(int index, int value) = 0;
  int incParam(int index) { return setParam(index, getParam(index) + 1); }
  int decParam(int index) { return setParam(index, getParam(index) - 1); }
  virtual int getStepTime(int s) = 0;
*/
 /*
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
      SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G
*/



/////////////////////////////////////////////////////////////
// "NULL MUTATOR"
// Straight beat
class CNullMutator : public CMutator
{
    void getName(byte *buf) 
    { 
      buf[0] = DGT_N;
      buf[1] = DGT_O;
      buf[2] = DGT_N;
      buf[3] = DGT_E;
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
// SWING
class CSwingMutator : public CMutator
{
  int SwingAmount;  
public:  
  CSwingMutator() 
  {
    SwingAmount = 50;
  }
  void getName(byte *buf) 
  { 
    // Shuf
    buf[0] = DGT_S;
    buf[1] = DGT_H;
    buf[2] = DGT_U;
    buf[3] = DGT_F;
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
      buf[1] = DGT_A;
      buf[2] = DGT_N;
      buf[3] = DGT_D;
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



