// <BEHOLD the GPL!>
// ntheory's DSPlib, a library for various DSP functions
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

#include <stdio.h>
#include <string.h>

#include <math.h>

#include <fcntl.h>		// For "open" call.
#include <sys/ioctl.h>		// For "ioctl" call.

#include <sys/soundcard.h>	// For the printer or something...

#include "DSPlib.h"

#define		DSPLIB_PI		3.14159

const int	DSPLIB_BITS_PER_SAMPLE	= 16;
const int	DSPLIB_CHANNELS		= 1;
const int	DSPLIB_ANY_RATE		= -1;

// Wave generation functions ------------------------------------------------------------------------------------------
//   Description:
//     These functions are used to generate various types of waves.  Currently there are only sine waves supported.
//
//   Notes:
//     These functions always use the fftw_real type so FFTW can work with them.
// --------------------------------------------------------------------------------------------------------------------

void GenerateSine (fftw_real *Out, int Length, double Frequency, double Amplitude, int SamplingRate)
{
  GenerateSine (Out, Length, Frequency, Amplitude, 0, SamplingRate);
}

void GenerateSine (fftw_real *Out, int Length, double Frequency, double Amplitude, double Phase, int SamplingRate)
{
  double TwoPi = DSPLIB_PI * 2;
  double SamplingRateOverLength = (double) SamplingRate / (double) Length;
  double Position, CurrentValue;
  double RadianPhase = Phase / (double) 360 * TwoPi;

  for (int Loop = 0; Loop < Length; Loop++) {
    Position = (((double) Loop / (double) Length) * TwoPi) / SamplingRateOverLength;
    Position += RadianPhase;

    CurrentValue = sin (Position * Frequency) * Amplitude;

    Out [Loop] = CurrentValue;
  }
}

void GenerateCosine (fftw_real *Out, int Length, double Frequency, double Amplitude, int SamplingRate)
{
  GenerateSine (Out, Length, Frequency, Amplitude, 90, SamplingRate);
}

void GenerateCosine (fftw_real *Out, int Length, double Frequency, double Amplitude, double Phase, int SamplingRate)
{
  GenerateSine (Out, Length, Frequency, Amplitude, Phase + ((double) 90.0), SamplingRate);
}

