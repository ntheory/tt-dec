#include <rfftw.h>
#include <SDL_audio.h>

// Wave generation functions.
void GenerateSine   (fftw_real *Out, int Length, double Frequency, double Amplitude, int SamplingRate);			// Generate a sine wave.  If the phase isn't specified it is set to zero.
void GenerateSine   (fftw_real *Out, int Length, double Frequency, double Amplitude, double Phase, int SamplingRate);
void GenerateCosine (fftw_real *Out, int Length, double Frequency, double Amplitude, int SamplingRate);			// Generate a cosine wave.  If the phase isn't specified it is set to zero.
void GenerateCosine (fftw_real *Out, int Length, double Frequency, double Amplitude, double Phase, int SamplingRate);

// Array related functions.
void MixArrays (fftw_real *In1, fftw_real *In2, fftw_real *Out, int Length);						// Mix arrays (in the audio sense).
void MixArrays (fftw_real *In1, fftw_real *In2, fftw_real *In3, fftw_real *In4, fftw_real *Out, int Length);
void MultiplyArrays (fftw_real *In1, fftw_real *In2, fftw_real *Out, int Length);					// Mix arrays (in the radio sense).  Doesn't automatically normalize to 1.
void MultiplyArrays (fftw_real *In1, fftw_real *In2, fftw_real *In3, fftw_real *In4, fftw_real *Out, int Length);
void SaveArray (fftw_real *Data, int Length, char *FileName, char *OutputDirectory);					// Save an array to a file.
void SaveArrayForSoX (fftw_real *Data, int Length, int SamplingRate, char *FileName, char *OutputDirectory);		// Save an array to a file (playable by SoX).
void Normalize (fftw_real *In, fftw_real *Out, int Length);								// Normalize an array of type "fftw_real".
void InvertArray (fftw_real *Data, int Length);										// Flip an array upside down (for processing IFFTs that come out inverted).
void CopyArray (fftw_real *Source, fftw_real *Destination, int Length);							// Copy an array to another array.

// Soundcard related functions.
int ConfigureSoundCard (int Channels, int Bits, int Rate, char *DeviceFile, int IOType);				// Get a handle to the soundcard.
int SyncSoundCard (int SoundCardHandle);										// Call IOCTL to sync the soundcard (used before writing).

// File related functions.
SDL_AudioSpec *GetSoundDataFromWAV (char *FileName, int DesiredRate,
		                    unsigned char **AudioBuffer, unsigned long *AudioBufferLength);			// Get data from a file, decode with SDL, and return at the desired rate.

// Conversion functions.
void ConvertToInts (fftw_real *Input, short *Output, int Length);							// Convert from reals to ints.
void ConvertToReals (short *Input, fftw_real *Output, int Length);							// Convert from ints to reals.

// Filter creation functions.
void CreateBandPassFilter (double CenterFreq, double Width, int SamplingRate, int Taps, fftw_real **Out);

// Defines needed to do our job.
#define		DSPLIB_CURRENT_DIRECTORY	"."

#define		SDL_AUDIO_BUFFER_SIZE		(2 << 31)
#define		SDL_AUDIO_DESIRED_CHANNELS	1

#define		SDL_AUDIO_DESIRED_FORMAT	AUDIO_S16SYS

// Externs for other programs to do our job.
extern const int DSPLIB_ANY_RATE;
extern const int DSPLIB_CHANNELS;
extern const int DSPLIB_BITS;
