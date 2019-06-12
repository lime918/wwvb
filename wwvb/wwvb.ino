/*************************************************************************************************
 * Arduino WWVB C-Max CMMR-6P-A2 clock v1.0 using the CMMR-6P-60 Eval Kit
 *
 * With apologies to Heathkit, we miss you! --> "The Almost Accurate Clock"
 *
 * Get the long tuned ferrite loopstick antenna and replace what's soldered to the board.
 *
 * Remote mount the receiver unit near a window if possible and well away from other electronic
 * equipment for best results.
 *
 * Continental US Summer longwave propagation will give best signal at sunrise and sunset.
 * Continental US Winter longwave propagation may give nearly 24 hr signal dependent on 
 * your location and local interference.
 *
 * capt.tagon's contributions
 *
 *   Decode WWVB data stream, Short impulse noise filtering, Drop bad frames,  
 *   Display: Signal Quality Indicator, Month & Day from Day of Year data,
 *   Time Zone & DST Indicator for Local Time Display. Extend clock function
 *   to increment day and year for midnight rollover and year end rollover.
 *
 *   Choose  UTC/Local Time, Choose between four US Time Zones for local time
 *
 *   Note that UTC time display and Time Zones are set with DIP switches, flip settings
 *   and hit reset button. Polling switches to dynamically change UTC/LocalTime/TZ messes
 *   up the receiver decoding by introducing oddball delays that make it mistime the falling edge.
 *   Dip-One 0=Local, Time 1=UTC | Dip-Two&Three 00=E, 01=C, 10=M, 11=P 
 *
 *   Frame Error Indicator - serves as signal indicator - Arduino serves as clock
 *   during the noisy parts of the day, as it starts picking up WWVB frames, it syncs
 *   to NIST time.
 *   1. Continuous full block indicates time as accurate as propagation delay will allow. 
 *   2. Block and half block alternating. Signal is degrading, sync update when block shows.
 *   3. Half block, 5 or less sequential frame errors, low bar more than 5 frame errors. 
 *   4. Low bar continuous, poor WWVB signal, no sync. Running on microprocessor clock only.
 *      Your time will only be as accurate as your Arduino crystal allows. Date functions
 *      will also rollover at midnight and year end.
 *
 * Contributed code that makes this all happen. Thank you for the informative websites, 
 * time spent and sharing your code. I know how much I spent re-kajiggering for WWVB whose
 * data stream is a testament to cold war technology .
 *
 * Captain           http://www.captain.at/electronic-atmega-dcf77.php
 *   DCF77 time reception and decoding using the ATMega 16
 * Mathias Dalheimer http://gonium.net/md/2006/11/05/arduino-dcf77-radio-clock-receiver/
 *   Interrupt driven clock and interrupt driven edge detection for DCF77
 * Rudi Niemeijer    http://www.rudiniemeijer.nl/wordpress/?p=516
 *   Amendments to DCF77 code, time display and addition of temperature functions
 * Peter H Anderson  http://phanderson.com/lcd106/lcd107.html
 *   LCD Display functions using LCD117 Serial LCD driver
 *************************************************************************************************/

//#include              // sprintf 2k penalty, abandon for Serial.print() if memory low
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define SPRITE_HEIGHT   16
#define SPRITE_WIDTH    16
#define LOGO_HEIGHT     32
#define LOGO_WIDTH      32

