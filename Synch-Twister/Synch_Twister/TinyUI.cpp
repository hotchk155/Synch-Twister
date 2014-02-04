////////////////////////////////////////////////////////
#include "Arduino.h"
#include "TinyUI.h"

#define P_DIGIT0  16
#define P_DIGIT1  15
#define P_DIGIT2  14
#define P_DIGIT3  17
#define P_DIGIT4  7

#define P_SHCLK  19
#define P_SHDAT  18
#define P_SWREAD 8

#define CBIT_DIGIT0  (1<<2) 
#define CBIT_DIGIT1  (1<<1)
#define CBIT_DIGIT2  (1<<0)
#define CBIT_DIGIT3  (1<<3)
#define DBIT_DIGIT4  (1<<7)
#define CBIT_SHCLK   (1<<5)
#define CBIT_SHDAT   (1<<4)
#define BBIT_SWREAD  (1<<0)

#define UI_MAXLEDARRAY  5         // number of LED arrays
#define UI_NUM_SWITCHES 6
#define UI_DEBOUNCE_MS 20         // key debounce
#define UI_AUTO_REPEAT_DELAY 500  // delay before key auto repeats
#define UI_AUTO_REPEAT_PERIOD 50  // delay between auto repeats
#define UI_DOUBLE_CLICK_TIME 200  // double click threshold


static byte uiLEDState[UI_MAXLEDARRAY];       // the output bit patterns for the LEDs
static byte uiDebounceCount[UI_NUM_SWITCHES];  // debounce registers for switches
static byte uiKeyStatus;              // input status for switches (The one other modules should read)
static byte uiLastKeyStatus;          
static byte uiLastKeypress;           
static unsigned long uiDoubleClickTime; 
static unsigned long uiAutoRepeatTime; 
static byte uiLongPress;
static byte uiLEDIndex;                   
static byte uiSwitchStates;
static byte uiLastSwitchStates;              
static byte uiKeyAPin;
static byte uiKeyBPin;
static byte uiKeyCPin;
static KeypressHandlerFunc uiKeypressHandler; 

////////////////////////////////////////////////////////
// Initialise variables
void CTinyUI::init()
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
  
  memset(uiLEDState, 0, sizeof(uiLEDState));
  memset(uiDebounceCount, 0, sizeof(uiDebounceCount));
  uiKeyStatus = 0;
  uiLEDIndex = 0;
  uiSwitchStates = 0;
  uiLastSwitchStates = 0;
  uiAutoRepeatTime = 0;
  uiKeyAPin = 0;
  uiKeyBPin = 0;
  uiKeyCPin = 0;
  uiKeypressHandler = NULL;
  uiLongPress = 0;
  
  // start the interrupt to service the UI   
  TCCR2A = 0;
  TCCR2B = 1<<CS22 | 0<<CS21 | 1<<CS20; // control refresh rate of display
  TIMSK2 = 1<<TOIE2;
  TCNT2 = 0; 
}

////////////////////////////////////////////////////////
void CTinyUI::setKeypressHandler(KeypressHandlerFunc f)
{
  uiKeypressHandler = f;
}

////////////////////////////////////////////////////////
void CTinyUI::setLEDs(byte which, byte mask)
{
  uiLEDState[4] &= ~mask;
  uiLEDState[4] |= which;
}

////////////////////////////////////////////////////////
void CTinyUI::clearLEDs(byte which)
{
  uiLEDState[4] &= ~which;
}

