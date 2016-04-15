// ----------------------------------------------------------------------------
/**
   File: EditDistanceCalculator.hpp

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

   Description: evaluation module to calculate edit distance between two strings

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _EDITDISTANCECALCULATOR_HPP_
#define _EDITDISTANCECALCULATOR_HPP_

#include <fst/vector-fst.h>

/* class to calculate edit distance */
class EditDistanceCalculator {
  unsigned int NumSentences;
  unsigned int NumThreads;
  std::vector<fst::VectorFst<fst::StdArc> > InputFsts;
  std::vector<std::vector<int> > InputSentences;
  std::vector<fst::VectorFst<fst::StdArc> > ReferenceFsts;
  std::vector<std::vector<int> > ReferenceSentences;

  std::vector<int> InputAndOutputIds;
  int SubstitutionId;
  int InsertionId;
  int DeletionId;
  fst::VectorFst<fst::StdArc> LeftFactor;
  fst::VectorFst<fst::StdArc> RightFactor;
  std::vector<fst::VectorFst<fst::StdArc> > ResultFsts;

  std::vector<std::string> Id2CharacterSequenceVector;

  std::vector<int> InsDelSubCorrNFoundNRef;
  std::vector<std::string> FileNames;
  std::string Prefix;
  bool OutputEditOperations;


  /* internal functions */
  void BuildLeftAndRightFactors();

  void CalculateEditDistance();

  static inline void CalculateEditDistanceIdxRange(
    const vector< fst::VectorFst< fst::StdArc > > *InputFsts,
    const vector< fst::VectorFst< fst::StdArc > > *ReferenceFsts,
    vector< fst::VectorFst< fst::StdArc > > *ResultFsts,
    const fst::VectorFst< fst::StdArc > *LeftFactor,
    const fst::VectorFst< fst::StdArc > *RightFactor,
    vector< int > *InsDelSubCorrNFoundNRef,
    unsigned int StartIdx,
    unsigned int EndIdx,
    const vector< string > *Id2CharacterSequenceVector,
    const vector< string > *Filenames,
    const string *Prefix,
    bool OutputEditOperations
  );
  
  static inline void BuildFstFromIdSequence(
    const vector< int > &IdSequence,
    fst::VectorFst< fst::StdArc > *Fst
  );
  
  static inline void CalculateEditDistanceSingleIdx(
    const fst::VectorFst< fst::StdArc > &InputFst,
    const fst::VectorFst< fst::StdArc > &ReferenceFst,
    fst::VectorFst< fst::StdArc > *ResultFst,
    const fst::VectorFst< fst::StdArc > &LeftFactor,
    const fst::VectorFst< fst::StdArc > &RightFactor,
    vector< int > *InsDelSubCorrNFoundNRef,
    const vector< string > *Id2CharacterSequenceVector,
    const string *FileName,
    const string *Prefix,
    bool OutputEditOperations);

protected:
  /* interface for derived classes */
  void SetInputAndReferenceSentences(
    const vector< vector< int > > &InputSentences_,
    const vector< vector< int > > &ReferenceSentences_,
    const std::vector<int> &InputAndOutputIds_,
    const std::vector<std::string> &Id2CharacterSequenceVector_
  );
  
  void SetInputFstsAndReferenceSentences(
    const vector< fst::VectorFst< fst::StdArc > > &InputFsts_,
    const vector< vector< int > > &ReferenceSentence,
    const vector< int > &InputAndOutputIds_,
    const vector< string > &Id2CharacterSequenceVector_
  );

public:
  /* constructor */
  EditDistanceCalculator(
    unsigned int NumThreads_,
    const vector< string > &FileNames_,
    const string &Prefix_,
    bool OutputEditOperations_
  );
  
  /* interface */
  const std::vector<int> &GetInsDelSubCorrNFoundNRef() const;
};

#endif