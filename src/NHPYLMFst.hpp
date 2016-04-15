// ----------------------------------------------------------------------------
/**
   File: NHPYLMFst.hpp

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

   Description: nested hierarchical pitman yor language model realized as fst

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _NHPYLMFST_HPP_
#define _NHPYLMFST_HPP_

#include <fst/fst.h>
#include "NHPYLM/NHPYLM.hpp"
#include "definitions.hpp"

/* fst for nested hierachical pitman yor language model */
class NHPYLMFst : public fst::Fst<fst::LogArc> {
  typedef fst::LogArc::StateId StateId; // state ids
  typedef fst::LogArc::Weight Weight;   // weights

  static const int kFileVersion;

  const NHPYLM &LanguageModel;    // reference to nested hierarchical pitman yor language model
  const int SentEndWordId;        // sentence and word id
  const unsigned int CHPYLMOrder; // order of character hierarchical pitman yor language model
  const unsigned int WHPYLMOrder; // order of word hierarchical pitman yor language model
  const int StartContextId;       // id of start context
  const int FinalContextId;       // id of final context
  const uint64 FSTProperties;     // properties of fst
  const std::string FSTType;      // type of fst
  const std::vector<bool> ActiveWords; // vector indicating active words
  const int FallbackSymbolId;     // the fallback symbol id used for input symbols (either EPS or PHI)

  mutable std::vector<std::vector<fst::LogArc> > Arcs; // vector containing arcs of all states

  /* internal functions */
  // get transitions from language model and build arcs
  const fst::LogArc *GetArcs(
    StateId s
  ) const;

  // used to sort the arcs according to the imput label
  static inline bool iLabelSort(
    const fst::LogArc &i,
    const fst::LogArc &j
  );
public:
  /* constructor and destructor */
  // setup the fst for the nested hierarchical pitman yor language model
  NHPYLMFst(
    const NHPYLM &LanguageModel_,
    int SentEndWordId_,
    const std::vector<bool> &ActiveWords_
  );


  /* interface */
  // Initial state
  StateId Start() const;

  // State's final weight
  Weight Final(
    StateId s
  ) const;
  
  // State's arc count
  size_t NumArcs(
    StateId s
  ) const;
  
  // State's input epsilon count
  size_t NumInputEpsilons(
    NHPYLMFst::StateId s
  ) const;
  
  // State's output epsilon count
  size_t NumOutputEpsilons(
    StateId s
  ) const;
  
  // Property bits
  uint64 Properties(
    uint64 mask, bool
  ) const;
  // Fst type name
  const string &Type() const;

  // Get a copy of this Fst
  Fst<fst::LogArc> *Copy(
    bool = false
  ) const;
  
  // Return input label symbol table; return NULL if not specified
  const fst::SymbolTable *InputSymbols() const;

  // Return output label symbol table; return NULL if not specified
  const fst::SymbolTable *OutputSymbols() const;

  // For generic state iterator construction
  void InitStateIterator(
    fst::StateIteratorData<fst::LogArc> *data
  ) const;
  
  // For generic arc iterator construction
  void InitArcIterator(
    StateId s, fst::ArcIteratorData<fst::LogArc> *data
  ) const;
};

#endif