////////////////////////////////////////////////////////
// Configure the LEDs to show a decimal number
void CTinyUI::showNumber(int n, int start)
{  
  byte xlatDigit[10]  = {  
    DGT_0,  DGT_1,  DGT_2,  DGT_3,  DGT_4,  DGT_5,  DGT_6,  DGT_7,  DGT_8,  DGT_9   };
  int div[4] = {
    1000, 100, 10, 1  };
  if(start < 0 || start >= 4)
  {
    show(SEG_DP, SEG_DP, SEG_DP, SEG_DP);
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
void checKeyPin(byte pin, byte key)
{
    if(!digitalRead(pin))
      uiSwitchStates |= key;
    else  
      uiSwitchStates &= ~key;
}

////////////////////////////////////////////////////////
// Manage UI functions
void CTinyUI::run(unsigned long milliseconds)
{
  if(uiKeyAPin) checKeyPin(uiKeyAPin, TUI_KEY_A);
  if(uiKeyBPin) checKeyPin(uiKeyBPin, TUI_KEY_B);
  if(uiKeyCPin) checKeyPin(uiKeyCPin, TUI_KEY_C);

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
        uiDebounceCount[i] = UI_DEBOUNCE_MS;        
        newKeyPress = 1;
      }
      else if(!(uiSwitchStates & mask) && (uiKeyStatus & mask)) // key pressed before, not now
      {
        uiKeyStatus &= ~mask;
        uiDebounceCount[i] = UI_DEBOUNCE_MS;
      }
    }
  }

  unsigned int flags = 0;
  
  // Manage auto repeat etc
  if(!uiKeyStatus) // no keys pressed
  {
    uiLongPress = 0;
    uiKeyStatus = 0;
    uiAutoRepeatTime = 0; 
  }
  else if(uiKeyStatus != uiLastKeyStatus) // change in keypress
  {
    uiLongPress = 0;
    if(newKeyPress)
    {
      if(uiKeyStatus == uiLastKeypress && milliseconds < uiDoubleClickTime)
        flags = TUI_DOUBLE;
      else
        flags = TUI_PRESS;
      uiLastKeypress = uiKeyStatus;
      uiDoubleClickTime = milliseconds + UI_DOUBLE_CLICK_TIME;
    }
    uiAutoRepeatTime = milliseconds + UI_AUTO_REPEAT_DELAY;
  }
  else 
  {
    // keys held - not a new press
    if(uiAutoRepeatTime < milliseconds)
    {
      if(uiLongPress)  // key flagged as held?
      {
        flags = TUI_AUTO;  // now it is an auto repeat
      }
      else
      {
        uiLongPress = 1;  // otherwise flag as held
        flags = TUI_HOLD;
      }
      uiAutoRepeatTime = milliseconds + UI_AUTO_REPEAT_PERIOD;
    }
  }
  if(flags && uiKeypressHandler) 
      uiKeypressHandler(flags | uiKeyStatus);
  uiLastKeyStatus = uiKeyStatus;  
}






////////////////////////////////////////////////////////
void CTinyUI::setExtraKey(byte key, byte pin)
{
  switch(key)
  {
    case TUI_KEY_A: uiKeyAPin = pin; break;
    case TUI_KEY_B: uiKeyBPin = pin; break;
    case TUI_KEY_C: uiKeyCPin = pin; break;    
  }
  
}

////////////////////////////////////////////////////////
void CTinyUI::show(byte seg0, byte seg1, byte seg2, byte seg3)
{
  uiLEDState[0] = seg0;
  uiLEDState[1] = seg1;
  uiLEDState[2] = seg2;
  uiLEDState[3] = seg3;
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
  if(d & 0x80) PORTC |= CBIT_SHDAT; 
  else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 6
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x40) PORTC |= CBIT_SHDAT; 
  else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 5    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x20) PORTC |= CBIT_SHDAT; 
  else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 4    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x10) PORTC |= CBIT_SHDAT; 
  else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 3    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x08) PORTC |= CBIT_SHDAT; 
  else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 2    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x04) PORTC |= CBIT_SHDAT; 
  else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 1    
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x02) PORTC |= CBIT_SHDAT; 
  else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // bit 0
  PORTC &= ~CBIT_SHCLK;
  if(d & 0x01) PORTC |= CBIT_SHDAT; 
  else PORTC &= ~CBIT_SHDAT;
  PORTC |= CBIT_SHCLK;
  // flush    
  PORTC &= ~CBIT_SHCLK;
  PORTC |= CBIT_SHCLK;

  // Turn on the appropriate display
  switch(uiLEDIndex)
  {
  case 0: 
    PORTC |= CBIT_DIGIT0; 
    break;
  case 1: 
    PORTC |= CBIT_DIGIT1; 
    break;
  case 2: 
    PORTC |= CBIT_DIGIT2; 
    break;
  case 3: 
    PORTC |= CBIT_DIGIT3; 
    break;
  case 4: 
    PORTD |= DBIT_DIGIT4; 
    break;
  }

  // Next pass we'll check the next display
  if(++uiLEDIndex >= UI_MAXLEDARRAY)
    uiLEDIndex = 0;
}

////////////////////////////////////////////////////////
CTinyUI TUI;

