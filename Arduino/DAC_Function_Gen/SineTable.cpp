// #define TEST_MAIN

#include "SineTable.h"
#include "math.h"

#ifdef TEST_MAIN

#include <stdio.h>
#include <stdlib.h>

int main( int argc, char *argv[]) {
  if( argc < 2) {
    printf("Usage:  %s <rate> <nsamp>\n", argv[0]);
    exit(1);
  }

  int rate = atoi( argv[1]);
  int ns = atoi( argv[2]);

  int *d = (int *)calloc( ns, sizeof(int));

  int rc = SineTable( rate, d, ns);
  printf("# rc = %d\n", rc);
  if( rc > 0) {
    for( int i=0; i<rc; i++)
      printf("%d %d\n", i, d[i]);
    for( int i=0; i<rc; i++)
      printf("%d %d\n", i+rc, d[i]);
  }
}

// #define DEBUG


#endif



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

// calculate cycle period
#define CYCLE_PER (1.0/rate)

// calculate the theoretical number of samples in 1 period
#define SAMP_PER_PERIOD (CYCLE_PER/(1e-6*PERIOD_US))

int SineTable( int rate, int *dat, int mdat) {

  static float ang;		// working angle 0-2*PI
  static float ang_step;		// angle step
  static int nsamp;		// number of samples used
  static int rc;

  ang = 0;

#ifdef DEBUG
  printf("# rate=%d mdat=%d\n", rate, mdat);
  printf("# SAMP_PER_PERIOD = %f\n", SAMP_PER_PERIOD);
#endif  

  if( SAMP_PER_PERIOD < 4)	// too fast, not enough samples
    return -1;

  if( SAMP_PER_PERIOD <= mdat) {	// one WF cycle fits in buffer provided

    rc = nsamp = int(SAMP_PER_PERIOD); // round off number of samples

    // calculate angle step
    ang_step = (2.0*M_PI)/nsamp;
#ifdef DEBUG
    printf("# nsamp = %d ang_step = %f\n", nsamp, ang_step);
#endif    

    int *pd = dat;
    while( nsamp--) {
      *pd++ = SIN_SCALE * sin(ang) + SIN_OFFSET;
      ang += ang_step;
    }

  } else {			// one WF cycle doesn't fit

    return -1;

  }
  
  return rc;
}




