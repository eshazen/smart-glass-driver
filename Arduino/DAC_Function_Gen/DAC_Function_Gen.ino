//
// Function generator for smart glass driver
//
// Generate sine waves from 10-1000 Hz
//
// Commands:
// h    - help
// f <freq> <amp>
//   <freq> is frequency in Hz.  Note that DAC is updated every 50us
//   <amp> is amplitude 0-2047
//

// output pins for InTENS board with MCP4921
#define DAC_LDAC 7

#define DAC_SDI 11
#define DAC_SCK 13

#define DAC_CS1 6
#define DAC_CS2 8

#include <TimerOne.h>
// #include "MCP320x.h"
#include "MCP4922.h"
#include "SineTable.h"
#include <SPI.h>

#define USE_ISR

// DAC 1
MCP4922 MYDAC1( DAC_SDI, DAC_SCK, DAC_CS1, DAC_LDAC);
MCP4922 MYDAC2( DAC_SDI, DAC_SCK, DAC_CS2, DAC_LDAC);

// NOTE:  if memory is tight/full, reduce size of samples[]
volatile int samples[500] = {2047, 2047};
#define MSAMP (sizeof(samples)/sizeof(samples[0]))
volatile int nsamp = 2;		// maximum sample# in samples[]
volatile int csamp = 0;		// current sample# in samples[]

void DAC_ISR(void) {
  MYDAC1.Set1(samples[csamp]);
  MYDAC2.Set1(4095-samples[csamp]);
  ++csamp;
  if( csamp >= nsamp)
    csamp = 0;
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100000);
  while( !Serial)
    ;

  Serial.println("SG 1.0");

  SPI.begin();
#ifdef DAC_SHDN
  pinMode( DAC_SHDN, OUTPUT);
  digitalWrite( DAC_SHDN, HIGH);
#endif

  TCCR0B = 0;			// stop timer 0

  // try to set up timer 1 for interrupts at PERIOD_US us (from SineTable.h)
#ifdef USE_ISR
  Timer1.initialize(PERIOD_US);
  Timer1.attachInterrupt(DAC_ISR);
  Timer1.start();
#endif
}

// maximum number of command arguments to parse
#define MAXARG 5

// these are global so they are included in the report of variable size use
static char buff[80];
static char* argv[MAXARG];
static unsigned iargv[MAXARG];

static int rate;
static int ampl;
static int rc;

void loop() {

  // send a prompt, wait for it to be transmitted
  Serial.write(">");    
  Serial.flush();
  
  // read a ststring into buff with editing
  my_gets( buff, sizeof(buff));

  // parse into text and integer tokens (see parse.ino)
  int argc = parse( buff, argv, iargv, MAXARG);

  switch( toupper( *argv[0])) {
  case 'H':
    Serial.println( "Commands:");
    Serial.println( "h    - help");
    Serial.println( "f <freq> <amp>");
    Serial.println( "  <freq> is frequency in Hz.  Note that DAC is updated every 50us");
    Serial.println( "  <amp> is amplitude 0-2047");

    break;

  case 'F':
    rate = iargv[1];
    ampl = iargv[2];

    rc = SineTable( rate, ampl, (int *)samples, MSAMP);
    if( rc > 0)
      nsamp = rc;
    break;

  default:
    Serial.print("Unknown command: ");
    Serial.println( *argv[0]);
  }

}
