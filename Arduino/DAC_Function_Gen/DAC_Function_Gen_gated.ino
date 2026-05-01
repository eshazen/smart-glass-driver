//
// Function generator for smart glass driver
//
// Generate sine waves from approximately 40-1000 Hz with the current
// 20 kHz DAC update rate and 500-sample waveform table.
//
// Commands:
// h                  - help
// f <freq> <amp>     - continuous sine wave
// g <freq> <amp> <gate_freq>
//                    - gated sine wave, 50% duty-cycle envelope
// x                  - force both DAC outputs to midscale/off
//

// output pins for InTENS board with MCP4921/MCP4922-style DAC wiring
#define DAC_LDAC 7

#define DAC_SDI 11
#define DAC_SCK 13

#define DAC_CS1 6
#define DAC_CS2 8

#include <TimerOne.h>
#include "MCP4922.h"
#include "SineTable.h"
#include <SPI.h>

#define USE_ISR

// DAC update rate. PERIOD_US is defined in SineTable.h.
#define DAC_UPDATE_RATE_HZ (1000000UL / PERIOD_US)

// Midscale/off code. Sending this to both DAC boards gives approximately
// zero differential drive between the two control inputs.
#define DAC_OFF_CODE 2048

// DAC 1 and DAC 2 share SDI/SCK/LDAC, but use different chip-select pins.
MCP4922 MYDAC1(DAC_SDI, DAC_SCK, DAC_CS1, DAC_LDAC);
MCP4922 MYDAC2(DAC_SDI, DAC_SCK, DAC_CS2, DAC_LDAC);

// NOTE: if memory is tight/full, reduce size of samples[].
// samples[] stores one full carrier sine-wave cycle as DAC codes.
volatile int samples[500] = {2047, 2047};
#define MSAMP (sizeof(samples) / sizeof(samples[0]))

// nsamp = active number of valid samples in samples[]
// csamp = current sample index used by the timer interrupt
volatile int nsamp = 2;
volatile int csamp = 0;

// Gate/envelope state.
// gateEnabled=false: output continuous sine wave.
// gateEnabled=true: output sine during the ON part and midscale during OFF.
volatile bool gateEnabled = false;
volatile bool forceOff = false;
volatile unsigned long gateCounter = 0;
volatile unsigned long gatePeriodTicks = 1;
volatile unsigned long gateOnTicks = 1;  // 50% duty cycle in this version

void writeOffState() {
  MYDAC1.Set1(DAC_OFF_CODE);
  MYDAC2.Set1(DAC_OFF_CODE);
}

void DAC_ISR(void) {
  bool outputOn = true;

  if (forceOff) {
    outputOn = false;
  } else if (gateEnabled) {
    // Start every gate-ON window from sine phase zero.
    // Since samples[0] is near midscale, this makes turn-on cleaner.
    if (gateCounter == 0) {
      csamp = 0;
    }

    outputOn = (gateCounter < gateOnTicks);

    ++gateCounter;
    if (gateCounter >= gatePeriodTicks) {
      gateCounter = 0;
    }
  }

  if (outputOn) {
    int s = samples[csamp];
    MYDAC1.Set1(s);
    MYDAC2.Set1(4095 - s);

    ++csamp;
    if (csamp >= nsamp) {
      csamp = 0;
    }
  } else {
    writeOffState();

    // Keep carrier phase parked at zero-crossing while fully off.
    if (!gateEnabled || forceOff) {
      csamp = 0;
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100000);
  while (!Serial)
    ;

  Serial.println(F("SG 1.1 gated"));

  SPI.begin();
#ifdef DAC_SHDN
  pinMode(DAC_SHDN, OUTPUT);
  digitalWrite(DAC_SHDN, HIGH);
#endif

  TCCR0B = 0;  // stop timer 0; do not rely on millis(), micros(), or delay()

#ifdef USE_ISR
  Timer1.initialize(PERIOD_US);
  Timer1.attachInterrupt(DAC_ISR);
  Timer1.start();
#endif
}

// maximum number of command arguments to parse
#define MAXARG 4

// these are global so they are included in the report of variable size use
static char buff[64];
static char *argv[MAXARG];
static int iargv[MAXARG];

static int rate;
static int ampl;
static int gateRate;
static int rc;

void printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("h    - help"));
  Serial.println(F("f <freq> <amp>"));
  Serial.println(F("  continuous sine wave"));
  Serial.println(F("g <freq> <amp> <gate_freq>"));
  Serial.println(F("  gated sine wave with 50% duty cycle"));
  Serial.println(F("  <freq> is carrier frequency in Hz"));
  Serial.println(F("  <amp> is amplitude 0-2047"));
  Serial.println(F("  <gate_freq> is ON/OFF envelope frequency in Hz"));
  Serial.println(F("x    - force both DACs to midscale/off"));
}

