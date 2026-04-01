//
// Eric's ADC and DAC test code for Arduino shield
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

volatile int dac_values[] = {0, 2047, 4095, 2047};
#define NDAC (sizeof(dac_values)/sizeof(dac_values[0]))
volatile int nsamp = 0;

void DAC_ISR(void) {
  MYDAC.Set1(dac_values[nsamp]);
  MYDAC.Set2(4095-dac_values[nsamp]);
  ++nsamp;
  if( nsamp >= NDAC)
    nsamp = 0;
}

void setup() {
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

void loop() {
#ifdef USE_ISR
  ;
#else
  DAC_ISR();
#endif  
}
