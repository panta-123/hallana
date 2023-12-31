Usage of ET in Hall A C++/Root Analyzer
=======================================

I. GENERAL REMARKS
------------------

The Event Transfer (ET) system was invented by the Jlab DAQ
group for obtaining data online, and for inserting data into the
datastream.  It is documented at:
coda.jlab.org/coda/Users_Guide/2_1_guide/API/ET/3.1/table_of_contents.htm

The CODA classes written by me are "wrappers" around the ET 
library (in the case of THaEtClient) and around the EVIO library
(in the case of THaCodaFile).  These two derive from abstract
class THaCodaData.  The main advantages of this scheme is 
to allow a common interface bwteen files and ET, and to 
provide a simple, safe usage (e.g. users cannot cause deadtime).
More info is at hallaweb.jlab.org/equipment/daq/codaclass.html

The example listed in the above URL, which is also in tstcoda_main.C
in the hana_decode directory, makes it clear how to use the 
interface.  The codaclasses are available separately, and 
compile on SunOS with CC compiler as well as Linux (RedHat 6.2) 
with g++ compiler.


II. SPECIFIC COMPILATION INSTRUCTIONS
-------------------------------------

Follow these steps to implement the CODA interface. 
There is more than one way to do it, but here is a simple way.
The code referred to here is presently in
haplix7:/home/rom/haAna/ana_0.5

 --------
| STEP 1 |  The main Makefile needs a flag ONLINE_ET turned on.
 --------   However, this should be off by default so that users 
            and user institutions don't need ET library (they only
            read data files). 
            Here are the diffs:

11a12,14
> # If compiling for ONLINE_ET analysis using ET system
> #export ONLINE_ET = 1
> 
30c33
< DCDIR         = hana_decode_1.3
---
> DCDIR         = hana_decode_1.4
53a57,69
> # ONLIBS is needed for ET
> # CODA environment variable must be set.  Examples are
> #   CODA = /adaqfs/coda/2.2        (in adaq cluster)
> #   CODA = /data7/user/coda/2.2    (on haplix cluster)
>   CODA = /data7/user/coda/2.2
>   LIBET = $(CODA)/Linux/lib/libet.so
>   ONLIBS = $(LIBET) -lieee -lpthread -ldl -lresolv
> 
>   ifdef ONLINE_ET
>      LIBS += $(ONLIBS)
>      GLIBS += $(ONLIBS)
>   endif
> 
64a81,84
> #ifdef ONLINE_ET
> SRC += THaOnlRun.C
> #endif
> 


 --------
| STEP 2 |  THaRun is modified to use abstract class THaCodaData.
 --------   Here are diffs:


[rom@haplix1 ana_0.5]$ diff THaRun.h THaRun_orig.h
23c23
< class THaCodaData;
---
> class THaCodaFile;
33c33
<   THaCodaData*  fCodaData;     //CODA data associated wih this run
---
>   THaCodaFile*  fCodaFile;     //CODA file associated wih this run


(In the .C file, we simply replace fCodaFile by fCodaData)
diff THaRun.C THaRun_orig.C
18d17
< #include "THaCodaData.h"
29c28
<   fCodaData = 0;
---
>   fCodaFile = 0;
39c38
<   fCodaData = new THaCodaFile;  //Do not open the file
---
>   fCodaFile = new THaCodaFile;  //Do not open the file
51c50
<   fCodaData   = new THaCodaFile;
---
>   fCodaFile   = new THaCodaFile;
66c65
<      fCodaData   = new THaCodaFile;
---
>      fCodaFile   = new THaCodaFile;
77c76
<   delete fCodaData;
---
>   delete fCodaFile;
84c83
<   return fCodaData ? fCodaData->codaClose() : 0;
---
>   return fCodaFile ? fCodaFile->codaClose() : 0;
120c119
<   // by fCodaData->codaRead()
---
>   // by fCodaFile->codaRead()
122c121
<   return fCodaData ? fCodaData->getEvBuffer() : 0;
---
>   return fCodaFile ? fCodaFile->getEvBuffer() : 0;
133,135c132,134
<   if( !fCodaData ) fCodaData = new THaCodaFile;
<   if( !fCodaData ) return -3;
<   return fCodaData->codaOpen( fFilename.Data() );
---
>   if( !fCodaFile ) fCodaFile = new THaCodaFile;
>   if( !fCodaFile ) return -3;
>   return fCodaFile->codaOpen( fFilename.Data() );
160c159
<   return fCodaData ? fCodaData->codaRead() : -1;
---
>   return fCodaFile ? fCodaFile->codaRead() : -1;



