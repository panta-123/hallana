Hall A C++ Event Decoder
==========================

Robert Michaels,  rom@jlab.org,  Updated Feb 2015

The interface is THaEvData.  There is a new "OO" decoder
CodaDecoder (replaces THaCodaDecoder) which is chosen by
the appropriate lines in /src/THaInterface.

```c++
gHaDecoder = CodaDecoder::Class();
```

### Notes from March 29, 2000

I.  To Set Up

By now you've probably figured out how to untar the file,
but in case not, do this

```shell
tar xvf hana_decode_version.tar
```

(where version = version number, e.g. 1.5)

This makes a directory ./hana_decode_version with the code.

I will keep the latest version on www.jlab.org/~rom   or:
/home/rom/public_html/hana_decode_version.tar in JLab CUE

The Makefile is shipped to work with the analyzer and just
make the library it needs.  To compile standalone test codes
see the notes about the "trick" in Makefile; you will need a
libHalla.so and the headers from the main analyzer.

To install into ana_0.5, see section IV.

II. Getting Started

 The easiest way to learn to use the decoder is to look at
 the examples of executibles.  They are documented in the
 Makefile.  Here are some more details.

 Test Executibles:

 tstcoda  --  test of the CODA interface to a file or
              ET client connection.

 tstio    --  simple tests of CODA I/O from a file.
              Reads a file and does something with it; depending
              on arguments it will printout, or filter to an output,
              or make a fast sum of event types.

 tdecpr   --  test of decoder with self-explanatory printouts.
              This is a 'trivial' first glimpse at what decoder does.

 tdecex   --  test of decoder, example for a use by a detector class.
              A developer writing a detector class should imitate this.

 etclient --  test of ET connection for online data.
              Either does a printout of data, or a fast access to test speed.

 prfact   --  standalone code to print the prescale factors and exit.

 (noted, 2015): a few more were added, see *main*.C

To understand how to use decoding classes, look at the 'main'
routines corresponding to the above executibles.  They are
tstcoda_main.C, tstio_main.C, tdecpr_main.C, tdecex_main.C,
etclient_main.C, and prfact_main.C.  For developers of a detector
class, see THaGenDetTest.C which shows the proposed usage of
decoding for a detector class.  However, that example is dated.
It is now preferable for users to look at Ole's guidlines and
imitate, for example, THaScintillator class in the parent directory.

To compile standalone codes, uncomment the line
export STANDALONE = 1
in the Makefile.
This allows to compile the test executibles and builds a library
libdc.a which is independent of the rest of the analyzer; something
you would want to do if you ran the THaScaler class package as a
standalone package.  To run scaler analysis you'll need to copy
this library:  "cp libdc.a ../hana_scaler/libdc_local.a" because
the THaScaler Makefile is looking for "libdc_local.a".


III.  Contents of this Distribution (this was old material, ca 2000, so
      I will delete it)

See *.h *.C, the main interface is THaEvData, and the OO decoder
is choosen in /src/THaInterface.  See also *main*.C for some
examples of implementation in standalone codes.

IV. How to make ana_0.XXX compatible with hana_decode_VERSION
    (e.g. XXX = 0.6 and VERSION = 1.5, or whatever it is).

1. As usual, do this in Makefile:

DCDIR is now hana_decode_VERSION

2. ONLINE_ET flag in Makefile controls whether ET system
   is used.  For off-site, it should be left off.  See
   the file README_ET for more details.
   













