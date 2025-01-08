
// ==============================================================
// cw_keyboard_hid.ino :: An Iambic Keyer USB keyboard simulator
//
// (c) Stephen Mathews - K4SDM
// Derived from code written by Scott Baker KJ7NLA
// ==============================================================

#include <Arduino.h>
#include <Keyboard.h>
#include <EEPROM.h>

#define VERSION 01

// Change these to suit your wiring - I use these as next to 
// Ground to simplify wiring
#define DIH_PIN 3
#define DAH_PIN 2
#define ledPin LED_BUILTIN

#define VBAND_DAH ']'
#define VBAND_DIT '['
#define BUZZ_PIN   4
const uint8_t pinDit  = DIH_PIN;  // dit key input
const uint8_t pinDah  = DAH_PIN;  // dah key input
const uint8_t pinSw1  = 7;  // push-button switch
      uint8_t pinBuzz = BUZZ_PIN;  // buzzer/speaker pin

//#define DEBUG 1           // uncomment for debug

// Morse-to-ASCII lookup table
const char m2a[129] PROGMEM =
  {'~',' ','E','T','I','A','N','M','S','U','R','W','D','K','G','O',
   'H','V','F','*','L','*','P','J','B','X','C','Y','Z','Q','*','*',
   '5','4','S','3','*','*','*','2','&','*','+','*','*','*','J','1',
   '6','=','/','*','*','#','(','*','7','*','G','*','8','*','9','0',
   0x08,'*','*','*','*','*','*','*','*','*','*','*','?','_','*','*',
   '*','\\','"','*','*','.','*','*','*','*','@','*','*','*','\'','*',
   '*','-','*','*','*','*','*','*','*','*',';','!','*',')','*','*',
   '*','*','*',',','*','*','*','*',':','*','*','*','*','*','*','`',0x08};

// ASCII-to-Morse lookup table
const uint8_t a2m[64] PROGMEM =
  {0x01,0x6b,0x52,0x4c,0x89,0x4c,0x28,0x5e,  //   ! " # $ % & '
   0x36,0x6d,0x4c,0x2a,0x73,0x61,0x55,0x32,  // ( ) * + , - . /
   0x3f,0x2f,0x27,0x23,0x21,0x20,0x30,0x38,  // 0 1 2 3 4 5 6 7
   0x3c,0x3e,0x78,0x6a,0x4c,0x31,0x4c,0x4c,  // 8 9 : ; < = > ?
   0x5a,0x05,0x18,0x1a,0x0c,0x02,0x12,0x0e,  // @ A B C D E F G
   0x10,0x04,0x17,0x0d,0x14,0x07,0x06,0x0f,  // H I J K L M N O
   0x16,0x1d,0x0a,0x08,0x03,0x09,0x11,0x0b,  // P Q R S T U V W
   0x19,0x1b,0x1c,0x4c,0x40,0x4c,0x80,0x4d}; // X Y Z [ \ ] ^ _

// user interface
#define NBP  0  // no-button-pushed
#define BSC  1  // button-single-click
#define BPL  2  // button-push-long

#define LONGPRESS  500  // long button press

#define RUN_MODE  0  // user practice mode
#define SETUP     1  // change settings

volatile uint8_t  event    = NBP;
volatile uint8_t  menumode = RUN_MODE;
uint8_t keyerwpm;

char vband_mode = 0;


#define DITCONST  1200       // dit time constant
#define MAXWPM    35         // max keyer speed
#define INITWPM   10         // startup keyer speed  // SDM
#define MINWPM    5         // min keyer speed

#define MINTONE 450
#define MAXTONE 850

#define MINLESSONMODE 0
#define MAXLESSONMODE 1

#define MINLESSON 1
#define MAXLESSON 39

#define MINLESSONCNT 1
#define MAXLESSONCNT 80


// 4x20 LCD
#define COLSIZE   20
#define ROWSIZE    4
#define MAXCOL    COLSIZE-1  // max display column
#define MAXLINE   ROWSIZE-1  // max display row
#define SPACE10   "          "
#define SPACE20   "                    "

//#define BAUDRATE 9600
#define BAUDRATE 115200

// Class instantiation
char speed_set_mode = 0;
char speed = 0;

uint8_t maddr = 1;
uint8_t myrow = 0;
uint8_t mycol = 0;
char tmpstr[12];

char lookup_cw(uint8_t x);
void print_cw(void);
void maddr_cmd(uint8_t cmd);
inline void read_switch(void);
void read_paddles(void);
void iambic_keyer(void);
void straight_key(void);
void send_cwchr(char ch);
void ditcalc(void);
void doError(void);
void(*resetFunc) (void) = 0;

#define SKHZ    220    // error beep   = @A4 note
#define DDHZ    659    // dit/dah beep = @E5 note
char xmit_buf[620] = {0};
uint8_t xmit_cnt=0;

char last_ch = 0;

void print_cw() {
  char ch = lookup_cw(maddr);

  if( !vband_mode )
  {
    if( speed_set_mode == 0 )
    {
      Keyboard.press( ch );
      Keyboard.release( ch );
      if( ch == 0x08 && 0 )
      {
        Keyboard.press( ch );
        Keyboard.release( ch );
      }
    }
    else
    {
      short wpm = keyerwpm;

      if( ch == 'E' )
      {
        wpm--;
        if( wpm < MINWPM )
          wpm = MINWPM;
        change_wpm( wpm );
        print_wpm( wpm );
      }

      if( ch == 'T' )
      {
        wpm++;
        if( wpm > MAXWPM )
          wpm = MAXWPM;
        change_wpm( wpm );
        print_wpm( wpm );
      }

    }
  }    

  //printchar(ch);
  if( last_ch == '`' && ch == 0x08 )
  {
     speed_set_mode = 1;
     speed = keyerwpm;
  }

  if( last_ch == 0x08 && ch == '`' )
  {
     speed_set_mode = 0;
     EEPROM[1] = keyerwpm;
  }
  last_ch = ch;
}

void print_wpm( char wpm )
{
  char tens = (wpm / 10);
  char ones = wpm % 10;

  delay(2);

  Keyboard.press( ' ' );
  Keyboard.release( ' ' );

  delay(2);

  Keyboard.press( '0'+tens );
  Keyboard.release( '0'+tens );

  delay(2);

  Keyboard.press( '0'+ones );
  Keyboard.release( '0'+ones );


}



char realtime_xmit = 0;

short keyertone = 650;
short remotetone = 750;

byte lesson = 1;
byte lesson_size = MAXLESSONCNT;
byte lesson_mode = 0;

byte farns = 0;

volatile uint8_t  keyermode  = 0;   // keyer mode
volatile uint8_t  keyswap    = 0;   // key swap


// table lookup for CW decoder
char lookup_cw(uint8_t addr) {
  char ch = '*';
  if (addr < 129) ch = pgm_read_byte(m2a + addr);

  #ifdef DEBUG
  Serial.println(addr);
  #endif  // DEBUG

  return ch;
}


// print the ascii char

// update the morse code table address
void maddr_cmd(uint8_t cmd) {
  if (cmd == 2) {
    // print the tranlated ascii
    // and reset the table address
    print_cw();
    maddr = 1;
  }
  else {
    // update the table address
    tone(pinBuzz, keyertone);
    maddr = maddr<<1;
    maddr |= cmd;
  }
}

// An Iambic (mode A/B) morse code keyer
// Copyright 2021 Scott Baker KJ7NLA


// keyerinfo bit definitions
#define DIT_REG     0x01     // dit pressed
#define DAH_REG     0x02     // dah pressed
#define KEY_REG     0x03     // dit or dah pressed
#define WAS_DIT     0x04     // last key was dit
#define BOTH_REG    0x08     // both dit/dah pressed

// keyermode bit definitions
#define IAMBICA   0        // iambic A keyer mode
#define IAMBICB   1        // iambic B keyer mode

// keyerstate machine states
#define KEY_IDLE    0
#define CHK_KEY     1
#define KEY_WAIT    2
#define IDD_WAIT    3
#define LTR_GAP     4
#define WORD_GAP    5

