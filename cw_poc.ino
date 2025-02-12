
// ==============================================================
// cw_keyboard_hid.ino :: An Iambic Keyer USB keyboard simulator
//
// (c) Stephen Mathews - K4SDM
// Derived from code written by Scott Baker KJ7NLA
// ==============================================================

#define CW_OLED 1
#define CW_LCD  0


#include <Arduino.h>
#include <Keyboard.h>
#include <EEPROM.h>

const char version[] = "1.0.3";
#define VERSION 10

// Change these to suit your wiring - I use these as next to 
// Ground to simplify wiring
#define DIH_PIN 3
#define DAH_PIN 2
#define ledPin LED_BUILTIN

//#define VBAND_DAH KEY_RIGHT_CTRL
//#define VBAND_DIT KEY_LEFT_CTRL
#define VBAND_DAH 0xe2
#define VBAND_DIT 0xe1

#define BUZZ_GND   A1
#define BUZZ_PIN   A3
//#define BUZZ_PIN   4

const uint8_t pinDit  = DIH_PIN;  // dit key input
const uint8_t pinDah  = DAH_PIN;  // dah key input
const uint8_t pinSw1  = 7;  // push-button switch
const uint8_t pinInnerBuzz = BUZZ_PIN;
const uint8_t pinOuterBuzz = BUZZ_PIN;
      uint8_t pinBuzz = BUZZ_PIN;  // buzzer/speaker pin

//#define DEBUG 1           // uncomment for debug




// Morse-to-ASCII lookup table
const char m2a[0xc6] PROGMEM =
  {'~',' ','E','T','I','A','N','M','S','U','R','W','D','K','G','O',
   'H','V','F','*','L','*','P','J','B','X','C','Y','Z','Q','*','*',
   '5','4','S','3','*','*','*','2','&','*','+','*','*','*','J','1',
   '6','=','/','*','*','#','(','*','7','*','G','*','8','*','9','0',
   '\n','*','*','*','*','*','*','*','*','*','*','*','?','_','*','*',
   '*','*','"','*','*','.','*','*','*','*','@','*','*','*','\'','*',
   '*','-','*','*','*','*','*','*','*','*',';','!','*',')','*','*',
   '*','*','*',',','*','*','*','*',':','*','*','*','*','*','*','`',
   '\n','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*',
   '*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*',
   '*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*',
   '*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*',
   '*','*','*','*','*','\\' /*0x08*/};

// ASCII-to-Morse lookup table
const uint8_t a2m[64] PROGMEM =
  {0x01,0x6b,0x52,0x4c,0x89,0x4c,0x28,0x5e,  //   ! " # $ % & '
   0x36,0x6d,0x4c,0x2a,0x73,0x61,0x55,0x32,  // ( ) * + , - . /
   0x3f,0x2f,0x27,0x23,0x21,0x20,0x30,0x38,  // 0 1 2 3 4 5 6 7
   0x3c,0x3e,0x78,0x6a,0x4c,0x31,0x4c,0x4c,  // 8 9 : ; < = > ?
   0x5a,0x05,0x18,0x1a,0x0c,0x02,0x12,0x0e,  // @ A B C D E F G
   0x10,0x04,0x17,0x0d,0x14,0x07,0x06,0x0f,  // H I J K L M N O
   0x16,0x1d,0x0a,0x08,0x03,0x09,0x11,0x0b,  // P Q R S T U V W
   0x19,0x1b,0x1c,0x4c,0xc5,0x4c,0x80,0x4d}; // X Y Z [ \ ] ^ _



const char* lesson_licw    = "REATINPGSLCDHOFUWBKMY59,QXV73?16.ZJ/28\\40";
const char* lesson_koch    = "KMRSUAPTLOWI.NJEF0Y,VG5/Q9ZH38B?427C1D6X\\";
const char* lesson_estonia = "ESTONIADRMULCHPFWYGBJKQXZY1234567890,.?/\\";
char* lesson_seq;