// Array related functions --------------------------------------------------------------------------------------------
//   Description:
//     These functions are used to work with arrays.
//
//   Notes:
//     These functions always use the fftw_real type so FFTW can work with them.
// --------------------------------------------------------------------------------------------------------------------

  // Mix two or four arrays using addition ----------------------------------------------------------------------------
  //   Notes:
  //     The first function allows two inputs, the second allows four.  Both functions divide the final output
  //     amplitude by the number of inputs.  This is usually used to mix audio so that it doesn't clip.
  // ------------------------------------------------------------------------------------------------------------------

    void MixArrays (fftw_real *In1, fftw_real *In2, fftw_real *Out, int Length)
    {
      for (int Loop = 0; Loop < Length; Loop++) {
        Out [Loop] = (In1 [Loop] + In2 [Loop]) / 2;
      }
    }

    void MixArrays (fftw_real *In1, fftw_real *In2, fftw_real *In3, fftw_real *In4, fftw_real *Out, int Length)
    {
      for (int Loop = 0; Loop < Length; Loop++) {
        Out [Loop] = (In1 [Loop] + In2 [Loop] + In3 [Loop] + In4 [Loop]) / 4;
      }
    }

  // Mix two arrays using multiplication ------------------------------------------------------------------------------
  //   Notes:
  //     This is usually used to mix two radio waves together to modulate them.
  // ------------------------------------------------------------------------------------------------------------------

    void MultiplyArrays (fftw_real *In1, fftw_real *In2, fftw_real *Out, int Length)
    {
      for (int Loop = 0; Loop < Length; Loop++) {
        Out [Loop] = In1 [Loop] * In2 [Loop];
      }
    }

  // Save an array to disk --------------------------------------------------------------------------------------------
  //   Notes:
  //     None.
  // ------------------------------------------------------------------------------------------------------------------

    void SaveArray (fftw_real *Data, int Length, char *FileName, char *OutputDirectory)
    {
      char *FinalFileName = new char [strlen (OutputDirectory) + strlen ("/") + strlen (FileName) + strlen (".dat") + 1];

      strcpy (FinalFileName, OutputDirectory);
      strcat (FinalFileName, "/");
      strcat (FinalFileName, FileName);
      strcat (FinalFileName, ".dat");					// This makes our life easier with xmgrace.

      FILE *OutFile = fopen (FinalFileName, "w");

      if (OutFile != NULL) {
        for (int Loop = 0; Loop < Length; Loop++) {
          fprintf (OutFile, "%lf\n", Data [Loop]);
        }

        fprintf (OutFile, "\n");
        fclose (OutFile);
      }
      else {
        printf ("fopen failed for %s\n", FinalFileName);
      }

      delete FinalFileName;
    }

  // Save an array to disk as a SoX readable .dat file ----------------------------------------------------------------
  //   Notes:
  //     SoX can play these data files as regular audio.
  // ------------------------------------------------------------------------------------------------------------------

    void SaveArrayForSoX (fftw_real *Data, int Length, int SamplingRate, char *FileName, char *OutputDirectory)
    {
      char *FinalFileName = new char [strlen (OutputDirectory) + strlen ("/") + strlen (FileName) + strlen (".sox.dat") + 1];
      fftw_real *NormalizedData = new fftw_real [Length];

      strcpy (FinalFileName, OutputDirectory);
      strcat (FinalFileName, "/");
      strcat (FinalFileName, FileName);
      strcat (FinalFileName, ".sox.dat");

      FILE *OutFile = fopen (FinalFileName, "w");

      Normalize (Data, NormalizedData, Length);				// This is required by the SoX format.

      if (OutFile != NULL) {
        fprintf (OutFile, "; Sample Rate %d\n", SamplingRate);

        for (int Loop = 0; Loop < Length; Loop++) {
          fprintf (OutFile, "%lf\n", Data [Loop]);
        }

        fprintf (OutFile, "\n");
        fclose (OutFile);
      }
      else {
        printf ("fopen failed for %s\n", FinalFileName);
      }

      delete NormalizedData;
      delete FinalFileName;
    }

  // Normalize an array -----------------------------------------------------------------------------------------------
  //   Notes:
  //     Normalizes an array of type "fftw_real".
  // ------------------------------------------------------------------------------------------------------------------

    void Normalize (fftw_real *In, fftw_real *Out, int Length)
    {
      // Find the largest element's absolute value.
      fftw_real Largest = fabs (In [0]);
      fftw_real Current;

      for (int Loop = 1; Loop < Length; Loop++) {
	Current = fabs (In [Loop]);

	if (Largest < Current) {
	  Largest = Current;
	}
      }

      // Now divide all the elements by the largest.
      for (int Loop = 0; Loop < Length; Loop++) {
	Out [Loop] = In [Loop] / Largest;
      }
    }

  // Calculate a power spectrum and perform band-pass filtering -------------------------------------------------------
  //   Notes:
  //     Two separate functions.  These functions have not been tested in a VERY long time.  This comment will be
  //     updated when I know they are both working.
  //
  //     The band-pass filter is not an FIR or IIR.  It works on frequency domain data and is simply an iterative
  //     approach to removing the necessary FFT buckets.
  // ------------------------------------------------------------------------------------------------------------------

    void CalculatePowerSpectrum (fftw_real *Series, fftw_real *PowerSpectrum, int DataSize)
    {
      PowerSpectrum [0] = Series [0] * Series [0];  // DC component

      for (int Loop = 1; Loop < (DataSize + 1) / 2; ++Loop) {
        PowerSpectrum [Loop] = Series [Loop] * Series [Loop] +
                               Series [DataSize - Loop] * Series [DataSize - Loop];
      }

      if (DataSize % 2 == 0) {
        PowerSpectrum [DataSize / 2] = Series [DataSize / 2] * Series [DataSize / 2];
      }
    }

    void BandPassFilter (fftw_real *Data, int Start, int End, int SamplingRate, int FFTSize)
    {
      int BucketSize = FFTSize / SamplingRate;

      // Start the loop after the DC coefficient (element 0)
      for (int Loop = 1; Loop < Start / BucketSize; Loop++) {
        Data [Loop] = 0;
      }

      for (int Loop = End / BucketSize; Loop < FFTSize; Loop++) {
        Data [Loop] = 0;
      }
    }

  // Invert an array --------------------------------------------------------------------------------------------------
  //   Notes:
  //     Poor man's 180 degree phase shift of time-domain data.
  // ------------------------------------------------------------------------------------------------------------------

    void InvertArray (fftw_real *Data, int Length)
    {
      for (int Loop = 0; Loop < Length; Loop++) {
        Data [Loop] = -Data [Loop];
      }
    }

  // Copy array -------------------------------------------------------------------------------------------------------
  //   Notes:
  //     None.
  //
  //   Future improvements:
  //     Use memcpy.
  // ------------------------------------------------------------------------------------------------------------------
    void CopyArray (fftw_real *Source, fftw_real *Destination, int Length)
    {
      for (int Loop = 0; Loop < Length; Loop++) {
        Destination [Loop] = Source [Loop];
      }
    }

