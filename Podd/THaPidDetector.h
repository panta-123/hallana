#ifndef Podd_THaPidDetector_h_
#define Podd_THaPidDetector_h_

//////////////////////////////////////////////////////////////////////////
//
// THaPidDetector.h
//
// Abstract base class for a generic spectrometer detector capable of
// particle identification.
//
// This is a special THaNonTrackingDetector that is capable of
// providing particle identification information.  Typical examples are
// Cherenkov detectors and shower counters.
//
//////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"

class THaPidDetector : public THaNonTrackingDetector {
  
public:
  THaPidDetector() = default; // for ROOT I/O
  virtual ~THaPidDetector() = default;

  virtual Bool_t   IsPid() { return true; }

protected:

  //Only derived classes may construct me

  THaPidDetector( const char* name, const char* description,
                  THaApparatus* a = nullptr );

  ClassDef(THaPidDetector,0)  //ABC for a PID detector
};

#endif
