//
// Function generator for smart glass driver
//
// Generate sine waves from 10-1000 Hz
//

// these are pinouts for analog shield on Arduino with an Uno R3
#define DAC_SDI 11
#define DAC_SCK 13
#define DAC_CS 9
#define DAC_LDAC 7
#define DAC_SHDN 6

#include <TimerOne.h>
// #include "MCP320x.h"
#include "MCP4922.h"
#include "SineTable.h"
#include <SPI.h>

#define USE_ISR

// MCP320x MYADC(ADC_CS_PIN);
MCP4922 MYDAC( DAC_SDI, DAC_SCK, DAC_CS, DAC_LDAC);

volatile int samples[800] = {0, 2047, 4095, 2047};
#define MSAMP (sizeof(samples)/sizeof(samples[0]))
volatile int nsamp = 4;
volatile int csamp = 0;

void DAC_ISR(void) {
  MYDAC.Set1(samples[csamp]);
  MYDAC.Set2(4095-samples[csamp]);
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
  pinMode( DAC_SHDN, OUTPUT);
  digitalWrite( DAC_SHDN, HIGH);

  TCCR0B = 0;			// stop timer 0

  // try to set up timer 1 for interrupts at 125us)
#ifdef USE_ISR
  Timer1.initialize(PERIOD_US);
  Timer1.attachInterrupt(DAC_ISR);
  Timer1.start();
#endif
}

char buff[10];

void loop() {

  Serial.print("rate> ");

  my_gets( buff, sizeof(buff));

  int rate = atoi(buff);
  int rc = SineTable( rate, samples, MSAMP);
  Serial.println(rc);

  if( rc > 0)
    nsamp = rc;
  
}