// Soundcard related functions ----------------------------------------------------------------------------------------
//   Description:
//     These functions are used to make dealing with the soundcard a little easier.
//
//   Notes:
//     It is still the programmer's responsibility to get the data out of the sound card.
// --------------------------------------------------------------------------------------------------------------------

  // Configure the soundcard ------------------------------------------------------------------------------------------
  //   Description:
  //     Sets the soundcard to take a certain number of channels at a specified rate and number of bits per sample.
  //
  //   Notes:
  //     You can specify read (O_RDONLY), write (O_WRONLY), or both (O_RDWR).
  // ------------------------------------------------------------------------------------------------------------------

    int ConfigureSoundCard (int Channels, int Bits, int Rate, char *DeviceFile, int IOType)
    {
      int SoundCardHandle = open (DeviceFile, IOType);

      if (SoundCardHandle != -1) {
        // No error.  Make IOCTL calls to inform the hardware of our choices
        ioctl (SoundCardHandle, SOUND_PCM_WRITE_CHANNELS, &Channels);
        ioctl (SoundCardHandle, SOUND_PCM_WRITE_BITS,     &Bits);
        ioctl (SoundCardHandle, SOUND_PCM_WRITE_RATE,     &Rate);
      }

      return SoundCardHandle;
    }

  // Synchronize the soundcard ----------------------------------------------------------------------------------------
  //   Description:
  //     Makes sure the soundcard is done doing whatever it needs to do.
  //
  //   Notes:
  //     Use this before closing the soundcard while you're waiting for a sample to finish playing or before you
  //     write to the card.
  // ------------------------------------------------------------------------------------------------------------------

    int SyncSoundCard (int SoundCardHandle)
    {
      return ioctl (SoundCardHandle, SNDCTL_DSP_SYNC, 0);
    }