-------------------------------------------------------------

 --------
| STEP 3 |  A new class THaOnlRun can be created which inherits from
 --------   THaRun, and uses ET system instead of CODA file.  The use
            of word 'File' unfortunately remains, but this is neverthe-
            less a simple way to proceed.  Hopefully Ole will do something 
            like this.  Below is the THaOnlRun class.  This code is
            also in haplix7:/home/rom/haAna/ana_0.5
            Of course, this must be added to HallA_LinkDef.h

THaOnlRun.h:
============

#ifndef ROOT_THaOnlRun
#define ROOT_THaOnlRun

//+SEQ,CopyRight,T=NOINCLUDE.

//////////////////////////////////////////////////////////////////////////
//
// THaOnlRun
//
// Description of an online run using ET system. 
//
//////////////////////////////////////////////////////////////////////////

//*KEEP,TNamed.
#include "TNamed.h"
#include "THaRun.h"
//*KEEP,TDatime.
#include "TDatime.h"
//*KEND.

#include <limits.h>

class THaCodaData;

class THaOnlRun : public THaRun {
  
protected:
  TString  fComputer;   // computer where DAQ is running, e.g. 'adaql2'
  TString  fSession;    // SESSION = unique ID of DAQ, usually an env. var., e.g 'onla'
  UInt_t   fMode;       // mode (0=wait forever for data, 1=time out, recommend 1)
  UInt_t   fOpened;

public:
  THaOnlRun();
  THaOnlRun(const char* computer, const char* session, UInt_t mode);
  virtual  Int_t  OpenFile();
  virtual  Int_t  OpenFile( const char* computer, const char* session, UInt_t mode);
  
  ClassDef(THaOnlRun,0)   //Description of an online run using ET system
};

#endif


THaOnlRun.C
===========


//*-- Author :    Robert Michaels    March 2001 

//////////////////////////////////////////////////////////////////////////
//
// THaOnlRun :: THaRun
//
// Description of an online run using ET system to get data in real time.
// Inheriting from THaRun, this is essentially a TNamed object with
// additional support for a run number, filename, CODA file IO, 
// and run statistics.
//
//////////////////////////////////////////////////////////////////////////

//*KEEP,THaOnlRun.
#include "THaOnlRun.h"
//*KEND.

#include <iostream>
#include "THaCodaData.h"
#include "THaEtClient.h"
#include "string.h"

ClassImp(THaOnlRun)

//______________________________________________________________________________
THaOnlRun::THaOnlRun() : THaRun()
{
  // Default constructor

  fCodaData = new THaEtClient();
  fMode = 1;
  ClearEventRange();
}

//______________________________________________________________________________
THaOnlRun::THaOnlRun( const char* computer, const char* session, UInt_t mode) : THaRun(ses
sion), fComputer(computer), fSession(session), fMode(mode)
{
  // Normal constructor

  fCodaData = new THaEtClient();
  ClearEventRange();
}

//______________________________________________________________________________
Int_t THaOnlRun::OpenFile()
{
  // Open ET connection. 

  if (fComputer.IsNull() || fSession.IsNull()) {
      return -2;   // must set computer and session, at least;
  } 

  return fCodaData->codaOpen(fComputer, fSession, fMode);

}

//______________________________________________________________________________
Int_t THaOnlRun::OpenFile( const char* computer, const char* session, UInt_t mode )
{
// Set the computer name, session name, and mode, then open the 'file'.
// It isn't really a file.  It is an ET connection.

  fComputer = computer;
  fSession = session;
  fMode = mode;
  return OpenFile();
}


---------------------------------------------------

 --------
