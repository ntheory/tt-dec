// <BEHOLD the GPL!>
// ntheory's tt-dec, a software-based, post processing style touch tone decoder
// Copyright (C) 2003  ntheory
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// </BEHOLD>

#include "../library/DSPlib.h"
#include "../library/DSPlibFilter.h"

#include <math.h>

// DTMF frequencies
#define		ROW1				697
#define		ROW2				770
#define		ROW3				852
#define		ROW4				941

#define		COL1				1209
#define		COL2				1336
#define		COL3				1477
#define		COL4				1633

// Amplitude of the waves to build the FIRs
#define		AMPLITUDE			2

// Duration parameters
#define		MIN_DTMF_DURATION_MS		24							// Minimum number of milliseconds to be a valid touch tone
#define		ACCUMULATOR_DURATION_MS		8							// The amount of time we average our FIR results over

#define		DURATION_THRESHOLD		(MIN_DTMF_DURATION_MS / ACCUMULATOR_DURATION_MS)	// The number of times to average our FIR results before checking for valid touch tones

// Don't detect anything when the input power is this low
#define		POWER_THRESHOLD			0.02

// The percentage error allowed for a touch tone
#define		STANDARD_DTMF_TOLERANCE		0.035							// The standard allowance
#define		SIDE_DTMF_TOLERANCE		(STANDARD_DTMF_TOLERANCE / 5.0)				// What I allow above and below in percent

// A scaling factor to make the FIRs sufficiently long to not confuse rows 1 and 2 (because they're very close) 
#define		FILTER_LENGTH_SCALE_FACTOR	2

// This is where we accumulate our samples before we average them.
typedef struct {
  fftw_real Row1, Row2, Row3, Row4;
  fftw_real Col1, Col2, Col3, Col4;
} AccumulatorsType;

// This is where we count how many times we've successfully detected a tone (we compare this to DURATION_THRESHOLD to see if
// we have a match)
typedef struct {
  int Row1, Row2, Row3, Row4;
  int Col1, Col2, Col3, Col4;
} CountersType;

AccumulatorsType Accumulators;
CountersType     Counters;

// The length of the filter (different for each input rate)
int FilterLength;

// The filter objects
DSPlibFilter *Row1Filter, *Row2Filter, *Row3Filter, *Row4Filter;
DSPlibFilter *Col1Filter, *Col2Filter, *Col3Filter, *Col4Filter;

// Functions to create and delete the filters
void CreateFilters (long Rate);
void DeleteFilters ();

// Function to check for present touch tones
void CheckDTMF (AccumulatorsType *Power);

bool ToneDetected;

