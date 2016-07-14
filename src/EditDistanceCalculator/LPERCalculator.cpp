// ----------------------------------------------------------------------------
/**
   File: LPERCalculator.cpp
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
*/
// ----------------------------------------------------------------------------
#include <numeric>
#include <fst/arc-map.h>
#include <fst/rmepsilon.h>
#include <CustomArcMappers.hpp>
#include "LPERCalculator.hpp"


using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::cerr;

LPERCalculator::LPERCalculator(const vector< fst::VectorFst< fst::LogArc > > &InputFsts_, const vector< fst::VectorFst< fst::LogArc > > &ReferenceFsts, const vector< string > &Id2CharacterSequenceVector, unsigned int NumThreads_, const std::vector<std::string> &FileNames_, const std::string &Prefix_, bool OutputEditOperations_, const std::vector<ArcInfo> &InputArcInfos_) :
  EditDistanceCalculator(NumThreads_, FileNames_, Prefix_, OutputEditOperations_)
{
//   FileReader::PrintFST("lattice_debug/BeforeRemove.fst", Id2CharacterSequenceVector, InputFsts_[0], false, NONE);
  RemoveWeightAndConvertToStdArc(InputFsts_, &InputFsts, InputArcInfos_);
//   fst::ArcMapFst<fst::StdArc, fst::LogArc, fst::StdToLogMapper > StdToLogFst(InputFsts[0], fst::StdToLogMapper());
//   FileReader::PrintFST("lattice_debug/AfterRemove.fst", Id2CharacterSequenceVector, fst::VectorFst<fst::LogArc>(StdToLogFst), false, NONE);
  ParseFsts(ReferenceFsts, &ReferenceSentences);
  InputAndReferenceIds.resize(Id2CharacterSequenceVector.size() - CHARACTERSBEGIN);
  std::iota(InputAndReferenceIds.begin(), InputAndReferenceIds.end(), CHARACTERSBEGIN);

  SetInputFstsAndReferenceSentences(InputFsts, ReferenceSentences, InputAndReferenceIds, Id2CharacterSequenceVector);
}


void LPERCalculator::ParseFsts(const vector< fst::VectorFst< fst::LogArc > > &Fsts, vector< vector< int > > *Sentences)
{
  Sentences->clear();
  Sentences->resize(Fsts.size());
  int IdxSentence = 0;
  for (const fst::VectorFst< fst::LogArc > &Fst : Fsts) {
    Sentences->at(IdxSentence).clear();
    int sid = Fst.Start();

    // continue until there are no more left
    while (true) {
      fst::ArcIterator<fst::Fst<fst::LogArc> > ai(Fst, sid);
      if (ai.Done()) {
        break;
      }
      const fst::LogArc &arc = ai.Value();
      if (arc.ilabel >= CHARACTERSBEGIN) {
        Sentences->at(IdxSentence).push_back(arc.ilabel);
      }
      sid = arc.nextstate;
    }
    IdxSentence++;
  }
}


void LPERCalculator::RemoveWeightAndConvertToStdArc(const vector< fst::VectorFst< fst::LogArc > > &LogFsts, vector< fst::VectorFst< fst::StdArc > > *StdFsts, const std::vector<ArcInfo> &InputArcInfos)
{
  StdFsts->resize(LogFsts.size());
  for (unsigned int IdxFst = 0; IdxFst < LogFsts.size(); ++IdxFst) {
    fst::ArcMapFst<fst::LogArc, fst::LogArc, fst::RestoreIlabelMapper> LogArcMapFst(LogFsts.at(IdxFst), fst::RestoreIlabelMapper(InputArcInfos));
    fst::RmEpsilonFst<fst::LogArc> LogArcRmEpsilonFst(LogArcMapFst);
    fst::ArcMap(LogArcRmEpsilonFst, &StdFsts->at(IdxFst), fst::RmWeightMapper<fst::LogArc, fst::StdArc>());
  }
}