static const PROGMEM uint8_t logo_bmp[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x08, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xfc, 0x1c, 0x0e, 0x0e, 0x0e, 0x0c, 0x1c, 
0x1c, 0x38, 0x38, 0x70, 0xe0, 0xc0, 0xc0, 0x80, 0x00, 0x00, 0x01, 0x80, 0x82, 0xc0, 0xe0, 0x70, 
0x70, 0x38, 0x18, 0x1c, 0x1c, 0x0c, 0x0c, 0x0c, 0x1c, 0xf8, 0xf0, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x00, 0x6e, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x7f, 0xfc, 0xc0, 
0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xc0, 0xc0, 0xe0, 0xf1, 0xfb, 0xdf, 0xde, 0xdf, 0xfb, 
0xf3, 0xc1, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf0, 0xff, 0x3f, 0x01, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xc0, 0xe0, 0xf0, 0x38, 0x1c, 0x1c, 0x0e, 0x0e, 
0x06, 0x07, 0x07, 0x07, 0x1f, 0x7f, 0xf3, 0xe1, 0xf1, 0x3d, 0x1f, 0x07, 0x03, 0x01, 0x81, 0xe0, 
0xf1, 0xf1, 0xf0, 0xf1, 0xf0, 0xe1, 0xc3, 0x0f, 0x1f, 0x79, 0xf1, 0xc1, 0xe1, 0xf1, 0x7f, 0x1f, 
0x07, 0x03, 0x03, 0x07, 0x06, 0x0e, 0x0e, 0x0c, 0x1c, 0x38, 0x70, 0xe0, 0xc0, 0xb7, 0x40, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x07, 0x0f, 0x1c, 
0x38, 0x30, 0x70, 0x60, 0x60, 0xe0, 0xc0, 0xf0, 0xfc, 0xdf, 0x87, 0x83, 0x8f, 0x9e, 0xf8, 0xf0, 
0xc0, 0x80, 0x01, 0x07, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x07, 0xc3, 0xe0, 0xf0, 0xbc, 0x9e, 0x87, 
0x8f, 0xbe, 0xf8, 0xe0, 0xc0, 0xc0, 0xc0, 0xe0, 0xe0, 0x60, 0x70, 0x30, 0x38, 0x1c, 0x1e, 0x0f, 
0x03, 0xd6, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xfe, 0x1f, 0x03, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x8f, 0xdf, 0xfb, 0x73, 0xfb, 0xff, 0xcf, 0x87, 0x03, 0x03, 
0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x07, 0xff, 0xfc, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xee, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xa0, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x8f, 0x1f, 0xbc, 0x30, 
0xb0, 0x30, 0xb0, 0x38, 0x98, 0x9c, 0x0c, 0x8e, 0x87, 0x07, 0x83, 0x81, 0x00, 0x80, 0x80, 0x81, 
0x01, 0x83, 0x87, 0x0e, 0x8c, 0x1c, 0x98, 0x38, 0xb8, 0x30, 0xb0, 0x38, 0x9f, 0x8f, 0x03, 0x80, 
0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x6e, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};



  static const unsigned char PROGMEM fullstren_bmp[] =
{ B00000000, B00000000,
  B10010010, B00000000,
  B01010100, B01000000,
  B00111000, B01000000,
  B00010000, B01000000,
  B00010000, B01000000,
  B00010001, B01000000,
  B00010001, B01000000,
  B00010101, B01000000,
  B00010101, B01000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000 };

static const unsigned char PROGMEM midstren_bmp[] =
{ B00000000, B00000000,
  B10010010, B00000000,
  B01010100, B00000000,
  B00111000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00010001, B00000000,
  B00010001, B00000000,
  B00010101, B00000000,
  B00010101, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000 };

static const unsigned char PROGMEM lowstren_bmp[] =
{ B00000000, B00000000,
  B10010010, B00000000,
  B01010100, B00000000,
  B00111000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00010100, B00000000,
  B00010100, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000 };

static const unsigned char PROGMEM nostren_bmp[] =
{ B00000000, B00000000,
  B10010010, B00000000,
  B01010100, B00000000,
  B00111000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00010000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000 };

//Inputs
#define wwvbRxPin      2       // WWVB receiver digital input
//#define utcSwitchPin   8       // Localtime/UTC selector switch digital input
//#define tz1SwitchPin   9       // TZ selector switch digital input
//#define tz2SwitchPin  10       // TZ selector switch digital input
//#define tempSensorPin  5       // LM35 temperature sensor analog input
//Outputs
#define wwvbPdnPin     3 //1 - off (don't leave floating). 50ms startup delay after powerup required
#define wwvbAonPin     4 //agc hold control. hold low during noisy activity (e.g. stepper motors).
                         //has internal pullup. leave unconnected if not required.
//#define ledMarkPin   7         // LED mark indicator digital output
//#define ledFramePin  6         // LED frame indicator digitaloutput
//#define ledBitPin    5         // LED bit value indicator digital output
//#define ledRxPin     4         // LED indicator digital output

