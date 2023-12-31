//*-- Author :    Ole Hansen   07-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometer
//
// Abstract base class for a spectrometer.
//
// It implements a generic Reconstruct() method that should be
// useful for most situations. See specific description below.
//
// It also provides support for lists of different detector types
// (e.g. tracking/non-tracking/PID) for efficient processing.
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometer.h"
#include "THaParticleInfo.h"
#include "THaTrackingDetector.h"
#include "THaNonTrackingDetector.h"
#include "THaPIDinfo.h"
#include "THaTrack.h"
#include "TClass.h"
#include "TList.h"
#include "TMath.h"
#include "TList.h"
#include "VarDef.h"

#ifdef WITH_DEBUG
#include <iostream>
#endif

using namespace std;

//_____________________________________________________________________________
THaSpectrometer::THaSpectrometer( const char* name, const char* desc ) :
  THaApparatus(name, desc),
  fTracks{new TClonesArray("THaTrack", kInitTrackMultiplicity)},
  fTrackPID{new TClonesArray("THaPIDinfo", kInitTrackMultiplicity)},
  fTrackingDetectors{new TList},
  fNonTrackingDetectors{new TList},
  fPidDetectors{new TObjArray},
  fPidParticles{new TObjArray},
  fGoldenTrack{nullptr},
  fThetaGeo{0.0}, fPhiGeo{0.0},   fThetaSph{0.0}, fPhiSph{0.0},
  fSinThGeo{0.0}, fCosThGeo{1.0}, fSinPhGeo{0.0}, fCosPhGeo{1.0},
  fSinThSph{0.0}, fCosThSph{1.0}, fSinPhSph{0.0}, fCosPhSph{1.0},
  fPcentral{1.0},
  fCollDist{0.0},
  fStagesDone{0},
  fPID{false}
{
  // Constructor.
  // Protected. Can only be called by derived classes.

  THaSpectrometer::Clear();

  fProperties |= kNeedsRunDB;

  fTrkIfo.SetSpectrometer( this );
}

//_____________________________________________________________________________
THaSpectrometer::~THaSpectrometer()
{
  // Destructor. Delete the lists of specialized detectors.

  fPidParticles->Delete();   //delete all THaParticleInfo objects

  delete fPidParticles;          fPidParticles = nullptr;
  delete fPidDetectors;          fPidDetectors = nullptr;
  delete fNonTrackingDetectors;  fNonTrackingDetectors = nullptr;
  delete fTrackingDetectors;     fTrackingDetectors = nullptr;
  delete fTrackPID;              fTrackPID = nullptr;
  delete fTracks;                fTracks = nullptr;

  RemoveVariables();
}

//_____________________________________________________________________________
Int_t THaSpectrometer::AddDetector( THaDetector* pdet, Bool_t quiet,
                                    Bool_t first )
{
  // Add a detector to the internal lists of spectrometer detectors.
  // Duplicate detector names are not allowed.
  //
  // NOTE: The detector object must be allocated by the caller, but will be
  // deleted by the spectrometer. Do not delete detectors you've added
  // to an apparatus/spectrometer. Recommended: AddDetector( new MyDetector )
  //
  // NOTE: If an error occurs (wrong detector class, detector already exists),
  // the passed detector is deleted and -1 is returned.

  auto* sdet = dynamic_cast<THaSpectrometerDetector*>( pdet );
  if( !sdet ) {
    if( !quiet )
      Error("AddDetector", "Detector is not a THaSpectrometerDetector. "
	    "Detector not added. Detector object deleted.");
    delete pdet;
    return -1;
  }
  Int_t status = THaApparatus::AddDetector( pdet, quiet, first );
  if( status != 0 )
    return status;

  assert(sdet);
  if( sdet->IsTracking() )
    fTrackingDetectors->Add( sdet );
  else
    fNonTrackingDetectors->Add( sdet );

  if( sdet->IsPid() )
    fPidDetectors->Add( sdet );

  return 0;
}

//_____________________________________________________________________________
Int_t THaSpectrometer::AddPidParticle( const char* shortname, 
				       const char* name, 
				       Double_t mass, Int_t charge )
{
  // Add a particle type to the list of particles for which we want PID.
  // The "charge" argument is optional.
  
  fPidParticles->Add( new THaParticleInfo( shortname, name, mass, charge ));
  return 0;
}