// more key info
#define GOTKEY  (keyerinfo & KEY_REG)
#define NOKEY   !GOTKEY
#define GOTBOTH  GOTKEY == KEY_REG

uint8_t  keyerstate = KEY_IDLE;
uint8_t  keyerinfo  = 0;

uint16_t dittime;        // dit time
uint16_t dahtime;        // dah time
uint16_t lettergap1;     // letter space for decode
uint16_t lettergap2;     // letter space for send
uint16_t wordgap1;       // word space for decode
uint16_t wordgap2;       // word space for send
uint8_t  sw1Pushed = 0;  // button pushed

// read and debounce switch
inline void read_switch() {
  sw1Pushed = !digitalRead(pinSw1);
}

// read and debounce paddles
void read_paddles() {
  uint8_t ditv1 = !digitalRead(pinDit);
  uint8_t dahv1 = !digitalRead(pinDah);
  uint8_t ditv2 = !digitalRead(pinDit);
  uint8_t dahv2 = !digitalRead(pinDah);

  if (ditv1 && ditv2) {
    if (keyswap) keyerinfo |= DAH_REG;
    else keyerinfo |= DIT_REG;
  }
  if (dahv1 && dahv2) {
    if (keyswap) keyerinfo |= DIT_REG;
    else keyerinfo |= DAH_REG;
  }
  if (GOTBOTH) keyerinfo |= BOTH_REG;

  if( vband_mode )
  {
    //Keyboard.releaseAll();

    if( keyerinfo & DAH_REG )
      Keyboard.press( VBAND_DAH );
    else
      Keyboard.release( VBAND_DAH );

    if( keyerinfo & DIT_REG )
      Keyboard.press( VBAND_DIT );
    else
      Keyboard.release( VBAND_DIT );
  }
}