//Constants
#define WWVB_noise_millis 100  // Number of milliseconds in which we assume noise     (100ms)
#define WWVB_zero_millis  300  // Number of milliseconds for a logic 0 (100-300, 200ms nominal)
#define WWVB_one_millis   600  // Number of milliseconds for a logic 1 (400-600, 500ms nominal)
#define WWVB_mark_millis  900  // Number of milliseconds before we assume a mark    (700-900, 800ms nominal)

/* Definitions for the timer interrupt 2 handler
 * The Arduino runs at 16 Mhz, we use a prescaler of 64 -> We need to 
 * initialize the counter with 6. This way, we have 1000 interrupts per second.
 * We use tickCounter to count the interrupts.
 */
#define INIT_TIMER_COUNT 6
#define RESET_TIMER2 TCNT2 = INIT_TIMER_COUNT
int tickCounter = 0;

// Time variables
volatile byte ss;                        // seconds
volatile byte mm;                        // minutes
volatile byte hh;                        // UTC hours
volatile byte lhh;                       // local time hours
volatile byte day;                       // day
volatile word doy;                       // day of year
volatile byte mon;                       // month
volatile word year;                      // year
volatile byte dst;                       // daylight saving time flag
volatile byte lyr;                       // leap year flag
volatile byte prevday;                   // previous day for local time
volatile byte prevmon;                   // previous month for local time
volatile byte dayxing;                   // 00:00 UTC day change to Local

// WWVB receiver signal processing variables 
volatile byte wwvbSignalState = 0; // store wwvb receiver state 
byte previousSignalState;          // save previous receiver signal state
int previousFlankTime;             // detected signal edge time
int bufferPosition;                // buffer position counter
unsigned long long wwvbRxBuffer;   // 64 bit data buffer to hold 60 bits decoded from WWVB

/* WWVB time format struct - acts as an overlay on wwvbRxBuffer to extract time/date data.
 * All this points to a 64 bit buffer wwvbRxBuffer that the bits get inserted into as the
 * incoming data stream is decoded.
 */
struct wwvbBuffer {
  unsigned long long U12       :4;  // no value, empty four bits only 60 of 64 bits used
  unsigned long long Frame     :2;  // framing
  unsigned long long Dst       :2;  // dst flags
  unsigned long long Leapsec   :1;  // leapsecond
  unsigned long long Leapyear  :1;  // leapyear
  unsigned long long U11       :1;  // no value
  unsigned long long YearOne   :4;  // year (5 -> 2005)
  unsigned long long U10       :1;  // no value
  unsigned long long YearTen   :4;  // year (5 -> 2005)
  unsigned long long U09       :1;  // no value
  unsigned long long OffVal    :4;  // offset value
  unsigned long long U08       :1;  // no value
  unsigned long long OffSign   :3;  // offset sign
  unsigned long long U07       :2;  // no value
  unsigned long long DayOne    :4;  // day ones
  unsigned long long U06       :1;  // no value
  unsigned long long DayTen    :4;  // day tens
  unsigned long long U05       :1;  // no value
  unsigned long long DayHun    :2;  // day hundreds
  unsigned long long U04       :3;  // no value
  unsigned long long HourOne   :4;  // hours ones
  unsigned long long U03       :1;  // no value
  unsigned long long HourTen   :2;  // hours tens
  unsigned long long U02       :3;  // no value
  unsigned long long MinOne    :4;  // minutes ones
  unsigned long long U01       :1;  // no value
  unsigned long long MinTen    :3;  // minutes tens
};

// Decode variables for flags, counters and state
boolean signalNoise     = 0;       // noise detected
byte markCount          = 0;       // mark count, 6 pulses per minute
boolean prevMark        = 0;       // store previous mark
byte bitCount           = 0;       // bits, 60 pulses per minute
byte frameError         = 1;       // set for frame reject
word errorCount         = 0;       // keep count of frame errors

// Time Zone definition array, identifier & offset
byte TZ[4][2] = {
  {'E', 5},           // UTC -5 Eastern
  {'C', 6},           // UTC -6 Central
  {'M', 7},           // UTC -7 Mountain
  {'P', 8}            // UTC -8 Pacific
};

