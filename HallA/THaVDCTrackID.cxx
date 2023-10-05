//*-- Author :    Ole Hansen   09-Mar-02

//////////////////////////////////////////////////////////////////////////
//
// THaVDCTrackID
//
// This is a utility class for the VDC tracking classes.
// It allows unique identification of tracks.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVDCTrackID.h"
#include "THaVDCCluster.h"
#include "THaVDCPoint.h"
#include <iostream>
#include <iomanip>

using namespace std;

//_____________________________________________________________________________
THaVDCTrackID::THaVDCTrackID( const THaVDCPoint* lower,
			      const THaVDCPoint* upper)
  : fLowerU(-1), fLowerV(-1), fUpperU(-1), fUpperV(-1)
{
  // Constructor that automatically determines pivot numbers
  // from the given THaVDCPoints.

  if( lower ) {
    if( auto* cluster = lower->GetUCluster())
      fLowerU = cluster->GetPivotWireNum();
    if( auto* cluster = lower->GetVCluster())
      fLowerV = cluster->GetPivotWireNum();
  }
  if( upper ) {
    if( auto* cluster = upper->GetUCluster())
      fUpperU = cluster->GetPivotWireNum();
    if( auto* cluster = upper->GetVCluster())
      fUpperV = cluster->GetPivotWireNum();
  }
}

//_____________________________________________________________________________
void THaVDCTrackID::Print( Option_t* ) const
{
  // Print ID description

  cout << " "
       << setw(5) << fLowerU
       << setw(5) << fLowerV
       << setw(5) << fUpperU
       << setw(5) << fUpperV
       << endl;
}
///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCTrackID)