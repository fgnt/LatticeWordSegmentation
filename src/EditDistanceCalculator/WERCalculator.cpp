// ----------------------------------------------------------------------------
/**
   File: WERCalculator.cpp
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
#include "WERCalculator.hpp"
#include "SampleLib.hpp"

WERCalculator::WERCalculator(const vector< vector< int > > &InputSentences_, const vector< fst::VectorFst< fst::LogArc > > &ReferenceFsts_, const Dictionary &dict_, int WHPYLMContextLength, unsigned int NumThreads_, const std::vector<std::string> &FileNames_, const std::string &Prefix_, bool OutputEditOperations_) :
  EditDistanceCalculator(NumThreads_, FileNames_, Prefix_, OutputEditOperations_),
  dict(dict_),
  LexiconCorrNFoundNRef(3, 0),
  TotalNumReferenceWords(0)
{
//   std::cout << "Setting LexiconCorrNFoundNRef[1]" << std::endl;
  LexiconCorrNFoundNRef[1] = dict.GetId2Word().size() - 1;
//   std::cout << "Triming input sentences" << std::endl;
  TrimInputSentences(InputSentences_, WHPYLMContextLength);
//   std::cout << "Parsing reference sentences" << std::endl;
  ParseReferenceFsts(ReferenceFsts_);
//   std::cout << "Setting input and reference sentences" << std::endl;
  SetInputAndReferenceSentences(InputSentences, ReferenceSentences, InputAndReferenceIds, dict.GetId2CharacterSequenceVector());
//   std::cout << "Calculating LexiconCorrNFoundNRef" << std::endl;
  CalcLexiconCorrNFoundNRef();
}

void WERCalculator::TrimInputSentences(const vector< vector< int > > &InputSentences_, int WHPYLMContextLength)
{
  int NumInputSentences = InputSentences_.size();
  InputSentences.resize(NumInputSentences);
  for (int IdxInputSentence = 0; IdxInputSentence < NumInputSentences; ++IdxInputSentence) {
    InputSentences.at(IdxInputSentence).assign(InputSentences_.at(IdxInputSentence).begin() + WHPYLMContextLength, InputSentences_.at(IdxInputSentence).end() - 1);
  }
}

void WERCalculator::ParseReferenceFsts(const vector< fst::VectorFst< fst::LogArc > > &ReferenceFsts)
{
  int NumReferenceFsts = ReferenceFsts.size();
  ReferenceSentences.resize(NumReferenceFsts);
  for (int IdxReferenceFst = 0; IdxReferenceFst < NumReferenceFsts; ++IdxReferenceFst) {
//     std::cout << "Parsing reference FST " << IdxReferenceFst << std::flush;
    SampleLib::ParseSampleAndAddCharacterIdSequenceToDictionary(ReferenceFsts.at(IdxReferenceFst), &dict, &ReferenceSentences.at(IdxReferenceFst));
//     std::cout << " Done!" << std::endl << std::flush;
    ReferenceSentences.at(IdxReferenceFst).pop_back();
    TotalNumReferenceWords += ReferenceSentences.at(IdxReferenceFst).size();
  }
  InputAndReferenceIds.resize(dict.GetMaxNumWords() - dict.GetWordsBegin());
  std::iota(InputAndReferenceIds.begin(), InputAndReferenceIds.end(), dict.GetWordsBegin());
}

void WERCalculator::CalcLexiconCorrNFoundNRef()
{
  std::vector<int> ConcatenatedReferenceSentences;
  ConcatenatedReferenceSentences.reserve(TotalNumReferenceWords);
  for (const std::vector<int> &ReferenceSentence : ReferenceSentences) {
    ConcatenatedReferenceSentences.insert(ConcatenatedReferenceSentences.end(), ReferenceSentence.begin(), ReferenceSentence.end());
  }

  std::sort(ConcatenatedReferenceSentences.begin(), ConcatenatedReferenceSentences.end());
  std::vector<int>::iterator LastUniqueElement = std::unique(ConcatenatedReferenceSentences.begin(), ConcatenatedReferenceSentences.end());
  LexiconCorrNFoundNRef[2] = std::distance(ConcatenatedReferenceSentences.begin(), LastUniqueElement);
  LexiconCorrNFoundNRef[0] = LexiconCorrNFoundNRef[1] - (dict.GetId2Word().size() - 1 - LexiconCorrNFoundNRef[2]);
}


const vector< int > &WERCalculator::GetLexiconCorrNFoundNRef() const
{
  return LexiconCorrNFoundNRef;
}