byte lesson_window = 0;

char buzz_mode = 0;
const byte ver = 1;

class LCD_Sim
{

  public:

    void setCursor( char col, char row )
    {
    }

    void clear()
    {
      Keyboard.print('\n');
    }

    void print(char ch, bool draw = true )
    {
      Keyboard.print( ch );
    }

    void print( const char* str )
    {
      for( int i=0; str[i] != '\0'; i++ )
        Keyboard.print( str[i] );
    }

    void print_line( char row, const char* str )
    {
      Keyboard.println( str );
      Keyboard.print( '\n' );
    }

    void clear_line( char row )
    {
      Keyboard.print( '\n' );
    }

};


LCD_Sim lcds;







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
#define INITWPM   16         // startup keyer speed  // SDM
#define MINWPM    5         // min keyer speed

#define MINTONE 450
#define MAXTONE 850

#define MINLESSONMODE 1
#define MAXLESSONMODE 3

#define MINLESSON 1
#define MAXLESSON 40


#define MINWINDOW 0
#define MAXWINDOW 10


#define MINLESSONCNT 1
#define MAXLESSONCNT 60


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
byte buzz_tone = 650;

uint8_t maddr = 1;
uint8_t myrow = 0;
uint8_t mycol = 0;
char tmpstr[60];

char lookup_cw(uint8_t x);
void print_cw(bool);
void maddr_cmd(uint8_t cmd);
inline bool read_switch(void);
void read_paddles(void);
void iambic_keyer(bool);
void straight_key(void);
void send_cwchr(char ch);
void ditcalc(char);
void doError(void);
void(*resetFunc) (void) = 0;

#define SKHZ    220    // error beep   = @A4 note
#define DDHZ    659    // dit/dah beep = @E5 note
char xmit_buf[620] = {0};
uint8_t xmit_cnt=0;

char next2last_ch = 0;
char last_ch = 0;