//_____________________________________________________________________________
Int_t THaSpectrometer::CalcPID()
{
  // Combine the PID information from all detectors into an overall PID
  // for each track.  The actual work is done in the THaPIDinfo class.
  // This is just a loop over all tracks.
  // Called by Reconstruct().

  for( int i = 0; i < fTracks->GetLast()+1; i++ ) {
    if( auto* theTrack = static_cast<THaTrack*>( fTracks->At(i) ) ) {
      assert(dynamic_cast<THaTrack*>(theTrack));
      if( THaPIDinfo* pid = theTrack->GetPIDinfo() ) {
        pid->CombinePID();
      }
    }
  }    
  return 0;
}

//_____________________________________________________________________________
void THaSpectrometer::Clear( Option_t* opt )
{
  // Clear the spectrometer data for next event.

  THaApparatus::Clear(opt);
  // Clear the track array and also the track objects themselves since they
  // need to deallocate memory
  fTracks->Clear("C");
  TrkIfoClear();
  VertexClear();
  fGoldenTrack = nullptr;
  fStagesDone = 0;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaSpectrometer::Init( const TDatime& run_time )
{
  // Initialize spectrometer. First, ensure that the lists of detector types
  // are in a consistent state. Then, do the Apparatus initialization,
  // which reads our database and initializes the detectors. Finally, set up
  // the PID structures if PID enabled.

  ListInit();

  EStatus ret = THaApparatus::Init(run_time);
  if( ret )
    return ret;

  if( IsPID() )
    PidInit();

  // TODO: Set up vertex objects that can be associated with tracks?

  return kOK;
}

//_____________________________________________________________________________
void THaSpectrometer::DefinePidParticles()
{
  // Define the default set of PID particles:
  //  electron, pion, kaon, proton
  //
  // This spectrometer does not necessarily have to have PID detectors
  // that can identify all of these types. This is essentially a list of
  // convenient candidates for medium energy experiments.

  fPidParticles->Delete();    //make sure array is empty

  AddPidParticle( "e",  "electron", 0.511e-3, -1 );
  AddPidParticle( "pi", "pion",     0.139,     0 );
  AddPidParticle( "k",  "kaon",     0.4936,    0 );
  AddPidParticle( "p",  "proton",   0.938,     1 );
}

//_____________________________________________________________________________
Int_t THaSpectrometer::DefineVariables( EMode mode )
{
  // Define/delete standard variables for a spectrometer (tracks etc.)
  // Can be overridden or extended by derived (actual) apparatuses

  RVarDef vars[] = {
    { "tr.n",    "Number of tracks",             "GetNTracks()" },
    { "tr.x",    "Track x coordinate (m)",       "fTracks.THaTrack.fX" },
    { "tr.y",    "Track x coordinate (m)",       "fTracks.THaTrack.fY" },
    { "tr.th",   "Tangent of track theta angle", "fTracks.THaTrack.fTheta" },
    { "tr.ph",   "Tangent of track phi angle",   "fTracks.THaTrack.fPhi" },
    { "tr.p",    "Track momentum (GeV)",         "fTracks.THaTrack.fP" },
    { "tr.flag", "Track status flag",            "fTracks.THaTrack.fFlag" },
    { "tr.chi2", "Track's chi2 from hits",       "fTracks.THaTrack.fChi2" },
    { "tr.ndof", "Track's NDoF",                 "fTracks.THaTrack.fNDoF" },
    { "tr.d_x",  "Detector x coordinate (m)",    "fTracks.THaTrack.fDX" },
    { "tr.d_y",  "Detector y coordinate (m)",    "fTracks.THaTrack.fDY" },
    { "tr.d_th", "Detector tangent of theta",    "fTracks.THaTrack.fDTheta" },
    { "tr.d_ph", "Detector tangent of phi",      "fTracks.THaTrack.fDPhi" },
    { "tr.r_x",  "Rotated x coordinate (m)",     "fTracks.THaTrack.fRX" },
    { "tr.r_y",  "Rotated y coordinate (m)",     "fTracks.THaTrack.fRY" },
    { "tr.r_th", "Rotated tangent of theta",     "fTracks.THaTrack.fRTheta" },
    { "tr.r_ph", "Rotated tangent of phi",       "fTracks.THaTrack.fRPhi" },
    { "tr.tg_y", "Target y coordinate",          "fTracks.THaTrack.fTY"},
    { "tr.tg_th", "Tangent of target theta angle", "fTracks.THaTrack.fTTheta"},
    { "tr.tg_ph", "Tangent of target phi angle",   "fTracks.THaTrack.fTPhi"},    
    { "tr.tg_dp", "Target delta",                "fTracks.THaTrack.fDp"},
    { "tr.px",    "Lab momentum x (GeV)",        "fTracks.THaTrack.GetLabPx()"},
    { "tr.py",    "Lab momentum y (GeV)",        "fTracks.THaTrack.GetLabPy()"},
    { "tr.pz",    "Lab momentum z (GeV)",        "fTracks.THaTrack.GetLabPz()"},
    { "tr.vx",    "Vertex x (m)",                "fTracks.THaTrack.GetVertexX()"},
    { "tr.vy",    "Vertex y (m)",                "fTracks.THaTrack.GetVertexY()"},
    { "tr.vz",    "Vertex z (m)",                "fTracks.THaTrack.GetVertexZ()"},
    { "tr.pathl", "Pathlength from tg to fp (m)","fTracks.THaTrack.GetPathLen()"},
    { "tr.time",  "Time of track@Ref Plane (s)", "fTracks.THaTrack.GetTime()"},
    { "tr.dtime", "uncer of time (s)",           "fTracks.THaTrack.GetdTime()"},
    { "tr.beta",  "Beta of track",               "fTracks.THaTrack.GetBeta()"},
    { "tr.dbeta", "uncertainty of beta",         "fTracks.THaTrack.GetdBeta()"},
    { "status",   "Bits of completed analysis stages", "fStagesDone" },
    { nullptr }
  };

  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
const TVector3& THaSpectrometer::GetVertex() const
{
  // Return vertex vector of the Golden Track.
  // Overrides standard method of THaVertexModule.

  return (fGoldenTrack) ? fGoldenTrack->GetVertex() : fVertex;
}

//_____________________________________________________________________________
Bool_t THaSpectrometer::HasVertex() const
{
  return (fGoldenTrack) != nullptr && fGoldenTrack->HasVertex();
}

//_____________________________________________________________________________
void THaSpectrometer::ListInit()
{
  // Initialize lists of specialized detectors. Called by Init().

  fTrackingDetectors->Clear();
  fNonTrackingDetectors->Clear();
  fPidDetectors->Clear();

  TIter next(fDetectors);
  while( auto* theDetector = dynamic_cast<THaSpectrometerDetector*>( next() )) {

    if( theDetector->IsTracking() )
      fTrackingDetectors->Add( theDetector );
    else
      fNonTrackingDetectors->Add( theDetector );

    if( theDetector->IsPid() )
      fPidDetectors->Add( theDetector );
  }
}

//_____________________________________________________________________________
void THaSpectrometer::PidInit()
{
  // Initialize PID structures that can be associated with tracks

  // If not already done, set up this spectrometer's default PID particles
  if( fPidParticles->IsEmpty() )
    DefinePidParticles();

  UInt_t ndet = GetNpidDetectors();
  UInt_t npart = GetNpidParticles();

  // Set up initial set of PIDinfo objects
  for( Int_t i = 0; i < kInitTrackMultiplicity; i++ ) {
    new( (*fTrackPID)[i])  THaPIDinfo(ndet, npart);
  }
}

//_____________________________________________________________________________
Int_t THaSpectrometer::CoarseTrack()
{
  // Coarse Tracking: First step of spectrometer analysis

  // 1st step: Coarse tracking.  This should be quick and dirty.
  // Any tracks found are put in the fTrack array.
  TIter next( fTrackingDetectors );
  while( auto* theTrackDetector =
	 static_cast<THaTrackingDetector*>( next() )) {
    assert(dynamic_cast<THaTrackingDetector*>(theTrackDetector));
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "Call CoarseTrack() for " 
			<< theTrackDetector->GetName() << "... ";
#endif
    theTrackDetector->CoarseTrack( *fTracks );
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "done.\n";
#endif
  }

  fStagesDone |= kCoarseTrack;
  return 0;
}

//_____________________________________________________________________________
Int_t THaSpectrometer::CoarseReconstruct()
{
  // 2nd step: Coarse processing.  Pass the coarse tracks to the remaining
  // detectors for any processing that can be done at this stage.
  // This may include clustering and preliminary PID.
  // PID information is tacked onto the tracks as a THaPIDinfo object.

  if( !IsDone(kCoarseTrack) )
    CoarseTrack();

  TIter next( fNonTrackingDetectors );
  while( auto* theNonTrackDetector =
	 static_cast<THaNonTrackingDetector*>( next() )) {
    assert(dynamic_cast<THaNonTrackingDetector*>(theNonTrackDetector));
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "Call CoarseProcess() for " 
			<< theNonTrackDetector->GetName() << "... ";
#endif
    theNonTrackDetector->CoarseProcess( *fTracks );
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "done.\n";
#endif
  }

  fStagesDone |= kCoarseRecon;
  return 0;
}

//_____________________________________________________________________________
Int_t THaSpectrometer::Track()
{
  // Fine tracking.  Compute the tracks with high precision.
  // If coarse tracking was done, this step should simply "refine" the
  // tracks found earlier, not add new tracks to fTrack.

  if( !IsDone(kCoarseRecon) )
    CoarseReconstruct();

  TIter next( fTrackingDetectors );
  while( auto* theTrackDetector =
	 static_cast<THaTrackingDetector*>( next() )) {
    assert(dynamic_cast<THaTrackingDetector*>(theTrackDetector));
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "Call FineTrack() for " 
			<< theTrackDetector->GetName() << "... ";
#endif
    theTrackDetector->FineTrack( *fTracks );
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "done.\n";
#endif
  }

  // Reconstruct tracks to target/vertex
  // This usually also determines the track momentum
  FindVertices( *fTracks );

  fStagesDone |= kTracking;
  return 0;
}

//_____________________________________________________________________________
Int_t THaSpectrometer::Reconstruct()
{
  // This method implements a fairly generic algorithm for spectrometer
  // reconstruction which should be useful for most situations.
  // Additional, equipment-specific processing can be done in
  // a derived class that calls this method first.
  //
  // The algorithm is as follows:
  //
  // For all tracking detectors:
  //   CoarseTrack()
  // For all non-tracking detectors:
  //   CoarseProcess()
  // For all tracking detectors:
  //   FineTrack()
  // Reconstruct tracks to target
  // For all non-tracking detectors:
  //   FineProcess()
  // Compute additional attributes of tracks (e.g. momentum, beta)
  // Find "Golden Track"
  // Combine all PID detectors to get overall PID for each track
  //

  // Do prior analysis stages if not done yet
  if( !IsDone(kTracking) )
    Track();

  // Fine processing.  Pass the precise tracks to the
  // remaining detectors for any precision processing.
  // PID likelihoods should be calculated here.

  TIter next( fNonTrackingDetectors );
  while( auto* theNonTrackDetector =
	 static_cast<THaNonTrackingDetector*>( next() )) {
    assert(dynamic_cast<THaNonTrackingDetector*>(theNonTrackDetector));
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "Call FineProcess() for " 
			<< theNonTrackDetector->GetName() << "... ";
#endif
    theNonTrackDetector->FineProcess( *fTracks );
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "done.\n";
#endif
  }

  // Compute additional track properties (e.g. beta)
  // Find "Golden Track" if appropriate.

  TrackCalc();

  // Compute combined PID

  if( IsPID() )
    CalcPID();

  fStagesDone |= kReconstruct;
  return 0;
}

//_____________________________________________________________________________
void THaSpectrometer::TrackToLab( THaTrack& track, TVector3& pvect ) const
{
  // Convert TRANSPORT coordinates of 'track' to momentum vector 'pvect'
  // in the lab coordinate system (z = beam, y = up).
  // Uses the spectrometer angles from the database (loaded during Init())
  // for the transformation.
  // 
  // The track origin (vertex) is not calculated here because
  // doing so requires knowledge of beam positions and and angles.
  // Vertex calculations are done in a separate physics module.

  TransportToLab( track.GetP(), track.GetTTheta(), track.GetTPhi(), pvect );
}

//_____________________________________________________________________________
void THaSpectrometer::TransportToLab( Double_t p, Double_t th, Double_t ph,
				      TVector3& pvect ) const
{
  // Convert TRANSPORT vector to lab vector.
  // Inputs:
  //  p:  TRANSPORT momentum (absolute)
  //  th: Tangent of TRANSPORT theta
  //  ph: Tangent of TRANSPORT phi
  // Output:
  //  pvect: Vector in lab frame (z = beam, y = up) in same units as p.
  //
  // Note: Simple vector transformation can be done trivially by multiplying
  // with the appropriate rotation matrix, e.g.:
  //  TVector3 lab_vector = spect->GetToLabRot() * transport_vector;

  TVector3 v( th, ph, 1.0 );
  v *= p/TMath::Sqrt( 1.0+th*th+ph*ph );
  pvect = fToLabRot * v;
}

//_____________________________________________________________________________
void THaSpectrometer::LabToTransport( const TVector3& vertex, 
				      const TVector3& pvect,
				      TVector3& tvertex, Double_t* ray ) const
{
  // Convert lab coordinates to TRANSPORT coordinates in the spectrometer
  // coordinate system.
  // Inputs:
  //  vertex:  Reaction point in lab system
  //  pvect:   Momentum vector in lab
  // Outputs:
  //  tvertex: The vertex point in the TRANSPORT system, without any
  //           coordinate projections applied
  //  ray:     The TRANSPORT ray according to TRANSPORT conventions.
  //           This is an array of size 6 with elements x, tan(theta),
  //           y, tan(y), z, and delta.
  //           z is set to 0, and accordingly x and y are the TRANSPORT 
  //           coordinates in the z=0 plane. delta is computed with respect 
  //           to the present spectrometer's central momentum.
  //           Units are the same as of the input vectors.

  tvertex = fToTraRot * ( vertex - fPointingOffset );
  TVector3 pt = fToTraRot * pvect;
  if( pt.Z() != 0.0 ) {
    ray[1] = pt.X() / pt.Z();
    ray[3] = pt.Y() / pt.Z();
    // In the "ray", project the vertex to z=0
    ray[0] = tvertex.X() - tvertex.Z() * ray[1];
    ray[2] = tvertex.Y() - tvertex.Z() * ray[3];
  } else
    ray[0] = ray[1] = ray[2] = ray[3] = 0.0;
  
  ray[4] = 0.0;   // By definition for this ray, TRANSPORT z=0
  ray[5] = pt.Mag() / fPcentral - 1.0;
}

//_____________________________________________________________________________
void THaSpectrometer::SetCentralAngles( Double_t th, Double_t ph,
					Bool_t bend_down )
{
  // Given geographical angles theta and phi (in degrees), compute the
  // spectrometer's central angles in spherical coordinates and save trig.
  // values of angles for later use.
  // Note: negative theta corresponds to beam RIGHT.
  // phi should be close to zero unless this is a true out-of-plane spectrometer.
  // If 'bend_down' is true, the spectrometer bends downwards.

  fThetaGeo = TMath::DegToRad()*th; fPhiGeo = TMath::DegToRad()*ph;
  GeoToSph( fThetaGeo, fPhiGeo, fThetaSph, fPhiSph );
  fSinThGeo = TMath::Sin( fThetaGeo ); fCosThGeo = TMath::Cos( fThetaGeo );
  fSinPhGeo = TMath::Sin( fPhiGeo );   fCosPhGeo = TMath::Cos( fPhiGeo );
  Double_t st = fSinThSph = TMath::Sin(fThetaSph);
  Double_t ct = fCosThSph = TMath::Cos(fThetaSph);
  Double_t sp = fSinPhSph = TMath::Sin(fPhiSph);
  Double_t cp = fCosPhSph = TMath::Cos(fPhiSph);

  // Compute the rotation from TRANSPORT to lab and vice versa.
  Double_t norm = TMath::Sqrt(ct*ct + st*st*cp*cp);
  TVector3 nx( st*st*sp*cp/norm, -norm, st*ct*sp/norm );
  TVector3 ny( ct/norm,          0.0,   -st*cp/norm   );
  TVector3 nz( st*cp,            st*sp, ct            );
  if( bend_down ) { nx *= -1.0; ny *= -1.0; }
  fToLabRot.SetToIdentity().RotateAxes( nx, ny, nz );
  fToTraRot = fToLabRot.Inverse();
}

//_____________________________________________________________________________
Int_t THaSpectrometer::ReadRunDatabase( const TDatime& date )
{
  // Query the run database for parameters specific to this spectrometer
  // (central angles, momentum, offsets, drift, etc.)
  
  Int_t err = THaApparatus::ReadRunDatabase( date );
  if( err ) return err;

  FILE* file = OpenRunDBFile( date );
  if( !file ) return kFileError;

  Double_t th = 0.0, ph = 0.0;
  Double_t off_x = 0.0, off_y = 0.0, off_z = 0.0;

  const DBRequest req[] = {
    { "theta",    &th                       },
    { "phi",      &ph,        kDouble, 0, true },
    { "pcentral", &fPcentral                },
    { "colldist", &fCollDist, kDouble, 0, true },
    { "off_x",    &off_x,     kDouble, 0, true },
    { "off_y",    &off_y,     kDouble, 0, true },
    { "off_z",    &off_z,     kDouble, 0, true },
    { nullptr }
  };
  err = LoadDB( file, date, req );
  fclose(file);
  if( err )
    return kInitError;

  SetCentralAngles( th, ph, false );

  fPointingOffset.SetXYZ( off_x, off_y, off_z );

  return kOK;
}

//_____________________________________________________________________________
ClassImp(THaSpectrometer)