// Selector switch input variables UTC/Local & Time Zone Select
boolean displayUTC = 0;             // change to 1 to display UTC
byte selectTZ      = 3;             // 0=ET, 1=CT, 2=MT, 3=PT

// End of Month - to calculate Month and Day from Day of Year 
int eomYear[14][2] = {
  {0,0},      // Begin
  {31,31},    // Jan
  {59,60},    // Feb
  {90,91},    // Mar
  {120,121},  // Apr
  {151,152},  // May
  {181,182},  // Jun
  {212,213},  // Jul
  {243,244},  // Aug
  {273,274},  // Sep
  {304,305},  // Oct
  {334,335},  // Nov
  {365,366},  // Dec
  {366,367}   // overflow
};

byte previousSecond;             // store state to trip updates when second changes

boolean clockStarted = 0;        // valid time detected, clock started, clear screen

//float calculatedTemperature = 0; // calculated temperature from sensor

/**********************************************************************************
 * Arduino setup() routines to initialize clock 
 **********************************************************************************/

void setup(void) {
  Serial.begin(9600);            // Serial comms for debugging
  // UTC display inputs
//  pinMode(utcSwitchPin, INPUT);  // set pin mode input
//  pinMode(tz1SwitchPin, INPUT);  // set pin mode input
//  pinMode(tz2SwitchPin, INPUT);  // set pin mode input

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

//display.display();
//delay(2000);
  // Show atomic splash screen
  display.clearDisplay();

  display.drawBitmap(
    0,0,//(display.width() / 2),
    //(display.height() / 2),
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(2000);

  //utcSwitchCheck();              // dip switch read during restart - display LocalTime/UTC
  //tzSwitchCheck();               // dip switch read during restart - choose timezone to display
  //getTemperature();              // read temperature from sensor


        display.cp437(true);  
    display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  
  Serial.println(F("initializing receiver"));
  wwvbInit();                    // initialize clock, ISR for time correction

 
}

/**********************************************************************************
 * Main loop()
 **********************************************************************************/

void loop(void) {
  if (ss != previousSecond) {    // upon seconds change
    //if ((ss % 5) == 0) {         // every five seconds
    //  getTemperature();          // read temperature sensor and compute temp degrees Celsus
    //}
    #ifndef DEBUG_DATASTRUC
    serialDumpTime();            // print date, time to LCD
    #endif
    previousSecond = ss;         // store previous second
  }
  if (wwvbSignalState != previousSignalState) {  // upon WWVB receiver signal change
    scanSignal();                                // decode receiver data
    previousSignalState = wwvbSignalState;       // store previous receiver signal state
  }
}

/***************************************************************************************
 * wwvbInit()
 *
 * Initialize variables, set clock to zero, set pin modes, configure Timer2,
 * and attach interrupt for incoming WWVB data.
 ***************************************************************************************/

void wwvbInit() {
  previousSignalState = 0;               // clear variables
  previousFlankTime   = 0;
  bufferPosition      = 63;              // set receive buffer position for decrement
  wwvbRxBuffer        = 0;               // empty receive buffer

  ss=mm=hh=day=mon=year=0;               // initalize clock time to Zero

  // WWWVB Receiver input and indicators
  pinMode(wwvbRxPin, INPUT);             // set WWVB Rx pin input
  pinMode(wwvbPdnPin, OUTPUT);
  pinMode(wwvbAonPin, OUTPUT);
  //pinMode(ledRxPin, OUTPUT);             // set LED indicator outputs
  //pinMode(ledBitPin, OUTPUT);
  //pinMode(ledMarkPin, OUTPUT);
  //pinMode(ledFramePin, OUTPUT);
  
  // Timer2 Settings: Timer Prescaler /64, 
  TCCR2B |= (1<<CS22);                     // turn on CS22 bit
  TCCR2B &= ~((1<<CS21) | (1<<CS20));      // turn off CS21 and CS20 bits   
  // Use normal mode
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));    // turn off WGM21 and WGM20 bits 
  TCCR2B &= ~(1<<WGM22);                   // turn off WGM22
  // Use internal clock - external clock not used in Arduino
  ASSR |= (0<<AS2);
  
  digitalWrite(wwvbAonPin, HIGH); //leave on or disconnected.
  
  digitalWrite(wwvbPdnPin, HIGH); //shut down receiver.
  delay(100); //wait for fast powerup threshold (>50ms)
  digitalWrite(wwvbPdnPin, LOW); //restart receiver for faster signal acquisition.
  
  TIMSK2 |= (1<<TOIE2) | (0<<OCIE2A);      //Timer2 Overflow Interrupt Enable  
  RESET_TIMER2;

  attachInterrupt(0, int0handler, CHANGE); // attach interrupt handler and fire on signal change
}

