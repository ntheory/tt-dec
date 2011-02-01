// <BEHOLD the GPL!>
// ntheory's DSPlibFilter, a library for FIR filtering to complement DSPlib
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
// </BEHOLD>

#include <rfftw.h>
#include <fftw.h>
#include <string.h>
#include "DSPlibFilter.h"

// Basic constructor
DSPlibFilter::DSPlibFilter (unsigned int TapCount, fftw_real *Taps)
{
  // Clear everything to NULL.
  this->Taps            = NULL;
  this->BufferedSamples = NULL;

  this->OutputSample = DSPFILTER_INVALID;

  // Clear the tap count and position variables.
  this->TapCount          = 0;
  this->BufferedSamplePos = 0;

  // Get the tap count and allocate memory for the taps.
  this->TapCount = TapCount;
  this->Taps = new fftw_real [this->TapCount];

  // Copy the taps.
  memcpy (this->Taps, Taps, this->TapCount * sizeof (fftw_real));

  // Allocate the necessary amount of space for the current samples,
  // and allocate some space for an area to buffer samples.
  this->BufferedSamples = new fftw_real [this->TapCount];

  // Set all the samples (input, buffer, and output) to invalid values.
  for (int Loop = 0; Loop < this->TapCount; Loop++) {
    this->BufferedSamples [Loop] = DSPFILTER_INVALID;
  }
}

DSPlibFilter::~DSPlibFilter ()
{
  // Deallocate any memory we allocated.
  if (this->Taps            != NULL) delete this->Taps;
  if (this->BufferedSamples != NULL) delete this->BufferedSamples;

  // Zero the tap count.
  this->TapCount = 0;
}

// ----------------------------------------------------------------------------
// Sample based filter IO functions:
//   - GetSample
//   - PeekSample
//   - PutSample
//
// ----------------------------------------------------------------------------

fftw_real DSPlibFilter::GetSample  ()
{
  if (this->BufferedSamples [DSPFILTER_LAST_TAP] == DSPFILTER_INVALID) {
    // The input has run dry.
    return DSPFILTER_INVALID;
  }
  else {
    // Perform the filtering.
    this->Filter ();

    // Shift and invalidate the current and buffered samples.
    this->ShiftAndInvalidateSamples ();

    this->BufferedSamplePos--;
  }

  return this->OutputSample;
}

fftw_real DSPlibFilter::PeekSample ()
{
  return this->OutputSample;
}

long      DSPlibFilter::PutSample  (fftw_real Sample)
{
  if (this->BufferedSamplePos == this->TapCount) {
    // No room left.  We're one past the end.
    return DSPFILTER_INVALID;
  }
  else {
    this->BufferedSamples [this->BufferedSamplePos++] = (Sample / 32768.0);
  }
}

void DSPlibFilter::Filter ()
{
  this->OutputSample = 0.0;

  for (int Loop = 0; Loop < this->TapCount; Loop++) {
    this->OutputSample += this->Taps [Loop] * this->BufferedSamples [Loop];
  }
}

void DSPlibFilter::ShiftAndInvalidateSamples ()
{
  // Not that'd we'd really have a zero-tap filter but it can't hurt
  // that much to check.

  if ((this->TapCount > 1) &&
      (this->BufferedSamplePos == this->TapCount)) {
    // More than one tap and we've got a full buffer.
    memmove (this->BufferedSamples, &(this->BufferedSamples [1]), DSPFILTER_LAST_TAP * sizeof (fftw_real));

    this->BufferedSamples [DSPFILTER_LAST_TAP] = DSPFILTER_INVALID;
  }
}
