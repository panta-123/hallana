////////////////////////////////////////////////////////////////////////
//
//       THaString.cxx  (implementation)
//
////////////////////////////////////////////////////////////////////////

#include "THaString.h"
#include "Textvars.h"   // For Podd::vsplit
#include "Helper.h"
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace std;

namespace THaString {

//_____________________________________________________________________________
int CmpNoCase( const string& r, const string& s )
{
  // Case insensitive compare.  Returns -1 if "less", 0 if equal, 1 if
  // "greater".

  string::const_iterator p =  r.begin();
  string::const_iterator p2 = s.begin();

  while (p != r.end() && p2 != s.end()) {
    if (toupper(*p) != toupper(*p2))
      return (toupper(*p) < toupper(*p2)) ? -1 : 1;
    ++p;
    ++p2;
  }

  return (r.size() == s.size()) ? 0 : (r.size() < s.size()) ? -1 : 1;
}

//_____________________________________________________________________________
string::size_type FindNoCase( string data, string chunk )
{
  // Find position of "chunk" in "data".  Case insensitive.

  Lower(data);
  Lower(chunk);
  return data.find(chunk);
}

//_____________________________________________________________________________
vector<string> Split( const string& s )
{
  // Split on whitespace.
  return Podd::vsplit(s);
}

//_____________________________________________________________________________
unsigned int Hex( const string& s )
{
  // Conversion to unsigned interpreting as hex.
  istringstream ist(s);
  unsigned int in = 0;
  ist >> hex >> in;
  return in;
}

//_____________________________________________________________________________
string ToLower( const string& s )
{
  // Return copy of this string converted to lower case.

  string result(s);
  // The bizarre cast here is needed for newer gccs
  transform( ALL(s), result.begin(), (int(*)(int))tolower );
  return result;
}

//_____________________________________________________________________________
string ToUpper( const string& s )
{
  // Return copy of this string converted to upper case.
  string result(s);
  transform( ALL(s), result.begin(), (int(*)(int))toupper );
  return result;
}

//_____________________________________________________________________________
void Lower( string& s )
{
  // Convert this string to lower case

  transform( ALL(s), s.begin(), (int(*)(int))tolower );
}

//_____________________________________________________________________________
void Upper( string& s )
{
  // Convert this string to upper case
  transform( ALL(s), s.begin(), (int(*)(int))toupper );
}

//_____________________________________________________________________________

}
