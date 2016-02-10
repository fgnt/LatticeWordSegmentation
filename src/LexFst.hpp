// ----------------------------------------------------------------------------
/**
   File: LexFst.hpp

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

   Description: lexicon realized as fst

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _LEXFST_HPP_
#define _LEXFST_HPP_

#include <fst/vector-fst.h>
#include "definitions.hpp"

/* class for lexicon fst */
class LexFst : public fst::VectorFst<fst::LogArc> {
  typedef fst::LogArc::StateId StateId; // alias for state id

  const bool Debug;                       // debuging option
  const std::vector<std::string> Symbols; // symbols for debug output
  const int CharactersBegin;              // first character id
  const int CharactersEnd;                // number of characters
  StateId HomeState;                      // home state of fst
  StateId UnkState;                       // state for unknown sequences
  std::vector<double> CharacterSequenceProbabilityScale; // weights for character sequence probability scaling

  /* internal functions */
  void addArcSorted(StateId s, const fst::LogArc &arc);                      // function to add an arc in sorted order
  static inline bool iLabelSort(const fst::LogArc &i, const fst::LogArc &j); // comparison function for arc sorting
  int getLastHistState(StateId s) const;                                     // returns last state of history sequence
  void rmArcWithId(StateId s, CharId id);                                    // finds and removes an arc with given id from given state

public:
  /* constructor */
  LexFst(bool pDebug, const vector< string > &pSymbols, int pCharactersBegin, const std::vector<double> &CharacterSequenceProbabilityScale); // construct lexicon fst

  /* interface */
  void BuildLexiconTansducer(const Word2IdHashmap &Word2Id);                            // build lexicon transducer from Word2Id map
  void initializeArcs();                                                                // initialize arcs
  void addWord(std::vector<int>::const_iterator WordBegin, int WordLength, int WordId); // add word to lexicon fst
  void rmWord(std::vector<int>::const_iterator WordBegin, int WordLength);              // remove word from lexicon fst
  uint64 Properties(uint64 mask, bool) const;                                           // Property bits
};

#endif