int main (int argc, char **argv) {
  SDL_AudioSpec *AudioSpec = NULL;
  unsigned char *AudioBuffer = NULL;
  unsigned long AudioBufferLength;
  long Rate = DSPLIB_ANY_RATE;
  long MinDTMFDuration, DurationCounter = 0;
  fftw_real Row1Sample;
  char *InputFile = argv [1];

  // Check to see if we have an input file
  if (InputFile == NULL) {
    printf ("You need to enter an input file.\n");
    exit (0);
  }

  // Have SDL read the input file
  AudioSpec = GetSoundDataFromWAV (InputFile, Rate, &AudioBuffer, &AudioBufferLength);

  // Die if SDL doesn't like it
  if (AudioSpec == NULL) {
    printf ("SDL hates you.\n");
    exit (0);
  }

  // Get the sampling rate and calculate the minimum DTMF duration in samples, then calculate the filter length
  Rate = AudioSpec->freq;

  MinDTMFDuration = (unsigned long) (((fftw_real) Rate / (fftw_real) 1000.0) * (fftw_real) ACCUMULATOR_DURATION_MS);
  FilterLength = MinDTMFDuration * FILTER_LENGTH_SCALE_FACTOR;

  // See if the minimum DTMF duration is too short (just a sanity check)
  if (MinDTMFDuration == 0) {
    printf ("Minimum DTMF duration in samples is zero.  No good!\n");
    exit (0);
  }

  // Create the filters
  CreateFilters (Rate);

  // Initialize the accumulators and the counters
  Accumulators.Row1 = Accumulators.Row2 = Accumulators.Row3 = Accumulators.Row4 = 0.0;
  Accumulators.Col1 = Accumulators.Col2 = Accumulators.Col3 = Accumulators.Col4 = 0.0;

  Counters.Row1, Counters.Row2, Counters.Row3, Counters.Row4 = 0;
  Counters.Col1, Counters.Col2, Counters.Col3, Counters.Col4 = 0;

  ToneDetected = false;

  for (unsigned long Loop = 0; Loop < (AudioBufferLength / sizeof (short)) - FilterLength; Loop++) {
    // Grab a sample.  We need to typecast this array because the SDL routines make an array of shorts and our pointer
    // is to an array of unsigned chars (which is also why we have to divide by sizeof (short) above).
    fftw_real Sample = ((short *) AudioBuffer) [Loop];
    
    // Put the new sample into the filter objects
    Row1Filter->PutSample (Sample); Col1Filter->PutSample (Sample);
    Row2Filter->PutSample (Sample); Col2Filter->PutSample (Sample);
    Row3Filter->PutSample (Sample); Col3Filter->PutSample (Sample);
    Row4Filter->PutSample (Sample); Col4Filter->PutSample (Sample);

    // If the first filter gives valid output (the FIR is sufficiently primed) we can assume that the rest will
    // as well (they're all the same lenght with the same startup time) and we start our DTMF detection
    if ((Row1Sample = Row1Filter->GetSample ()) != DSPFILTER_INVALID) {
      // Increment the duration counter
      DurationCounter++;

      // Add up the filter outputs.  We rectify them with fabs so we can calculate the power by simple averaging later.
      // This is similar to rectifying an AC signal and calculating the RMS of the resulting DC.
      Accumulators.Row1 += fabs (Row1Sample);               Accumulators.Col1 += fabs (Col1Filter->GetSample ());
      Accumulators.Row2 += fabs (Row2Filter->GetSample ()); Accumulators.Col2 += fabs (Col2Filter->GetSample ());
      Accumulators.Row3 += fabs (Row3Filter->GetSample ()); Accumulators.Col3 += fabs (Col3Filter->GetSample ());
      Accumulators.Row4 += fabs (Row4Filter->GetSample ()); Accumulators.Col4 += fabs (Col4Filter->GetSample ());

      // If we've grabbed enough samples we can calculate the power
      if (DurationCounter == MinDTMFDuration) {
	// Reset the counter
	DurationCounter = 0;

	// Average all of the accumulators to get the power
	Accumulators.Row1 /= MinDTMFDuration; Accumulators.Col1 /= MinDTMFDuration;
	Accumulators.Row2 /= MinDTMFDuration; Accumulators.Col2 /= MinDTMFDuration;
	Accumulators.Row3 /= MinDTMFDuration; Accumulators.Col3 /= MinDTMFDuration;
	Accumulators.Row4 /= MinDTMFDuration; Accumulators.Col4 /= MinDTMFDuration;

	// Do the DTMF detection
	CheckDTMF (&Accumulators);

	// Clear the accumulators for the next step
        Accumulators.Row1 = Accumulators.Row2 = Accumulators.Row3 = Accumulators.Row4 = 0.0;
        Accumulators.Col1 = Accumulators.Col2 = Accumulators.Col3 = Accumulators.Col4 = 0.0;
      }
    }
  }

  if (!ToneDetected) {
    printf ("No tones detected.\n");
  }

  printf ("\n");

  // Delete the filters
  DeleteFilters ();
}