/***********************************************************************************
 * Clock counter - The interrupt routine for counting seconds - increment hh:mm:ss.
 ***********************************************************************************/

ISR(TIMER2_OVF_vect) {
  RESET_TIMER2;
  tickCounter += 1;       // increment second
  if (tickCounter >= 1000) {
    ss++;
    if (ss >= 60) {       // increment minute
      ss = 0;
      mm++;
      if (mm >= 60) {     // increment hour
        mm = 0;
        hh++;
        if (hh >= 24) {   // increment day
          hh = 0;
          doy++;
          if (doy >= (366 + lyr)) { // incr year
            doy = 1;
            year++;
          }
        }
      }
    }
    tickCounter -= 1000; //accounts for cumulative errors. (bresenham's algorithm)
  }
}

/***********************************************************************
 * int0handler()
 *
 * Interrupthandler for INT0 - called when the signal on Pin 2 changes.
 ************************************************************************/

void int0handler() {
  // check the value again - since it takes some time to
  // activate the interrupt routine, we get a clear signal.
  wwvbSignalState = digitalRead(wwvbRxPin);         // read pulse level
 // digitalWrite(ledRxPin, wwvbSignalState);          // change Rx indicator LED to match
}

/*******************************************************************************************************
 * scanSignal()
 *
 * Evaluates the signal as it is received. Decides whether we received
 * a "mark", a "1" or a "0" based on the length of the pulse.
 *
 * We're decoding awesome US cold war technology here, no FEC, no parity. Attempt to remove noise spikes.
 * Count 1 frame start, 6 marks, 60 bits. Anything else is garbage, triggers a frame reject, clears 
 * the receive buffer and restarts recording bits.
 *
 * Hope no bits got flipped.
 *
 * Secret, remote mount antenna and receiver as far from your workbench as possible in a Ft. Collins, CO 
 * facing window with loop antenna broadside to that direction. Remove the stock tuned loopstick antenna
 * and replace with the long tuned version.
 *******************************************************************************************************/

void scanSignal(void){ 
   if (wwvbSignalState == 1) {             // see if receiver input signal is still high
      int thisFlankTime=millis();          // retrieve current time
      previousFlankTime=thisFlankTime;     // add time to count
    } 
    else {                                         // or a falling flank
      int difference=millis() - previousFlankTime; // determine pulse length
      signalNoise = 0;                             // clear signal noise detected
      //digitalWrite(ledMarkPin, LOW);        
      //digitalWrite(ledFramePin, LOW);        
      if (difference < WWVB_noise_millis) {        // below minimum - pulse noise
        // enough of this and it will cause bit flips and erroneous frame markers
        signalNoise = 1;
      }
        else if (difference < WWVB_zero_millis) {
          appendSignal(0);                           // decode bit as 0
          //digitalWrite(ledBitPin, HIGH);           // bit indicator LED on
          prevMark = 0;                              // set mark counter to zero
          bitCount ++;                               // increment bit counter
        } 
        else if (difference < WWVB_one_millis){
          appendSignal(1);                           // decode bit as 1
          //digitalWrite(ledBitPin, LOW);              // bit indicator LED off
          prevMark = 0;                              // set mark counter to zero
          bitCount ++;                               // increment bit counter
        }
        else {  // 10 second and frame markers
        // two sequential marks -> start of frame. If we read 6 marks and 60 bits,
        // we should have received a valid frame
        if ((prevMark == 1) && (markCount == 6) && (bitCount == 60)) { 
          appendSignal(0);
          //digitalWrite(ledFramePin, HIGH);        // frame received, ready for new frame
          markCount  = 0;                         // start counting marks, 6 per minute
          prevMark   = 0;                         // set bit counter to one
          bitCount   = 1;                         // should be a valid frame
          frameError = 0;                         // set frame error indicator to zero
          errorCount = 0;                         // set frame error count to zero
          finalizeBuffer();                       // hand off to decode time/date
        } else if ((prevMark == 1) && ((markCount != 6) || (bitCount != 60))) { // bad decode-frame reject
          //digitalWrite(ledFramePin, HIGH);        
          markCount  = 0;                         // bad start of frame set mark count to zero 
          prevMark   = 0;                         // clear previous to restart frame
          bitCount   = 1;                         // set bit count to one
          frameError = 1;                         // and indicate frame error
          errorCount ++;                          // increment frame error count
          bufferPosition = 63;                    // set rx buffer position to beginning
          wwvbRxBuffer   = 0;                     // and clear rx buffer
        } else {                                  // 10 second marker
          markCount ++;                           // increment mark counter, 6 per minute
          appendSignal(0);                        // marks count as 0
          //digitalWrite(ledMarkPin, HIGH);        
          prevMark = 1;                           // set mark state to one, following mark indicates frame
          bitCount ++;                            // increment bit counter
        }
      }
    }
}

