
//
// create a sine wave table
// 
// rate is the desired output rate in Hz
// dat is buffer to store the samples
// mdat is the maximum number of samples
//
// returns the actual number of samples or -1 on error
//
// expected to work only where the number of samples per cycle
// is reasonable.
//

#include "SineTable.h"

// calculate the theoretical number of samples in 1 period
#define SAMP_PER_PERIOD ((1.0/rate)/(1e-6*PERIOD_US))

int SineTable( int rate, int *dat, int mdat) {

  static float a;
  static float da;
  static float ap;

  a = 0;

  if( SAMP_PER_PERIOD < 4)	// too fast
    return -1;
  if( SAMP_PER_PERIOD <= mdat) {	// one cycle fits
    // calculate actual period
    int nsamp = int(SAMP_PER_PERIOD);
    ap = (1e-6*PERIOD_US)*nsamp; // actual length of period
    da = (2.0*PI)/
  } else {

  }
  

}