// iambic keyer state machine
void iambic_keyer() {
  static uint32_t ktimer;
  switch (keyerstate) {
    case KEY_IDLE:
      read_paddles();
      if (GOTKEY) {
        keyerstate = CHK_KEY;
      } else {
        keyerinfo = 0;
      }
      break;
    case CHK_KEY:
      switch (keyerinfo & 0x07) {
        case 0x05:  // dit and prev dit
        case 0x03:  // dit and dah
        case 0x01:  // dit
          keyerinfo &= ~DIT_REG;
          keyerinfo |= WAS_DIT;
          ktimer = millis() + dittime;
          maddr_cmd(0);
          if( realtime_xmit == 0 )
            Serial.print(".");
          else if( realtime_xmit != 0 && xmit_cnt < sizeof( xmit_buf )-1 )
            xmit_buf[xmit_cnt++] = '.';
          keyerstate = KEY_WAIT;
          break;
        case 0x07:  // dit and dah and prev dit
        case 0x06:  // dah and prev dit
        case 0x02:  // dah
          keyerinfo &= ~DAH_REG;
          keyerinfo &= ~WAS_DIT;
          ktimer = millis() + dahtime;
          maddr_cmd(1);
          if( realtime_xmit == 0 )
            Serial.print("-");
          else if( realtime_xmit != 0 && xmit_cnt < sizeof( xmit_buf )-1 )
            xmit_buf[xmit_cnt++] = '-';
          keyerstate = KEY_WAIT;
          break;
        default:
          // case 0x00:  // nothing
          // case 0x04:  // nothing and prev dit
          keyerstate = KEY_IDLE;
      }
      break;
    case KEY_WAIT:
      // wait dit/dah duration
      if (millis() > ktimer) {
        // done sending dit/dah
        noTone(pinBuzz);
        // inter-symbol time is 1 dit
        ktimer = millis() + dittime;
        keyerstate = IDD_WAIT;
      }
      break;
    case IDD_WAIT:
      // wait time between dit/dah
      if (millis() > ktimer) {
        // wait done
        keyerinfo &= ~KEY_REG;
        if (keyermode == IAMBICA) {
          // Iambic A
          // check for letter space
          ktimer = millis() + lettergap1;
          keyerstate = LTR_GAP;
        } else {
          // Iambic B
          read_paddles();
          if (NOKEY && (keyerinfo & BOTH_REG)) {
            // send opposite of last paddle sent
            if (keyerinfo & WAS_DIT) {
              // send a dah
              ktimer = millis() + dahtime;
              maddr_cmd(1);
            }
            else {
              // send a dit
              ktimer = millis() + dittime;
              maddr_cmd(0);
            }
            keyerinfo = 0;
            keyerstate = KEY_WAIT;
          } else {
            // check for letter space
            ktimer = millis() + lettergap1;
            keyerstate = LTR_GAP;
          }
        }
      }
      break;
    case LTR_GAP:
      if (millis() > ktimer) {
        // letter space found so print char
        maddr_cmd(2);
          if( xmit_cnt < sizeof( xmit_buf )-1 )
          {
            if( realtime_xmit == 0 )
              Serial.print(" ");
            else if( realtime_xmit == 1 )
            {
              xmit_buf[xmit_cnt++] = ' ';
            }
            else if( realtime_xmit == 2 )
            {
              xmit_buf[xmit_cnt++] = ' ';
              xmit_buf[xmit_cnt] = '\0';
              Serial.print(xmit_buf);
              memset( xmit_buf, 0, sizeof( xmit_buf ) );
              xmit_cnt = 0;
            }
          }
        // check for word space
        ktimer = millis() + wordgap1;
        keyerstate = WORD_GAP;
      }
      read_paddles();
      if (GOTKEY) {
        keyerstate = CHK_KEY;
      } else {
        keyerinfo = 0;
      }
      break;
    case WORD_GAP:
      if (millis() > ktimer) {
        // word gap found so print a space
        maddr = 1;
        print_cw();
        keyerstate = KEY_IDLE;
        if( realtime_xmit == 1 )
        {
          Serial.print("\r\n");
        }
        else if( realtime_xmit == 2 )
        {
            Serial.print(xmit_buf);
            memset( xmit_buf, 0, sizeof( xmit_buf ) );
            xmit_cnt = 0;
        }
      }
      read_paddles();
      if (GOTKEY) {
        keyerstate = CHK_KEY;
      } else {
        keyerinfo = 0;
      }
      break;
    default:
      break;
  }


      if( !vband_mode )
      {
          if (Serial.available() > 0) {
            // read the incoming byte:
            char incomingByte = Serial.read();
        
            // say what you got:
            if( incomingByte == '.' )
              {
                tone(pinBuzz, remotetone);
                delay( dittime );
                noTone( pinBuzz );
              }
            else if( incomingByte == '-' )
              {             
                tone(pinBuzz, remotetone);
                delay( dahtime );
                noTone( pinBuzz );
              }
              delay( dittime );
          }
      }
  
}

// handle straight key mode
void straight_key() {
  uint8_t pin = keyswap ? pinDah : pinDit;
  if (!digitalRead(pin)) {
    tone(pinBuzz, keyertone);
    while(!digitalRead(pin)) {
      delay(1);
    }
    noTone(pinBuzz);
  }
}

void straight_key_interpret() {
  uint8_t pin = keyswap ? pinDah : pinDit;
  if (!digitalRead(pin)) {
    tone(pinBuzz, keyertone);
    while(!digitalRead(pin)) {
      delay(1);
    }
    noTone(pinBuzz);
  }
}

// convert ascii to morse
void send_cwchr(char ch) {
  uint8_t mcode;
  // remove the ascii code offset (0x20) to
  // create a lookup table address
  uint8_t addr = (uint8_t)ch - 0x20;
  // then lookup the Morse code from the a2m table
  // note: any unknown unknown ascii char is
  // translated to a '?' (mcode 0x4c)
  if (addr < 64) mcode = pgm_read_byte(a2m + addr);
  else mcode = 0x4c;
  // if space (mcode 0x01) is found
  // then wait for one word space
  if (mcode == 0x01) delay(wordgap2);
  else {
    uint8_t mask = 0x80;
    // use a bit mask to find the leftmost 1
    // this marks the start of the Morse code bits
    while (!(mask & mcode)) mask = mask >> 1;
    while (mask != 1) {
      mask = mask >> 1;
      // use the bit mask to select a bit from
      // the Morse code starting from the left
      tone(pinBuzz, keyertone);
      // turn the side-tone on for dit or dah length
      // if the mcode bit is a 1 then send a dah
      // if the mcode bit is a 0 then send a dit
      delay((mcode & mask) ? dahtime : dittime);
      noTone(pinBuzz);
      // turn the side-tone off for a symbol space
      delay(dittime);
    }
    delay(lettergap2);  // add letter space
  }
}