bool validCarrierSettings(int freq, int amp) {
  if (freq <= 0) {
    Serial.println(F("Error: carrier frequency must be > 0 Hz"));
    return false;
  }

  if (amp < 0 || amp > SIN_SCALE) {
    Serial.println(F("Error: amplitude must be 0-2047"));
    return false;
  }

  return true;
}

bool updateSineTable(int freq, int amp) {
  rc = SineTable(freq, amp, (int *)samples, MSAMP);

  if (rc <= 0) {
    Serial.println(F("Error: could not generate sine table"));
    Serial.println(F("With PERIOD_US=50 and samples[500], try carrier frequency >= about 40 Hz."));
    return false;
  }

  noInterrupts();
  nsamp = rc;
  csamp = 0;
  interrupts();

  Serial.print(F("Carrier samples per cycle: "));
  Serial.println(rc);
  return true;
}

void loop() {
  // send a prompt, wait for it to be transmitted
  Serial.write('>');
  Serial.flush();

  // read a string into buff with editing
  my_gets(buff, sizeof(buff));

  // parse into text and integer tokens (see parse.ino)
  int argc = parse(buff, argv, iargv, MAXARG);

  if (argc <= 0) {
    return;
  }

  switch (toupper(*argv[0])) {
    case 'H':
      printHelp();
      break;

    case 'F':
      if (argc < 3) {
        Serial.println(F("Usage: f <freq> <amp>"));
        break;
      }

      rate = iargv[1];
      ampl = iargv[2];

      if (!validCarrierSettings(rate, ampl)) {
        break;
      }

      // Park outputs at midscale while rewriting the waveform table.
      noInterrupts();
      forceOff = true;
      interrupts();

      if (updateSineTable(rate, ampl)) {
        noInterrupts();
        gateEnabled = false;
        forceOff = false;
        gateCounter = 0;
        csamp = 0;
        interrupts();
        Serial.println(F("Mode: continuous sine"));
      }
      break;

    case 'G':
      if (argc < 4) {
        Serial.println(F("Usage: g <freq> <amp> <gate_freq>"));
        break;
      }

      rate = iargv[1];
      ampl = iargv[2];
      gateRate = iargv[3];

      if (!validCarrierSettings(rate, ampl)) {
        break;
      }

      if (gateRate <= 0) {
        Serial.println(F("Error: gate frequency must be > 0 Hz"));
        break;
      }

      // Park outputs at midscale while rewriting the waveform table.
      noInterrupts();
      forceOff = true;
      interrupts();

      {
        unsigned long periodTicks = DAC_UPDATE_RATE_HZ / (unsigned long)gateRate;
        if (periodTicks < 2) {
          Serial.println(F("Error: gate frequency is too high for the DAC update rate"));
          break;
        }

        if (updateSineTable(rate, ampl)) {
          noInterrupts();
          gatePeriodTicks = periodTicks;
          gateOnTicks = periodTicks / 2;  // fixed 50% duty cycle
          if (gateOnTicks < 1) {
            gateOnTicks = 1;
          }
          gateCounter = 0;
          gateEnabled = true;
          forceOff = false;
          csamp = 0;
          interrupts();

          Serial.print(F("Mode: gated sine, gate frequency approximately "));
          Serial.print(DAC_UPDATE_RATE_HZ / gatePeriodTicks);
          Serial.println(F(" Hz, 50% duty"));
        }
      }
      break;

    case 'X':
      noInterrupts();
      forceOff = true;
      gateEnabled = false;
      gateCounter = 0;
      csamp = 0;
      interrupts();
      Serial.println(F("Mode: forced off/midscale"));
      break;

    default:
      Serial.print(F("Unknown command: "));
      Serial.println(*argv[0]);
  }
}