void CreateFilters (long Rate)
{
  void MakeFilter (fftw_real *CenterFilter, fftw_real *LowerEdgeFilter, fftw_real *UpperEdgeFilter,
                   fftw_real *FinalFilter,
		   double Frequency, double Amplitude, unsigned long Rate);

  fftw_real *CenterFilter = NULL;
  fftw_real *LowerEdgeFilter = NULL;
  fftw_real *UpperEdgeFilter = NULL;
  fftw_real *FinalFilter = NULL;

  // Allocate new filters.  There are four filters here for a reason.  The first (CenterFilter) is the
  // frequency in the DTMF standard of a row or column.  The second and third (LowerEdgeFilter and
  // UpperEdgeFilter) are to create a "fake window" so our filter will be a little more lenient like
  // the standard says it should.  The last filter is where all of the three previous filters are mixed
  // together to create the... uh, well... final filter.
  CenterFilter    = new fftw_real [FilterLength];
  LowerEdgeFilter = new fftw_real [FilterLength];
  UpperEdgeFilter = new fftw_real [FilterLength];
  FinalFilter     = new fftw_real [FilterLength];

  // If the allocation fails we'll quit
  if (!(CenterFilter && LowerEdgeFilter && UpperEdgeFilter && FinalFilter)) {
    printf ("Couldn't allocate filters\n");
    exit (0);
  }

  // Make the filters for the rows and columns

  MakeFilter (CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, (double) ROW1, AMPLITUDE, Rate);
  Row1Filter = new DSPlibFilter (FilterLength, FinalFilter);

  MakeFilter (CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, (double) ROW2, AMPLITUDE, Rate);
  Row2Filter = new DSPlibFilter (FilterLength, FinalFilter);

  MakeFilter (CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, (double) ROW3, AMPLITUDE, Rate);
  Row3Filter = new DSPlibFilter (FilterLength, FinalFilter);

  MakeFilter (CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, (double) ROW4, AMPLITUDE, Rate);
  Row4Filter = new DSPlibFilter (FilterLength, FinalFilter);

  MakeFilter (CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, (double) COL1, AMPLITUDE, Rate);
  Col1Filter = new DSPlibFilter (FilterLength, FinalFilter);

  MakeFilter (CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, (double) COL2, AMPLITUDE, Rate);
  Col2Filter = new DSPlibFilter (FilterLength, FinalFilter);

  MakeFilter (CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, (double) COL3, AMPLITUDE, Rate);
  Col3Filter = new DSPlibFilter (FilterLength, FinalFilter);

  MakeFilter (CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, (double) COL4, AMPLITUDE, Rate);
  Col4Filter = new DSPlibFilter (FilterLength, FinalFilter);

  // Delete the temporary filters
  delete CenterFilter;
  delete LowerEdgeFilter;
  delete UpperEdgeFilter;
  delete FinalFilter;

  // If we couldn't allocate a filter object we'll quit
  if (!(Row1Filter && Row2Filter && Row3Filter && Row4Filter &&
        Col1Filter && Col2Filter && Col3Filter && Col4Filter)) {
    printf ("Couldn't allocate a DSPlibFilter\n");
    exit (0);
  }
}

void DeleteFilters ()
{
  // Delete all of the filter objects we created in CreateFilters
  delete Row1Filter;
  delete Row2Filter;
  delete Row3Filter;
  delete Row4Filter;

  delete Col1Filter;
  delete Col2Filter;
  delete Col3Filter;
  delete Col4Filter;
}

void MakeFilter (fftw_real *CenterFilter, fftw_real *LowerEdgeFilter, fftw_real *UpperEdgeFilter,
                 fftw_real *FinalFilter,
		 double Frequency, double Amplitude, unsigned long Rate)
{
  // Generate the three tones (this is a special case hack of an FIR)
  GenerateSine (CenterFilter,    FilterLength, Frequency,                                     Amplitude, Rate);
  GenerateSine (LowerEdgeFilter, FilterLength, Frequency - (Frequency * SIDE_DTMF_TOLERANCE), Amplitude, Rate);
  GenerateSine (UpperEdgeFilter, FilterLength, Frequency + (Frequency * SIDE_DTMF_TOLERANCE), Amplitude, Rate);

  // Mix them together
  MixArrays (CenterFilter, CenterFilter, LowerEdgeFilter, UpperEdgeFilter, FinalFilter, FilterLength);
}