// initial keyer speed
void ditcalc() {
  int farn = keyerwpm - farns;
  if( farn <= 0 ) farn = keyerwpm;

  dittime    = DITCONST/keyerwpm;
  dahtime    = (DITCONST * 3)/keyerwpm;
  lettergap1 = (DITCONST * 2.5)/(keyerwpm);
  lettergap2   = (DITCONST * 3)/(farn);
  wordgap1   = (DITCONST * 5)/(keyerwpm);
  wordgap2   = (DITCONST * 7)/(farn);
}

// change the keyer speed
void change_wpm(uint8_t wpm) {
  keyerwpm = wpm;
  ditcalc();
}


// reset the Arduino
void doError() {
  resetFunc();
}

// program setup
void setup() {
  // set GPIO
  pinMode(pinDit,  INPUT_PULLUP);
  pinMode(pinDah,  INPUT_PULLUP);
  pinMode(pinSw1,  INPUT_PULLUP);
  pinMode(pinBuzz, OUTPUT);
  // startup init
  Serial.begin(BAUDRATE);     // init serial/debug port

  if( EEPROM[0] != VERSION )
  {
    EEPROM[0] = VERSION;
    change_wpm(INITWPM);        // init keyer speed
  }
  else
  {
    if( !digitalRead(pinDit) && !digitalRead(pinDah) )
    {
      speed_set_mode = 0;
      change_wpm( INITWPM );
      EEPROM[1] = INITWPM;
      EEPROM[2] = 0;
    }
    else if( !digitalRead(pinDit) )
    {
      vband_mode = 1;
      EEPROM[2] = vband_mode;
    }
    else if( !digitalRead(pinDah) )
    {
      vband_mode = 0;
      EEPROM[2] = vband_mode;
    }
    else
    {
      char w = EEPROM[1];
      if( w > MAXWPM || w < MINWPM )
        change_wpm(INITWPM);
      else
        change_wpm(EEPROM[1]);

      vband_mode = EEPROM[2];
    }
  }


  while( !digitalRead(pinDit) || !digitalRead(pinDah) )
  {
     delay(5);
     digitalWrite( pinDit, HIGH );
     digitalWrite( pinDah, HIGH );
     delay(5);
  }

  delay(1500);

  digitalWrite( pinDit, HIGH );
  digitalWrite( pinDah, HIGH );

  //lcd.setRowOffsets( 0, 20, 30 40 );

  ditcalc();

  send_cwchr('O');
  send_cwchr('K');

  if( vband_mode )
    pinBuzz = 0;

}

// main loop
void loop() {
  uint32_t t0;
  read_switch();
  if (sw1Pushed) {
    event = BSC;
    delay(10); // debounce
    t0 = millis();
    // check for long button press
    while (sw1Pushed && (event != BPL)) {
      if (millis() > (t0 + LONGPRESS)) event = BPL;
      read_switch();
      delay(10);  // debounce
    }

    // button single click
    if (event == BSC) {
      switch (menumode) {
        case RUN_MODE:
          menumode = SETUP;
          break;
        default:
          menumode = 0;
      }
      //menu_trainer_mode();
      event = NBP;
    }

    // long button press
    else if (event == BPL) {
      //menu_msg();
      event = NBP;
    }
  }

  if( speed_set_mode == 1 )
  {
    // Abort Speed Setting
    if( !digitalRead(pinDit) && !digitalRead(pinDah) )
    {
      speed_set_mode = 0;
      change_wpm( speed );
    }
  }

  // no buttons pressed
  iambic_keyer();
}
