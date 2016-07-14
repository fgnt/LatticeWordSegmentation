// ----------------------------------------------------------------------------
/**
   File: NHPYLMFst.cpp
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
#include "NHPYLMFst.hpp"
#include "definitions.hpp"

using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::cerr;

const int NHPYLMFst::kFileVersion = 1;

NHPYLMFst::NHPYLMFst(const NHPYLM &LanguageModel_, int SentEndWordId_, const vector< bool > &ActiveWords_) :
  LanguageModel(LanguageModel_),
  SentEndWordId(SentEndWordId_),
  CHPYLMOrder(LanguageModel_.GetCHPYLMOrder()),
  WHPYLMOrder(LanguageModel_.GetWHPYLMOrder()),
  StartContextId(LanguageModel_.GetContextId(std::vector<int>(WHPYLMOrder - 1, SentEndWordId_))),
  FinalContextId(LanguageModel_.GetFinalContextId()),
  FSTProperties(fst::kOEpsilons | fst::kILabelSorted | fst::kOLabelSorted),
  FSTType("vector"),
  ActiveWords(ActiveWords_),
  FallbackSymbolId(PHI_SYMBOLID),
  Arcs(LanguageModel_.GetFinalContextId() + 1)
{
}

int NHPYLMFst::Start() const
{
  return StartContextId;
}

NHPYLMFst::Weight NHPYLMFst::Final(StateId s) const
{
  if (s == FinalContextId) {
    return Weight::One();
  } else {
    return Weight::Zero();
  }
}

size_t NHPYLMFst::NumArcs(NHPYLMFst::StateId s) const
{
  return Arcs.at(s).size();
}

size_t NHPYLMFst::NumInputEpsilons(NHPYLMFst::StateId s) const
{
  if ((FallbackSymbolId == EPS_SYMBOLID) && (s != 0) && (s != FinalContextId)) {
    return 1;
  } else {
    return 0;
  }
}

size_t NHPYLMFst::NumOutputEpsilons(NHPYLMFst::StateId s) const
{
  if ((s != 0) && (s != FinalContextId)) {
    return 1;
  } else {
    return 0;
  }
}

uint64 NHPYLMFst::Properties(uint64 mask, bool) const
{
  return mask & FSTProperties;
}

const string &NHPYLMFst::Type() const
{
  return FSTType;
}

fst::Fst< fst::LogArc > *NHPYLMFst::Copy(bool) const
{
  return new NHPYLMFst(LanguageModel, SentEndWordId, ActiveWords);
}

const fst::SymbolTable *NHPYLMFst::InputSymbols() const
{
  return NULL;
}

const fst::SymbolTable *NHPYLMFst::OutputSymbols() const
{
  return NULL;
}

void NHPYLMFst::InitStateIterator(fst::StateIteratorData< fst::LogArc > *data) const
{
  data->base = 0;
  data->nstates = FinalContextId + 1;
}

void NHPYLMFst::InitArcIterator(NHPYLMFst::StateId s, fst::ArcIteratorData< fst::LogArc > *data) const
{
  data->base = NULL;
  if (s != FinalContextId) {
    data->arcs = GetArcs(s);
    data->narcs = NumArcs(s);
  } else {
    data->narcs = 0;
    data->arcs = NULL;
  }
  data->ref_count = NULL;
}

const fst::LogArc *NHPYLMFst::GetArcs(StateId s) const
{
  if (Arcs.at(s).size() == 0) {
    std::vector<fst::LogArc> &State = Arcs.at(s);
    ContextToContextTransitions Transitions = LanguageModel.GetTransitions(s, SentEndWordId, ActiveWords);
    int NumTransitions = Transitions.NextContextIds.size();
    for (int TransitionIdx = 0; TransitionIdx < NumTransitions; TransitionIdx++) {
      if (Transitions.Words.at(TransitionIdx) != PHI_SYMBOLID) {
        State.push_back(fst::LogArc(Transitions.Words.at(TransitionIdx), Transitions.Words.at(TransitionIdx), -log(Transitions.Probabilities.at(TransitionIdx)), Transitions.NextContextIds.at(TransitionIdx)));
      } else {
        State.push_back(fst::LogArc(FallbackSymbolId, EPS_SYMBOLID, -log(Transitions.Probabilities.at(TransitionIdx)), Transitions.NextContextIds.at(TransitionIdx)));
      }
    }
    std::sort(State.begin(), State.end(), NHPYLMFst::iLabelSort);
  }
  return Arcs.at(s).data();
}

inline bool NHPYLMFst::iLabelSort(const fst::LogArc &i, const fst::LogArc &j)
{
  return i.ilabel < j.ilabel;
}