/************************************************************************************************
 * appendSignal()
 *
 * Append a decoded signal bit to the wwvbRxBuffer and decrement bufferPosition counter. 
 * Argument can be 1 or 0. The bufferPosition counter shifts the writing position within 
 * the buffer over 60 positions. Reverse bit order by starting at 63 and working back towards
 * 0 to account for MSB0/LSB0 mismach between processor and transmitted data.
 ************************************************************************************************/

void appendSignal(byte signal) {
  // bitwise OR wwvbRxBuffer contents with signal bit shifted to bufferPosition 
  wwvbRxBuffer = wwvbRxBuffer | ((unsigned long long) signal << bufferPosition);
  bufferPosition--;                    // decrement bufferPosition for next bit
}

/************************************************************************************************
 * finalizeBuffer()
 *
 * Evaluate the information stored in the buffer. This is where the WWVB signal data is evaluated
 * and time/date values are decoded from BCD and the internal clock is updated.
 ************************************************************************************************/

void finalizeBuffer(void) {
  
  byte dmm;     // store decoded minutes
  byte dhh;     // store decoded hours
  byte ddst;    // store decoded dst flags
  
  // point wwvbBuffer structure at wwvbRxBuffer to allow simpler extraction of data bits to decode
  // WWVB time and date information.
  struct wwvbBuffer *rx_buffer;
  rx_buffer = (struct wwvbBuffer *)(unsigned long long)&wwvbRxBuffer;

  // convert the received bits from BCD
  dmm = (10 * bcdToDec(rx_buffer->MinTen)) + bcdToDec(rx_buffer->MinOne);
  dhh = (10 * bcdToDec(rx_buffer->HourTen)) + bcdToDec(rx_buffer->HourOne);
  ddst = bcdToDec(rx_buffer->Dst);
  doy = (100 * bcdToDec(rx_buffer->DayHun)) + (10 * bcdToDec(rx_buffer->DayTen)) 
        + bcdToDec(rx_buffer->DayOne);
  year = 2000 + (10 * bcdToDec(rx_buffer->YearTen)) + bcdToDec(rx_buffer->YearOne);
  lyr = rx_buffer->Leapyear;

  /* DST flags Q'nD, odds are DST, evens standard
   * actual flags
   * bit 55 dst   00 standard time   10 dst begins tonight    Decimal 0 and 2 respectively
   * bit 56 dst   11 dst in effect   01 dst ends tonight      Decimal 1 and 3 respectively
   */
  if (ddst == 3 || ddst == 1) { dst = 1; } else { dst = 0; }
  
  /* date/time data takes one full minute to transmit after its frame reference bit has been
   * received. Add one minute to synch with our clock with the following frame reference to 
   * WWVB time.
   */
  if (dmm == 59) {            // if decoded minute is 59
    mm = 0;                   // set minutes to zero
    hh = (dhh + 1 + 24) % 24; // and advance to next hour
  } else {                    // otherwise
    mm = dmm + 1;             // just add one minute to decoded minutes
    hh = dhh;
  }

  // Reset seconds to zero, buffer position counter to 63 and clear receive buffer
  ss             = 0;
  bufferPosition = 63;
  wwvbRxBuffer   = 0;
}