| STEP 4 |  Now one can use a 'setup' macro analogous to the 
 --------   original 'setup.C' which Ole made.  Here is setup_onl.C 
            in haplix7:/home/rom/haAna/ana_0.5

setup_onl.C
===========
{
//
//  Steering script for Hall A analyzer demo
//  This demo is similar to 'setup.C' secept that it uses
//  a THaOnlRun object to get data from ET system in real time.
//

// Set up the equipment to be analyzed.

THaApparatus* HRSE = new THaElectronHRS("Electron HRS");
gHaApps->Add( HRSE );

THaApparatus* BEAM = new THaBeam("Beamline");
gHaApps->Add( BEAM );

// Set up the event layout for the output file

THaEvent* event = new THaEvent;

// Set up the analyzer - we use the standard one,
// but this could be an experiment-specific one as well.

THaAnalyzer* analyzer = new THaAnalyzer;


// Define the Run to come from computer "adaqcp" (where CODA
// is running), using session "par1" (a unique session name of
// the DAQ system), and in mode 1 (the recommended "time-out" mode).

THaOnlRun* run = new THaOnlRun( "adaqcp", "par1", 1);


// Define the analysis parameters

analyzer->SetEvent( event );
analyzer->SetOutFile( "Afile.root" );
run->SetLastEvent( 10000 );

}


III. SPECIFIC RUN-TIME INSTRUCTIONS
===================================

Running ET, you need to know:

  1. What computer is running DAQ ?  E.g. "adaql2"
  2. What is the session ?  This is a unique identifiying tag of the DAQ.
     Usually $SESSION is an environment variable of the online accounts.
  3. What mode do you wish ?   Available are two modes:
     mode = 0  -->  Analyzer waits forever for data.
     mode = 1  -->  Analyzer times out after typ 30 seconds (e.g. if DAQ stops)

These parameters are passed to ET via the codaOpen method of THaEtClient.

In the "higher level" class THaOnlRun shown above, one can pass them via 
parameters in the constructor; this only sets the parameters, it doesn't
try to codaOpen.  
THaOnlRun* run = new THaOnlRun( "adaqcp", "par1", 1);

The call to codaOpen then gets done in THaAnalysis when it does this
Int_t status = run.OpenFile();

CODA environment variable must be set.  Examples are
   CODA = /adaqfs/coda/2.2        (in adaq cluster)
   CODA = /data7/user/coda/2.2    (on haplix cluster)
User's $LD_LIBRARY_PATH must include $CODA/$OSNAME/lib (OSNAME= SunOS or Linux) 
Example:  OSNAME=Linux ; export OSNAME 
LD_LIBRARY_PATH=/app/root/lib:$CODA/$OSNAME/lib:.   ; export LD_LIBRARY_PATH

A typical session in the analyzer may look like the following,
where the macros setup_onl.C and next_onl.C are listed here
and also, at present, in haplix7:/home/rom/haAna/ana_0.5

analyzer [0] .x setup_onl.C          // sets up ET system.
analyzer [1] analyzer->Process( *run )
Run 0                   // run is "zero" because you're in middle of run
ET rate 50.3 Hz in  6 sec, avg 50.3 Hz
ET rate 28.9 Hz in  7 sec, avg 46.0 Hz
1000
Event limit reached.
Processed 1000 events.

analyzer [2] evlen->Draw()

analyzer [3] .x next_onl.C         //  this gets more events.
number of events ? 
4000
set out file  
analyzer [4] analyzer->Process( *run )
ET rate  1.5 Hz in 103 sec, avg 34.9 Hz
 
(etc)

The histogram 'evlen' will automatically increment.
 
----------------------------------------

setup_onl.C was already listed above.

next_onl.C is similar but only redifines the run:


{
#include <iostream>
// next_onl.C :  Continue running....

int nevent;
cout << "number of events ? "<<endl;
cin >> nevent;

THaOnlRun* run = new THaOnlRun( "adaqs2", "tst1", 1);
analyzer->SetEvent( event );
analyzer->SetOutFile( "Afile.root" );
cout << "set out file  "<<endl<<flush;
run->SetLastEvent( nevent );

}