// File related functions ---------------------------------------------------------------------------------------------
//   Description:
//     Functions to simplify my life when getting sound data from a file.
//
//   Notes:
//     None.
// --------------------------------------------------------------------------------------------------------------------

  // Get sound data from a WAVE file ----------------------------------------------------------------------------------
  //   Description:
  //     This function is just a wrapper for SDL to make it very easy to open a file.  It assumes we'll be using mono,
  //     16-bit audio.  The sampling rate is specified by the user.
  //
  //   Notes:
  //     None.
  // ------------------------------------------------------------------------------------------------------------------

  SDL_AudioSpec *GetSoundDataFromWAV (char *FileName, int DesiredRate, unsigned char **AudioBuffer, unsigned long *AudioBufferLength)
  {
    SDL_AudioSpec BasicAudioInfo;
    SDL_AudioSpec *ReturnSample;

    // We want signed 16-bit samples in the system's endianness, one channel at DesiredRate.
    BasicAudioInfo.format   = SDL_AUDIO_DESIRED_FORMAT;
    BasicAudioInfo.channels = SDL_AUDIO_DESIRED_CHANNELS;
    BasicAudioInfo.freq     = DesiredRate;

    ReturnSample = SDL_LoadWAV (FileName, &BasicAudioInfo, AudioBuffer, (Uint32 *) AudioBufferLength);

    // Make sure the return sample is valid!
    if (ReturnSample == NULL) {
      return ReturnSample;
    }
    
    // If we don't care about the rate just use the one that is returned.
    if (DesiredRate == DSPLIB_ANY_RATE) DesiredRate = ReturnSample->freq;

    // Now make sure the format matches our desired format.

    if ((ReturnSample->channels != SDL_AUDIO_DESIRED_CHANNELS) ||
        (ReturnSample->freq     != DesiredRate) ||
        (ReturnSample->format   != SDL_AUDIO_DESIRED_FORMAT)) {
      // The format doesn't match what we want.  Convert it.
      SDL_AudioCVT AudioCVT;

      if (SDL_BuildAudioCVT (&AudioCVT,
	    		     ReturnSample->format,     ReturnSample->channels,     ReturnSample->freq,
			     SDL_AUDIO_DESIRED_FORMAT, SDL_AUDIO_DESIRED_CHANNELS, DesiredRate) == -1) {
	// Conversion couldn't happen.  Get rid of the audio data and return NULL.
	SDL_FreeWAV ((Uint8 *) *AudioBuffer);
	(*AudioBuffer) = (unsigned char *) NULL;
	ReturnSample   = (SDL_AudioSpec *) NULL;
      }
      else {
	// We can do it.  Let's do it.
	AudioCVT.buf = (Uint8 *) malloc (*AudioBufferLength * AudioCVT.len_mult);
        AudioCVT.len = *AudioBufferLength;

	memcpy (AudioCVT.buf, *AudioBuffer, *AudioBufferLength);

	SDL_FreeWAV (*AudioBuffer);
	(*AudioBuffer) = (unsigned char *) NULL;

	SDL_ConvertAudio (&AudioCVT);

	(*AudioBuffer)       = AudioCVT.buf;
	(*AudioBufferLength) = AudioCVT.len_cvt;

	ReturnSample->format   = SDL_AUDIO_DESIRED_FORMAT;
	ReturnSample->channels = SDL_AUDIO_DESIRED_CHANNELS;
	ReturnSample->freq     = DesiredRate;
      }
    }
    else {
      // It matches, do nothing.
    }

    return ReturnSample;
  }

// Conversion functions -----------------------------------------------------------------------------------------------
//   Description:
//     These functions are to convert between different types of data.  They will mostly be used in conjunction with
//       the soundcard.
//
//   Notes:
//     None.
// --------------------------------------------------------------------------------------------------------------------

  // Convert to integers from reals -----------------------------------------------------------------------------------
  //   Description:
  //     This converts an array of type "fftw_real" to an array of type "short".
  //
  //   Notes:
  //     No checking is performed to avoid clipping.  It is left up to coersion by the compiler.
  // ------------------------------------------------------------------------------------------------------------------

  void ConvertToInts (fftw_real *Input, short *Output, int Length)
  {
    for (int Loop = 0; Loop < Length; Loop++) {
      Output [Loop] = (short) Input [Loop];
    }
  }

  // Convert to reals from integers -----------------------------------------------------------------------------------
  //   Description:
  //     This converts an array of type "short" to an array of type "fftw_real".
  //
  //   Notes:
  //     None.
  // ------------------------------------------------------------------------------------------------------------------

  void ConvertToReals (short *Input, fftw_real *Output, int Length)
  {
    for (int Loop = 0; Loop < Length; Loop++) {
      Output [Loop] = (fftw_real) Input [Loop];
    }
  }

// Filter creation functions ------------------------------------------------------------------------------------------
//   Description:
//     These functions are to create different types of FIR filters.
//
//   Notes:
//     None.
// --------------------------------------------------------------------------------------------------------------------

  // Create a band pass filter ----------------------------------------------------------------------------------------
  //   Description:
  //     This function creates a band pass filter automatically calculating the parameters based on the "Taps" and
  //       "SamplingRate" parameters.
  //
  //   Notes:
  //     Not implemented yet.  Yay!
  // ------------------------------------------------------------------------------------------------------------------

  //void CreateBandPassFilter (double CenterFreq, double Width, int SamplingRate, int Taps, fftw_real **Out)
  //{
  //}