// LCD routines to initialize LCD and clear screen
//void lcdInit() {           // using P H Anderson Serial LCD driver board
//  Serial.print("?G216");   // configure driver for 2 x 16 LCD
//  delay(300);
//  Serial.print("?BDD");    // set backlight brightness
//  delay(300);
//  Serial.print("?D00000000000001F1F");  // special character low bar
//  delay(300);
//  Serial.print("?D10000001F1F1F1F1F");  // special character half block
//  delay(300);
//  Serial.print("?D21F1F1F1F1F1F1F1F");  // special character full block
//  delay(300);
//  Serial.print("?D31C141C0000000000");  // special character degree symbol
//  delay(300);
//  Serial.print("?f");      // clear screen
//}

//void clearLCD() {
//  Serial.print("?x00?y0?f"); // movie cursor to line 1 char 1, clear screen
//  delay(300);
//}

// convert BCD to decimal numbers
byte bcdToDec(byte val) {
  return ((val/16*10) + (val%16));
}

// read sensor value from LM35 temperature sensor and calculate temperature
// replace sensor with LM34 for degrees Fahrenheit
//void getTemperature() {
//  int sensorValue;
//  sensorValue = analogRead(tempSensorPin);
//  calculatedTemperature = (5.0 * sensorValue * 100.0) / 1024.0;
//}

// Dip Switch for UTC - read once on startup - polling messes up reading WWVB receive data 
//void utcSwitchCheck() {
//  boolean utcVal = digitalRead(utcSwitchPin);
//  if (utcVal == 1) {
//      displayUTC = 1 ;                         // Show UTC
//  }
//}

//void tzSwitchCheck() {
//  boolean tz1Val = digitalRead(tz1SwitchPin);
//  boolean tz2Val = digitalRead(tz2SwitchPin);
//  if (tz1Val == 0 && tz2Val == 0) { selectTZ = 0; } // TZ E
//  if (tz1Val == 0 && tz2Val == 1) { selectTZ = 1; } // TZ C
//  if (tz1Val == 1 && tz2Val == 0) { selectTZ = 2; } // TZ M
//  if (tz1Val == 1 && tz2Val == 1) { selectTZ = 3; } // TZ P
//}  
  

/********************************************************************************************
 * serialDumpTime()
 *
 * Dump the time, date and current temperature to the serial LCD
 ********************************************************************************************/

