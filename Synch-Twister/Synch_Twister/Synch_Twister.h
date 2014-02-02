
#define MAX_STEPS 32
#define TICKS_PER_BEAT  96
#define TICKS_PER_STEP  24

////////////////////////////////////////////////////////
//
// SEVEN SEGMENT LED DISPLAY DEFINITIONS
//
////////////////////////////////////////////////////////

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
#define DGT_A (SEG_A|SEG_B|SEG_C|SEG_E|SEG_F|SEG_G)
#define DGT_B (SEG_C|SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_C (SEG_A|SEG_D|SEG_E|SEG_F)
#define DGT_D (SEG_B|SEG_C|SEG_D|SEG_E|SEG_G)
#define DGT_E (SEG_A|SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_F (SEG_A|SEG_E|SEG_F|SEG_G)
#define DGT_G (SEG_A|SEG_C|SEG_D|SEG_E|SEG_F)
#define DGT_H (SEG_B|SEG_C|SEG_E|SEG_F|SEG_G)
#define DGT_I (SEG_B|SEG_C)
#define DGT_J (SEG_B|SEG_C|SEG_D)
#define DGT_K (SEG_A|SEG_B|SEG_E|SEG_F|SEG_G)
#define DGT_L (SEG_D|SEG_E|SEG_F)
#define DGT_M (SEG_A|SEG_B|SEG_C|SEG_E|SEG_F)
#define DGT_N (SEG_A|SEG_B|SEG_C|SEG_E|SEG_F)
#define DGT_O (SEG_C|SEG_D|SEG_E|SEG_G)
#define DGT_P (SEG_A|SEG_B|SEG_E|SEG_F|SEG_G)
#define DGT_Q (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E) 
#define DGT_R (SEG_E|SEG_G)
#define DGT_S (SEG_A|SEG_C|SEG_D|SEG_F|SEG_G)
#define DGT_T (SEG_D|SEG_E|SEG_F|SEG_G)
#define DGT_U (SEG_B|SEG_C|SEG_D|SEG_E|SEG_F)
#define DGT_V (SEG_C|SEG_D|SEG_E)
#define DGT_W (SEG_A|SEG_C|SEG_D|SEG_E)
#define DGT_X (SEG_D|SEG_G)
#define DGT_Y (SEG_B|SEG_C|SEG_D|SEG_F|SEG_G)
#define DGT_Z (SEG_A|SEG_D|SEG_E|SEG_G)
#define DGT_DASH (SEG_G)

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