void CheckDTMF (AccumulatorsType *Power)
{
  fftw_real Average = 0.0;
  fftw_real LocalThreshold = 0.0;
  int RowCounter = 0;
  int ColCounter = 0;

  // Calculate the total power
  Average = Power->Row1 + Power->Row2 + Power->Row3 + Power->Row4 +
            Power->Col1 + Power->Col2 + Power->Col3 + Power->Col4;

  // Get the average of it by dividing by the number of inputs
  Average /= 8.0;

  // If the power is too low we should exit
  if (Average < POWER_THRESHOLD) {
    Counters.Row1 = Counters.Row2 = Counters.Row3 = Counters.Row4 = 0;
    Counters.Col1 = Counters.Col2 = Counters.Col3 = Counters.Col4 = 0;

    return;
  }

  // Ok, here's the meat of the algorithm.  Basically we look to see which filters have a power
  // greater than the average power.  If there isn't exactly one column and one row that is
  // larger than the average power then we aren't seeing a touch tone.  If there is more than
  // one column or more than one row we can't decypher which one is correct so we throw it away.
  //
  // We also keep track of how many times a row or column filter has passed this test.  Once
  // it has passed the test exactly DURATION_THRESHOLD times we'll print it out.  We then keep
  // counting the number of times it passes the test.  Once it fails we'll reset it to zero.
  // Since we only print success messages when it's exactly equal to DURATION_THRESHOLD the
  // touch tones can be as long as they'd like and they'll only be printed once.
  //
  // ::whew::

  // Increment the counters where necessary
  if (Power->Row1 > Average) { Counters.Row1++; RowCounter++; } else { Counters.Row1 = 0; }
  if (Power->Row2 > Average) { Counters.Row2++; RowCounter++; } else { Counters.Row2 = 0; }
  if (Power->Row3 > Average) { Counters.Row3++; RowCounter++; } else { Counters.Row3 = 0; }
  if (Power->Row4 > Average) { Counters.Row4++; RowCounter++; } else { Counters.Row4 = 0; }

  if (Power->Col1 > Average) { Counters.Col1++; ColCounter++; } else { Counters.Col1 = 0; }
  if (Power->Col2 > Average) { Counters.Col2++; ColCounter++; } else { Counters.Col2 = 0; }
  if (Power->Col3 > Average) { Counters.Col3++; ColCounter++; } else { Counters.Col3 = 0; }
  if (Power->Col4 > Average) { Counters.Col4++; ColCounter++; } else { Counters.Col4 = 0; }

  // Here's the big ugly test that will be simplified in the next version with arrays.  I was
  // tired, this was easy, and so it stays temporarily.  :)
  if ((RowCounter == 1) && (ColCounter == 1)) {
    // We detected two touch tones present at the same time.  Check to see if they've
    // been around long enough.
    if (Counters.Row1 == DURATION_THRESHOLD) {
      if (Counters.Col1 == DURATION_THRESHOLD) {
	printf ("1"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col2 == DURATION_THRESHOLD) {
	printf ("2"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col3 == DURATION_THRESHOLD) {
	printf ("3"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col4 == DURATION_THRESHOLD) {
	printf ("A"); fflush (stdout); ToneDetected = true;
      }
    }
    else if (Counters.Row2 == DURATION_THRESHOLD) {
      if (Counters.Col1 == DURATION_THRESHOLD) {
	printf ("4"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col2 == DURATION_THRESHOLD) {
	printf ("5"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col3 == DURATION_THRESHOLD) {
	printf ("6"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col4 == DURATION_THRESHOLD) {
	printf ("B"); fflush (stdout); ToneDetected = true;
      }
    }
    else if (Counters.Row3 == DURATION_THRESHOLD) {
      if (Counters.Col1 == DURATION_THRESHOLD) {
	printf ("7"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col2 == DURATION_THRESHOLD) {
	printf ("8"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col3 == DURATION_THRESHOLD) {
	printf ("9"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col4 == DURATION_THRESHOLD) {
	printf ("C"); fflush (stdout); ToneDetected = true;
      }
    }
    else if (Counters.Row4 == DURATION_THRESHOLD) {
      if (Counters.Col1 == DURATION_THRESHOLD) {
	printf ("*"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col2 == DURATION_THRESHOLD) {
	printf ("0"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col3 == DURATION_THRESHOLD) {
	printf ("#"); fflush (stdout); ToneDetected = true;
      }
      else if (Counters.Col4 == DURATION_THRESHOLD) {
	printf ("D"); fflush (stdout); ToneDetected = true;
      }
    }
  }
  else {
    // We didn't detect two touch tones present at the same time.  Clear the counters.
    Counters.Row1 = Counters.Row2 = Counters.Row3 = Counters.Row4 = 0;
    Counters.Col1 = Counters.Col2 = Counters.Col3 = Counters.Col4 = 0;
  }
}