void serialDumpTime(void) {
  //int tempout;
  //int tempoutd;
  char timeString[12];
  char dateString[12];
  //char tempString[8];
  uint8_t * pSprite;

  //tempout = calculatedTemperature;
  //tempoutd = ((calculatedTemperature - tempout) * 10);


  display.setCursor(0,0);             // Start at top-left corner
    
  if (year == 0) {
    display.clearDisplay();

    // Display "Aquiring Frame:" and bit count. At 60, frame is finished and clock should sync.
    // Count restarting or going over 60 indicates bad signal reception. Move receiver/antenna
    // to better location or try during more radio signal quiescent time of day.
    display.print(F("Acquiring Frame:"));
    Serial.print("Acquiring Frame:");
    //Serial.print("?x00?y1");
    //Serial.print("?0?1?2");
    //Serial.print("?x07?y1");
    if (bitCount < 10) { display.print(F("0")); Serial.print("0"); }
    display.println(bitCount, DEC);
    Serial.println(bitCount, DEC);
    //Serial.print("?x13?y1");
    //Serial.print("?2?1?0");
    //Serial.print("?c0"); 
    display.display();

  } else {  
    // Hour, minutes and seconds
    // Flashing seconds colon
    if (clockStarted == 0) { display.clearDisplay(); }           // first time, clear display
    //Serial.print("?x00?y0");                         // cursor to row one, char 1

    // calculate local time from offset and DST flag
    lhh = ((hh + dst + 24) - TZ[selectTZ][1]) % 24; // calculate local time
    // calculate local hour equivalent to 00hrs UTC so we can correct our day change to 00hrs local
    dayxing = ((0 + dst + 24) - TZ[selectTZ][1]) % 24; // calculate local time

    /* DCF77 sends month and day information, we aren't so lucky.
     * WWVB only sends Day of Year, Month and Day will need to be calculated, and 02/29 added for leapyear.
     * We're given Day of Year, compare against month ending day and calculate month and day.
     * Use leapyear flag to add one day to February.
     */
    int eom = 0;  // eom counter used to determine month and day
    while (eomYear[eom][lyr] < doy) {        // calculate month and day for UTC
      day = doy - eomYear[eom][lyr];
      mon = (eom + 1) % 12;
      eom++;
    }//while(eom...

    // because of the local time offset the date changes early (Ohrs UTC). We need to calculate the
    // previous month and day to correct the local time date display.
    int peom = 0;  // eom counter used to determine previous month and day
    while (eomYear[peom][lyr] < (doy - 1)) {  // calculate previous month and day for local time offset
      prevday = (doy - 1) - eomYear[peom][lyr];
      prevmon = (peom + 1) % 12;
      peom++;
    }//while(eomY...
  

    if (displayUTC == 1) {                   // display either UTC or local time
      if ((ss % 2) == 0)
        sprintf(timeString, "%02d:%02d.%02d UTC ", hh, mm, ss);
      else
        sprintf(timeString, "%02d %02d.%02d UTC ", hh, mm, ss);
      display.println(timeString);
    }  else {
      if ((ss % 2) == 0)
        sprintf(timeString, "%02d:%02d.%02d %c", lhh, mm, ss, TZ[selectTZ][0]);
      else 
        sprintf(timeString, "%02d %02d.%02d %c", lhh, mm, ss, TZ[selectTZ][0]);
      
      display.print(timeString);
      if (dst == 1) { display.print(F("D")); } else { display.print(F("S")); }
      display.println(F("T "));
    }//if-else (display...

   /******************************************************************************************
    * frame error indicator - serves as signal indicator - arduino serves as clock
    * during the noisy parts of the day, as it starts picking up WWVB frames, it syncs
    * to NIST time.
    * 1. Continuous full block indicates time as accurate as propagation delay will allow. 
    * 2. Block and half block alternating. Signal is degrading, sync update when block shows.
    * 3. Half block, 5 or less sequential frame errors, low bar more than 5 frame errors. 
    * 4. Low bar continuous, poor WWVB signal, no sync. Running on microprocessor clock only.
    ******************************************************************************************/
    //Serial.print("?x00?y1");       // ANSI Standard YYYYMMDD Sortable date string
    if (displayUTC == 1) {         // UTC can directly use month & day
       sprintf(dateString, "%04d%02d%02d ", year, mon, day);
       display.println(dateString);
       // Display temperature in degrees Celsus
    //   sprintf(tempString, "%3d.%1d?3C",  tempout, tempoutd);
    //   Serial.print(tempString);
    } else if (lhh < dayxing) {    // Local time can use date info up to UTC date crossing
       sprintf(dateString, "%04d%02d%02d ", year, mon, day);
       display.println(dateString);
       // Display temperature in degrees Celsus
    //   sprintf(tempString, "%3d.%1d?3C",  tempout, tempoutd);
  //     Serial.print(tempString);
    } else {                       // Delay change of date till 00hrs local
       sprintf(dateString, "%04d%02d%02d ", year, prevmon, prevday);
       display.println(dateString);
       // Display temperature in degrees Celsus
    //   sprintf(tempString, "%3d.%1d?3C",  tempout, tempoutd);
    //   Serial.print(tempString);
    } //if-else-if...
    
    clockStarted = 1;

    //display signal strength bars
    if (frameError == 1 && errorCount < 5)           // five or less sequential frame errors
      pSprite = midstren_bmp; 
    else if (frameError == 1 & errorCount >= 5)   // more than 5 frame errors
      pSprite = lowstren_bmp;
    else
      pSprite = fullstren_bmp;                  // clear signal reception
   

    display.drawBitmap(
      (display.width()  - SPRITE_WIDTH ),
      (display.height() - SPRITE_HEIGHT),
      pSprite, SPRITE_WIDTH, SPRITE_HEIGHT, 1);


    
  }//if-else(year == 0)...

    Serial.println("updating display");
    display.display();
}//serialDumpTime()