void print_cw(bool out = true) {
  char ch = lookup_cw(maddr);

  if( out )
  if( !vband_mode )
  {
    if( speed_set_mode == 0 )
    {
      Keyboard.write( ch );
      Keyboard.flush();

      if( ch == 0x08 && 0 )
      {
        Keyboard.write( ch );
        Keyboard.flush();
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
  next2last_ch = last_ch;
  last_ch = ch;

  Keyboard.flush();
}

void print_wpm( char wpm )
{
  char tens = (wpm / 10);
  char ones = wpm % 10;

  Keyboard.print( ' ' );

  delay(20);

  Keyboard.print( wpm, DEC );

  delay(20);

  Keyboard.print( ' ' );

  delay(20);

  //Keyboard.flush();

}



char realtime_xmit = 0;

short keyertone = 650;
short remotetone = 750;

char lesson = 1;
byte lesson_size = MAXLESSONCNT;
byte lesson_mode = 0;

byte farns = 0;

volatile uint8_t  keyermode  = 0;   // keyer mode
volatile uint8_t  keyswap    = 0;   // key swap


// table lookup for CW decoder
char lookup_cw(uint8_t addr) {
  char ch = '*';
  if (addr < sizeof(m2a) ) ch = pgm_read_byte(m2a + addr);

  #ifdef DEBUG
  Serial.println(addr);
  #endif  // DEBUG

  return ch;
}


// print the ascii char

// update the morse code table address
void maddr_cmd(uint8_t cmd, bool out = true) {
  if (cmd == 2) {
    // print the tranlated ascii
    // and reset the table address
    print_cw(out);
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
#define BOTH_KEY    6

// more key info
#define GOTKEY  (keyerinfo & KEY_REG)
#define NOKEY   !GOTKEY
#define GOTBOTH  (GOTKEY == KEY_REG)

uint8_t  keyerstate = KEY_IDLE;
uint8_t  keyerinfo  = 0;

uint16_t dittime;        // dit time
uint16_t dahtime;        // dah time
uint16_t lettergap1;     // letter space for decode
uint16_t lettergap2;     // letter space for send
uint16_t wordgap1;       // word space for decode
uint16_t wordgap2;       // word space for send
uint8_t  sw1Pushed = 0;  // button pushed
uint8_t  long_squeeze = 0;  // paddles squeezed

// read and debounce switch
inline bool read_switch() {
  sw1Pushed = ( !digitalRead(pinDit) && !digitalRead(pinDah) );
  delay(20);
  if( sw1Pushed )
    sw1Pushed = ( !digitalRead(pinDit) && !digitalRead(pinDah) );

  return sw1Pushed;
}



void print( const char* str )
{
  for( int i=0; str[i] != '\0'; i++ )
    print( str[i] );
}

void print( char ch )
{
  delay(20);
  Keyboard.print( ch );
  delay(20);
}


void println( const char* str )
{
  for( int i=0; str[i] != '\0'; i++ )
    Keyboard.print( str[i] );

    println();
}

void println( char ch )
{
    print( ch );

    println();
}

void println()
{
    delay(5);
    print( '\n' );
    delay(5);
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
  if (GOTBOTH)
  {
    keyerinfo |= BOTH_REG;
  }


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

void iambic_keyer( bool out = true ) {
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
          maddr_cmd(0, out);
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
          maddr_cmd(1, out);
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
              maddr_cmd(1, out);
            }
            else {
              // send a dit
              ktimer = millis() + dittime;
              maddr_cmd(0, out);
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
        maddr_cmd(2, out);
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
        print_cw( out );
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
void ditcalc( char keywpm = 0 ) {
  
  if( keywpm <= 0 )
    keywpm = keyerwpm;

  int farn = keywpm - farns;
  if( farn <= 0 ) farn = keywpm;

  dittime    = DITCONST/keywpm;
  dahtime    = (DITCONST * 3)/keywpm;
  lettergap1 = (DITCONST * 2)/(keywpm);
  lettergap2   = (DITCONST * 2)/(farn);
  wordgap1   = (DITCONST * 6.5)/(keywpm);
  wordgap2   = (DITCONST * 6.5)/(farn);
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


void refresh_reset()
{
  while( !digitalRead(pinDit) || !digitalRead(pinDah) )
  {
    delay(5);
    tone( pinBuzz, 700, 200);
    delay(200);
    tone( pinBuzz, 900, 200);
    delay(200);
    tone( pinBuzz, 800, 200);
    delay(500);

    Keyboard.releaseAll();

    delay(100);

    resetFunc();
  }
}


void printchar( char ch )
{
  Keyboard.print( ch );
}

void clear_line( char ch )
{
  Keyboard.print('\n');
}


// transmit a CW message
void send_cwmsg(char *str, uint8_t prn ) {
  for (uint8_t i=0; str[i]; i++) {
    if (prn) {
      printchar(str[i]);
    }
    send_cwchr(str[i]);
  }
}

void print_line( char* str )
{
  char len = strlen( str );
  for( int x=0; x < len; x++)
    Keyboard.print(str[x]);
  Keyboard.print( '\n' );
}

void print_line( char ch )
{
  Keyboard.print( ch );
  Keyboard.print( '\n' );
}

void print_line( char ln, char* str )
{
  println( str );
}

void print_line( char ln, char ch )
{
  println( ch );
}

// back to run mode
void back2run() {
  menumode = RUN_MODE;
  println();
// print_line(0, "READY >>");
  delay(700);

  println();

}




// user interface menu to
// increase or decrease keyer speed
void menu_wpm() {
  uint8_t prev_wpm = keyerwpm;
  println();
  print_line(0, "SPEED IS");


  bool dirty = true;
  keyerstate = 0;
  keyerinfo = 0;
  last_ch = 0;

  iambic_keyer(false);
  while ( !(last_ch == 'N' || last_ch == 'A' || last_ch == 'X' ) ) {

    if (last_ch == 'T') {
      keyerwpm++;
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    if (last_ch == 'E') {
      keyerwpm--;
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    // check limits
    if (keyerwpm < MINWPM) keyerwpm = MINWPM;
    if (keyerwpm > MAXWPM) keyerwpm = MAXWPM;

    if( dirty )
    {
      itoa(keyerwpm,tmpstr,10);
      strcat(tmpstr," WPM");
      print_line(1, tmpstr);
      dirty = false;
    }

 
    iambic_keyer(false);
  }
  delay(10); // debounce


  if (prev_wpm != keyerwpm) {
    EEPROM[4] = keyerwpm;
    ditcalc();
  }

  if( last_ch == 'A' )
  {
    menu_trainer_farns();
  } 
  else if( last_ch == 'N' )
    menu_tone();
  else // 'X'
  {
    send_cwmsg("OK", 0);
    back2run();
  }

}


// local keyer tone menu to
// increase or decrease keyer tone
void menu_tone() {
  uint16_t prev_tone = keyertone;
  println();
  print_line(0, "KEYER TONE IS ");
  // wait until button is released

  bool dirty = true;
  keyerstate = 0;
  keyerinfo = 0;
  last_ch = 0;

  iambic_keyer(false);
  while ( !(last_ch == 'N' || last_ch == 'A' || last_ch == 'X' ) ) {
    if (last_ch == 'T') {
      keyertone+=10;
      tone(pinBuzz, keyertone );
      delay( dittime );
      noTone( pinBuzz);
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    if (last_ch == 'E') {
      keyertone-=10;
      tone(pinBuzz, keyertone );
      delay( dittime );
      noTone( pinBuzz);
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    // check limits
    if (keyertone < MINTONE) keyertone = MINTONE;
    if (keyertone > MAXTONE) keyertone = MAXTONE;

    if( dirty )
    {
      itoa(keyertone,tmpstr,10);
      strcat(tmpstr," Hz");
      print_line(1, tmpstr);
      dirty = false;
    }

    iambic_keyer(false);
}
  delay(10); // debounce
  // if wpm changed the recalculate the
  // dit timing and and send an OK message
  
  





  if (prev_tone != keyertone) {
    EEPROM[10] = buzz_tone = (keyertone / 10);
  }

  if( last_ch == 'A' )
  {
    menu_wpm();
  } 
  else if( last_ch == 'N' )
    menu_mode();
  else // 'X'
  {
    send_cwmsg("OK", 0);
    back2run();
  }




}

// remote keyer tone menu to
// increase or decrease keyer tone
void menu_trainer_mode() {
  uint16_t prev_lesson_mode = lesson_mode;

  bool dirty = true;

  println();

  keyerstate = 0;
  keyerinfo = 0;
  last_ch = 0;

  iambic_keyer(false);
  while ( !(last_ch == 'N' || last_ch == 'A' || last_ch == 'X' ) ) {
    //keyerinfo = 0;
    //read_paddles();
    if ( last_ch == 'T' ) {
      lesson_mode+=1;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    if ( last_ch == 'E' ) {
      lesson_mode-=1;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    // check limits
    if (lesson_mode < MINLESSONMODE) lesson_mode = MINLESSONMODE;
    if (lesson_mode > MAXLESSONMODE) lesson_mode = MAXLESSONMODE;


    char* lesson_seq;

    if( lesson_mode == 1 )
      lesson_seq = lesson_licw;
    else if( lesson_mode == 3 )
      lesson_seq = lesson_estonia;
    else
      lesson_seq = lesson_koch;



    switch (lesson_mode) {
      case 1:
        if( dirty )
        {
          println();
          println("TRAINING order ");
          println("LICW method ");
          println( lesson_licw );
          println("");
        }
        break;
      case 2:
        if( dirty )
        {
          println();
          println("TRAINING order ");
          println("Koch method ");
          println( lesson_koch );
          println("");
        }
        break;
      case 3:
        if( dirty )
        {
          println();
          println("TRAINING order ");
          println("ESTONIA method ");
          println( lesson_estonia );
          println("");
        }
        break;
      default:
        if( dirty )
        {
          println();
          println("TRAINER order ");
          println("ERROR ");
          println("");
        }
        break;
    }
    
    dirty = false;


    iambic_keyer(false);

  }
  delay(10); // debounce
  // if wpm changed the recalculate the
  // dit timing and and send an OK message


  if( lesson_mode == 1 )
    lesson_seq = lesson_licw;
  else if( lesson_mode == 3 )
    lesson_seq = lesson_estonia;
  else
    lesson_seq = lesson_koch;


  if (prev_lesson_mode != lesson_mode) {
    EEPROM[2] = (byte)lesson_mode;
    ditcalc();
  }
  if( last_ch == 'A' )
  {
    menu_mode();
  } 
  else if( last_ch == 'N' )
    menu_trainer_lesson();
  else // 'X'
  {
    send_cwmsg("OK", 0);
    back2run();
  }
}


// remote keyer tone menu to
// increase or decrease keyer tone
void menu_trainer_lesson() {
  uint16_t prev_lesson = lesson;
  println();
  // wait until button is released

  bool dirty = true;

  // loop until button is pressed
  keyerstate = 0;
  keyerinfo = 0;
  last_ch = 0;

  iambic_keyer(false);
  while ( !(last_ch == 'N' || last_ch == 'A' || last_ch == 'X' ) ) {

    if ( last_ch == 'T' ) {
      lesson+=1;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    if ( last_ch == 'E' ) {
      lesson-=1;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    // check limits
    if (lesson < MINLESSON) lesson = MINLESSON;
    if (lesson > MAXLESSON) lesson = MAXLESSON;


    char* lesson_seq;

    if( lesson_mode == 1 )
      lesson_seq = lesson_licw;
    else if( lesson_mode == 3 )
      lesson_seq = lesson_estonia;
    else
      lesson_seq = lesson_koch;





    if( dirty )
    {

      println(" ");

      sprintf( tmpstr, "LESSON %d ", lesson);
      //itoa((int)lesson,tmpstr,10);
      println( tmpstr );

      for( int i=0; i<(lesson+1) && i<=40; i++)
        print( lesson_seq[i] );

      println("");


      //lcds.print('\n');
      dirty = false;
    }

    iambic_keyer(false);
  }
  delay(10); // debounce
  // if wpm changed then recalculate the
  // dit timing and and send an OK message
  if (prev_lesson != lesson) {
    EEPROM[1] = (byte)lesson;
  }

  if( last_ch == 'N' )
    menu_lesson_window();
  else if( last_ch == 'A' )
    menu_trainer_mode();
  else //( last_ch == 'X')
  {
    ditcalc();
    send_cwmsg("OK", 0);
    back2run();
  } 
}




// remote keyer tone menu to
// increase or decrease keyer tone
void menu_lesson_window() {
  uint16_t prev_lesson_window = lesson_window;
  println();
  
  // wait until button is released
  while (sw1Pushed) {
    read_switch();
    delay(10);
  }

  bool dirty = true;

  // loop until button is pressed
  keyerstate = 0;
  keyerinfo = 0;
  last_ch = 0;

  iambic_keyer(false);
  while ( !(last_ch == 'N' || last_ch == 'A' || last_ch == 'X' ) ) {

    if ( last_ch == 'T' ) {
      lesson_window+=1;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    if ( last_ch == 'E' ) {
      lesson_window-=1;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      dirty = true;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
    }
    // check limits
    if (lesson_window <= MINWINDOW) lesson_window = MAXWINDOW;
    if (lesson_window > MAXWINDOW) lesson_window = MINWINDOW;

    if( lesson_window > lesson ) lesson_window = 0;
    
    if( dirty )
    {
      println("");

      print("WINDOW ");
      itoa(lesson_window,tmpstr,10);
      print( tmpstr );
      println( "" );

      int idx = (lesson+1) - lesson_window;
      if( idx <= 0 || idx >= (lesson+1) )
        idx = 0;


      char strwindow[51] = {0};
      char window = ( lesson_window == 0 ? (lesson+1) : lesson_window );
      strncpy( strwindow, &lesson_seq[idx], window );
      //strncat( strwindow, "                                                  ", 45-(window) );

      println( strwindow );
      println( "" );

      dirty = false;
    }

    iambic_keyer(false);

  }
  delay(10); // debounce
  // if wpm changed the recalculate the
  // dit timing and and send an OK message
  if ( prev_lesson_window != lesson_window ) {
    EEPROM[8] = (byte)lesson_window;
  }

  if( last_ch == 'N' )
    menu_trainer_lesson_size();
  else if( last_ch == 'A' )
    menu_trainer_lesson();
  else //( last_ch == 'X')
  {
    ditcalc();
    send_cwmsg("OK", 0);
    back2run();
  } 

}




// remote keyer tone menu to
// increase or decrease keyer tone
void menu_trainer_lesson_size() {

  uint16_t prev_lesson_size = lesson_size;
  println();
  println("TRAINING SIZE ");
  // wait until button is released
  while (sw1Pushed) {
    read_switch();
    delay(10);
  }
  println();

  // loop until button is pressed
  bool dirty = true;
  keyerstate = 0;
  keyerinfo = 0;
  last_ch = 0;

  iambic_keyer(false);
  while ( !(last_ch == 'N' || last_ch == 'A' || last_ch == 'X' ) ) {

    if ( last_ch == 'T' ) {
      if( lesson_size < 10 )
        lesson_size+=1;
      else
        lesson_size+=10;

      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
      dirty = true;
    }
    if ( last_ch == 'E' ) {
      if( lesson_size <= 10 )
        lesson_size-=1;
      else
        lesson_size-=10;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
      dirty = true;
    }
    // check limits
    if (lesson_size < MINLESSONCNT) lesson_size = MINLESSONCNT;
    if (lesson_size > MAXLESSONCNT) lesson_size = MAXLESSONCNT;

    if( dirty )
    {
      sprintf( tmpstr, "Lesson size %d ", lesson_size );
      println( tmpstr );
      println( "" );
      dirty = false;
    }

    iambic_keyer(false);
  }
  delay(10); // debounce


  // if wpm changed the recalculate the
  // dit timing and and send an OK message

  if (prev_lesson_size != lesson_size) {
    EEPROM[6] = (byte)lesson_size;
  }

  if( last_ch == 'N' )
    menu_trainer_farns();
  else if( last_ch == 'A' )
    menu_lesson_window();
  else //( last_ch == 'X')
  {
    ditcalc();
    send_cwmsg("OK", 0);
    back2run();
  } 
}




// remote keyer tone menu to
// increase or decrease keyer tone
void menu_trainer_farns() {
  uint16_t prev_farns = farns;
  println();
  print_line(0, "TRAINER");
  print_line(1, "FARNSWORTH");
  // wait until button is released
  while (sw1Pushed) {
    read_switch();
    delay(10);
  }
  // loop until button is pressed
  bool dirty = true;
  keyerstate = 0;
  keyerinfo = 0;
  last_ch = 0;

  iambic_keyer(false);
  while ( !(last_ch == 'N' || last_ch == 'A' || last_ch == 'X' ) ) {

    if (last_ch == 'T') {
      farns+=1;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
      dirty = true;
    }
    if (last_ch == 'E') {
      farns-=1;
      //tone(pinBuzz, keyertone );
      //delay( dittime );
      //noTone( pinBuzz);
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
      dirty = true;
    }
    // check limits
    if (farns < 0) farns = 0;
    if (farns > 15) farns = 15;

    
    if( dirty )
    {
      itoa(farns,tmpstr,10);
      strcat(tmpstr,"   ");
      print_line(2, tmpstr);
      dirty = false;
    }


    iambic_keyer(false);
  }
  delay(10); // debounce
  // if wpm changed the recalculate the
  // dit timing and and send an OK message


  if (prev_farns != farns) {
    EEPROM[3] = farns;
  }

  if( last_ch == 'N' )
    menu_wpm();
  else if( last_ch == 'A' )
    menu_trainer_lesson_size();
  else //( last_ch == 'X')
  {
    ditcalc();
    send_cwmsg("OK", 0);
    back2run();
  } 

}





// select keyer mode
void menu_mode() {
  uint8_t prev_mode = keyermode;
  println();
  print_line(0, "KEYER IS");
  // wait until button is released

  // loop until button is pressed
  bool dirty = true;
  keyerstate = 0;
  keyerinfo = 0;
  last_ch = 0;

  iambic_keyer(false);
  while ( !(last_ch == 'N' || last_ch == 'A' || last_ch == 'X' ) ) {

    if (last_ch == 'T' || last_ch == 'E') {
      keyermode = keyermode == IAMBICA ? IAMBICB : IAMBICA;
      keyerstate = 0;
      keyerinfo = 0;
      last_ch = 0;
      dirty = true;
    }

    if( dirty )
    {
      switch (keyermode) {
        case IAMBICA:
          print_line(1, "IAMBIC A");
          break;
        case IAMBICB:
          print_line(1, "IAMBIC B");
          break;
        default:
          print_line(1, "ERROR");
          break;
      }
      dirty = false;
    }

    iambic_keyer(false);

  }


  if (prev_mode != keyermode) 
  {
    EEPROM[3] = farns;
  }

  if( last_ch == 'N' )
    menu_trainer_mode();
  else if( last_ch == 'A' )
    menu_tone();
  else //( last_ch == 'X')
  {
    ditcalc();
    send_cwmsg("OK", 0);
    back2run();
  } 

}

// send a message
void menu_quiz() {
  char prev_wpm = keyerwpm;
test_again:
  if( lesson_mode )
  {
    ditcalc();

    char *quiz = "ALL WORK  AND NO PLAY MAKES  JACK A DULL BOY.  A DULL BOY.   A DULL BOY.";
    myrow = 0;
    mycol = 0;
    println();
    //println();
    print( "QUIZ MODE - ");
    char tmp[4];
    itoa( (int)keyerwpm, tmp, 10 );
    print( tmp );
    println( " WPM\n");

    delay(2200);


    char* lesson_seq;

    if( lesson_mode == 1 )
      lesson_seq = lesson_licw;
    else if( lesson_mode == 3 )
      lesson_seq = lesson_estonia;
    else
      lesson_seq = lesson_koch;

    int len = strlen(quiz);

    if( len > lesson_size )
    {
      len = lesson_size;
    }
    
    srandom( millis() );

    //char window = (lesson+1) - lesson_window;
    //if( window <=0 )
    //  window = 0;
    char window = ( lesson_window == 0 ? (lesson+1) : lesson_window );

    for( int i=0; i < len; i++ )
    {
      quiz[i] = (random() % 5) == 0 && quiz[i-1] != ' ' && lesson_size > 6 ? ' ' : lesson_seq[(window) + ( random() % ( (lesson+1) - (window) ) )];
    }
    quiz[len] = 0;
repeat:    
    send_cwmsg(quiz, 1);

    ditcalc(prev_wpm);
    println();

    keyerstate = 0;
    keyerinfo = 0;
    last_ch = 0;

    iambic_keyer(false);
    while ( !(last_ch == 'X' ) ) {
      if (last_ch == 'T') {
        keyerwpm++;
        goto test_again;
      }

      else if (last_ch == 'E') {
        keyerwpm--;
        goto test_again;
      }

      else if (last_ch == 'N') {
        goto test_again;
      }


      else if (last_ch == 'R') {
        println("Repeating...");
        delay(1500);
        goto repeat;
      }

      iambic_keyer(false);
    }

    keyerwpm = prev_wpm;
    ditcalc();
    back2run();
  }
  else
  {
    char *msg = "ALL WORK  AND NO PLAY MAKES  JACK A DULL BOY. ";
    myrow = 0;
    mycol = 0;
    println();
    // wait until button is released


    keyerstate = 0;
    keyerinfo = 0;
    last_ch = 0;

    iambic_keyer(false);
    while ( !(last_ch == 'E' ) ) {

      send_cwmsg(msg, 1);

      iambic_keyer(false);
    }

    back2run();
  }
}



// program setup
void setup() {
  // set GPIO
  pinMode(pinDit,  INPUT_PULLUP);
  pinMode(pinDah,  INPUT_PULLUP);
  pinMode(pinSw1,  INPUT_PULLUP);
  pinMode(pinBuzz, OUTPUT);
  pinMode(BUZZ_GND, OUTPUT);
  digitalWrite( BUZZ_GND, LOW );
  // startup init
  Serial.begin(BAUDRATE);     // init serial/debug port


  byte val = EEPROM[0];
  if( val != ver )
  {
    EEPROM[0] = ver;
    EEPROM[1] = lesson = (byte) 5;
    EEPROM[2] = lesson_mode = (byte) 1;
    EEPROM[3] = farns = (byte) 8;
    EEPROM[4] = keyerwpm = (byte) INITWPM;
    EEPROM[5] = keyermode = (byte) IAMBICA;
    EEPROM[6] = lesson_size = (byte) MAXLESSONCNT;
    EEPROM[7] = buzz_mode = (byte) 1;
    EEPROM[8] = lesson_window = (byte) 0;
    EEPROM[9] = vband_mode = (byte) 0;
    EEPROM[10] = buzz_tone = (byte) 65;

    //EEPROM.write()
  }
  else
  {
    lesson = EEPROM[1];
    lesson_mode = EEPROM[2];
    farns = EEPROM[3];
    keyerwpm = EEPROM[4];
    keyermode = EEPROM[5];
    lesson_size = EEPROM[6];
    buzz_mode = EEPROM[7];
    lesson_window = EEPROM[8];
    vband_mode = EEPROM[9];
    buzz_tone = EEPROM[10];
    //EEPROM.write()
  }

  keyertone = buzz_tone * 10;

  if( lesson_mode == 1 )
    lesson_seq = lesson_licw;
  if( lesson_mode == 3 )
    lesson_seq = lesson_estonia;
  else
    lesson_seq = lesson_koch;
    
  if( buzz_mode == 0 )
    pinBuzz = pinInnerBuzz;
  else
    pinBuzz = pinOuterBuzz;
  
  delay(1500);

  //lcd.setRowOffsets( 0, 20, 30 40 );

  ditcalc();

  send_cwchr('O');
  send_cwchr('K');

  if( vband_mode )
    pinBuzz = 0;

  Keyboard.begin();
  Keyboard.end();

  delay(100);
  Keyboard.begin();
  Keyboard.releaseAll();
  keyerstate = 0;
  keyerinfo = 0;
}

// main loop
void loop() {

  digitalWrite( BUZZ_GND, LOW );

  // no buttons pressed
  iambic_keyer();

  if( last_ch == ';' && next2last_ch == ';' )
  {
    println();
    println( "CW Keyboard");
    println( "Version");
    println( version );
    println();

    menu_trainer_mode();
  }

  if( last_ch == '\n' && next2last_ch == ';' )
    menu_quiz();
}
