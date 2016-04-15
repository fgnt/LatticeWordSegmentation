// ----------------------------------------------------------------------------
/**
   File: definitions.hpp

   Status:         Version 1.0
   Language: C++

   License: UPB licence

   Copyright (c) <2013> <University of Paderborn>
   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify and
   merge the Software, subject to the following conditions:

   1.) The Software is used for non-commercial research and
       education purposes.

   2.) The above copyright notice and this permission notice shall be
       included in all copies or substantial portions of the Software.

   3.) Publication, Distribution, Sublicensing, and/or Selling of
       copies or parts of the Software requires special agreements
       with the University of Paderborn and is in general not permitted.

   4.) Modifications or contributions to the software must be
       published under this license. The University of Paderborn
       is granted the non-exclusive right to publish modifications
       or contributions in future versions of the Software free of charge.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   Persons using the Software are encouraged to notify the
   Department of Communications Engineering at the University of Paderborn
   about bugs. Please reference the Software in your publications
   if it was used for them.


   Author: Oliver Walter

   E-Mail: walter@nt.uni-paderborn.de

   Description: some definitions used in the segmenter and fst

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _DEFINES_HPP_
#define _DEFINES_HPP_

#include <unordered_map>
#include <fst/matcher.h>
#include <fst/compose-filter.h>
#include <fst/lookahead-filter.h>
#include "../NHPYLM/definitions.hpp"

#define EPS_SYMBOL         "<eps>"
#define EPS_SYMBOLID       0
#define PHI_SYMBOL         "<phi>"
#define PHI_SYMBOLID       1
#define UNKBEGIN_SYMBOL    "<unk>"
#define UNKBEGIN_SYMBOLID  2
#define UNKEND_SYMBOL      "</unk>"
#define UNKEND_SYMBOLID    3
#define SENTSTART_SYMBOL   "<s>"
#define SENTSTART_SYMBOLID 4
#define SENTEND_SYMBOL     "</s>"
#define SENTEND_SYMBOLID   5
#define CHARACTERSBEGIN    6

typedef int CharId; // alias for character id
typedef int WordId; // alias for word id

typedef fst::VectorFst<fst::StdArc> StdVectorFst;
typedef fst::VectorFst<fst::LogArc> LogVectorFst;
typedef fst::StateIterator<LogVectorFst>  LogStateIterator;

typedef std::unordered_map<std::string, int> StringToIntMap;

// phi matcher for composition with lexicon and language model transducer
typedef fst::PhiMatcher<fst::SortedMatcher<fst::Fst<fst::LogArc> > > PM;
typedef fst::ArcMapFst<fst::LogArc, fst::StdArc,
                       fst::LogToStdMapper> LogToStdMapFst;

typedef fst::ComposeFst<fst::LogArc> LogComposeFst;

// struct to hold arc information
struct ArcInfo {
  int label;
  float start;
  float end;
  ArcInfo(int label_, float start_, float end_):
    label(label_), start(start_), end(end_) {};
};

enum LatticeFileTypes {CMU_FST, HTK_FST, OPEN_FST, TEXT}; // file types for input lattices
enum InputTypes {INPUT_FST, INPUT_TEXT};                  // input modes: fst or text
enum SymbolWriteModes {NONE, NAMES, NAMESANDIDS};         // modes for symbol output in fst printing

#endif
