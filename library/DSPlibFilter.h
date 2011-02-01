// DSPlibFilter.h
//
// An extension to DSPlib to perform FIR filtering.
// by ntheory
//
// March 26th, 2003 - Started development.

// This value is used to show if/when a value is invalid (like
// when the input runs dry, etc).
#define		DSPFILTER_INVALID		-10000000

// This value gets the last value in a filter tap set.
#define		DSPFILTER_LAST_TAP		(this->TapCount - 1)

class DSPlibFilter {
  public:
    // Basic constructor
    DSPlibFilter (unsigned int TapCount, fftw_real *Taps);

    // Destructor
    ~DSPlibFilter ();

    // Get or put a single sample into the filter.  Returns DSPFILTER_INVALID
    // if the input runs dry.  "Getting" a sample actually advances to the
    // next sample.  "Peeking" a sample doesn't advance to the next sample.
    fftw_real GetSample  ();
    fftw_real PeekSample ();
    long      PutSample  (fftw_real Sample);

  private:
    unsigned int TapCount;
    fftw_real *Taps;

    fftw_real *CurrentSamples;
    fftw_real *BufferedSamples;

    fftw_real OutputSample;

    long BufferedSamplePos;

    // Perform the actual filtering operation.
    void Filter ();

    // Shift and invalidate the current and buffered samples.
    void ShiftAndInvalidateSamples ();
};
