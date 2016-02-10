// ----------------------------------------------------------------------------
/**
   File: SampleLib.cpp
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
   

   Note: License for functions SampleWeights, SampGen and
   ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst:
   Copyright 2010, Graham Neubig

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
   
   SampleWeights, SampGen and
   ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst: Modified
   according to own needs (by Jahn Heymann (2013) and Oliver Walter (2014))
*/
// ----------------------------------------------------------------------------
#include "fst/compose.h"
#include <fst/shortest-path.h>
#include "SampleLib.hpp"
#include "FileReader.hpp"
#include <beam-search.h>
// #include "DebugLib.hpp"

std::mutex SampleLib::mtx;

int SampleLib::AddCharacterIdSequenceToDictionaryAndLexFST(const vector< int > &Characters, Dictionary *LanguageModel, LexFst *LexiconTransducer)
{
  WordIdAddedPair wIdAddedPair = LanguageModel->AddCharacterIdSequenceToDictionary(Characters.begin(), Characters.size());
  if (wIdAddedPair.second && (LexiconTransducer != NULL)) {
    LexiconTransducer->addWord(Characters.begin(), Characters.size(), wIdAddedPair.first);
  }
  return wIdAddedPair.first;
}

void SampleLib::RemoveWordsFromDictionaryLexFSTAndLM(const const_witerator &Word, int NumWords, NHPYLM *LanguageModel, LexFst *LexiconTransducer, int SentEndWordId)
{
//   std::cout << "Removing: ";
//   DebugLib::PrintSentence(Word, NumWords, LanguageModel->GetId2CharacterSequenceVector());
  for (int IdxWord = 0; IdxWord < NumWords; ++IdxWord) {
    RemoveWordFromDictionaryLexFSTAndLM(Word + IdxWord, LanguageModel, LexiconTransducer, SentEndWordId);
  }
}

void SampleLib::RemoveWordFromDictionaryLexFSTAndLM(const const_witerator &Word, NHPYLM *LanguageModel, LexFst *LexiconTransducer, int SentEndWordId)
{
//   std::cout << "Removing: " << *Word << " from LM" << std::endl;
  if (LanguageModel->RemoveWordFromLm(Word) && (*Word != SentEndWordId)) {
//     std::cout << "Removing: " << *Word << " from Lexicon and Transducer" << std::endl;
    WordBeginLengthPair WordBeginLengh = LanguageModel->GetWordBeginLength(*Word);
    LexiconTransducer->rmWord(WordBeginLengh.first, WordBeginLengh.second);
    LanguageModel->RemoveWordFromDictionary(*Word);
  }
}

void SampleLib::ComposeAndSampleFromInputLexiconAndLM(const fst::Fst< fst::LogArc > *InputFst, const fst::Fst< fst::LogArc > *LexiconTransducer, const NHPYLM *LanguageModel, int SentEndWordId, fst::VectorFst< fst::LogArc > *SampledFst, std::vector< LatticeWordSegmentationTimer::SimpleTimer > *tInSample, int beamWidth, bool UseViterby)
{
//   std::cout << "Composing and Sampling: " << std::endl;

  // compose input with lexicon transducer
  (*tInSample)[0].SetStart();
  PM *PM11 = new PM(*InputFst, fst::MATCH_NONE);
  mtx.lock();
  PM *PM21 = new PM(*LexiconTransducer, fst::MATCH_INPUT, PHI_SYMBOLID, false);
  mtx.unlock();
  fst::ComposeFstOptions<fst::LogArc, PM> copts1(fst::CacheOptions(), PM11, PM21);
  fst::ComposeFst<fst::LogArc> Input_Unk_Lex(*InputFst, *LexiconTransducer, copts1);
//   fst::ArcSortFst<fst::LogArc, fst::OLabelCompare<fst::LogArc> > Input_Unk_Lex_OSort(Input_Unk_Lex, fst::OLabelCompare<fst::LogArc>());
//   OLookAhead::OLAM *OLAM12 = new OLookAhead::OLAM(Input_Unk_Lex_OSort, fst::MATCH_OUTPUT);
  (*tInSample)[0].AddTimeSinceStartToDuration();

  (*tInSample)[1].SetStart();
//   std::cout << std::endl << "Label2Index.size() create: " << OLAM12->GetData()->Label2Index()->size() << std::endl;
//   for (typename unordered_map<int, int>::const_iterator it = OLAM12->GetData()->Label2Index()->begin(); it != OLAM12->GetData()->Label2Index()->end(); ++it) {
//     std::cout << "r(" << it->first << ")=" << it->second << std::endl;
//   }
//   OLookAhead::Reachable RD(OLAM12->GetData());
  NHPYLMFst LanguageModelFST(*LanguageModel, SentEndWordId, GetActiveWordIdsInFst(Input_Unk_Lex, LanguageModel->GetMaxNumWords()));
//   NHPYLMFst LanguageModelFST(*LanguageModel, SentEndWordId, GetActiveWordIdsInFst(Input_Unk_Lex, LanguageModel->GetMaxNumWords()), &RD);
//   PM *PM22 = new PM(LanguageModelFST, fst::MATCH_INPUT, RD.Relabel(PHI_SYMBOLID), false);
  (*tInSample)[1].AddTimeSinceStartToDuration();

  // compose with language model
  (*tInSample)[2].SetStart();
  PM *PM12 = new PM(Input_Unk_Lex, fst::MATCH_NONE);
  PM *PM22 = new PM(LanguageModelFST, fst::MATCH_INPUT, PHI_SYMBOLID, false);
  fst::ComposeFstOptions<fst::LogArc, PM> copts2(fst::CacheOptions(), PM12, PM22);
  fst::ComposeFst<fst::LogArc> Input_Unk_Lex_LM(Input_Unk_Lex, LanguageModelFST, copts2);
  fst::VectorFst<fst::LogArc> *beamSearchFst = new fst::VectorFst<fst::LogArc>();
  if (beamWidth > 0) {
    fst::BeamTrim(Input_Unk_Lex_LM, beamSearchFst, beamWidth);
  }

//  int arcCnt = 0;
//  for (fst::StateIterator<fst::ComposeFst<fst::LogArc> > siter(Input_Unk_Lex_LM); !siter.Done(); siter.Next()) {
//    arcCnt += Input_Unk_Lex_LM.NumArcs(siter.Value());
//  }
//  std::cout << "No beam: " << arcCnt << " Arcs" << std::endl;
//  arcCnt = 0;
//  for (fst::StateIterator<fst::VectorFst<fst::LogArc> > siter(*beamSearchFst); !siter.Done(); siter.Next()) {
//    arcCnt += beamSearchFst->NumArcs(siter.Value());
//  }
//  std::cout << "Beam: " << arcCnt << " Arcs" << std::endl;

//   OLookAhead::LACF *LACF = new OLookAhead::LACF(Input_Unk_Lex_OSort, LanguageModelFST, OLAM12, PM22);
//   fst::ComposeFstImplOptions<OLookAhead::OLAM, PM, OLookAhead::LACF> copts2(fst::CacheOptions(), OLAM12, PM22, LACF);
//   fst::ComposeFst<fst::LogArc> Input_Unk_Lex_LM(Input_Unk_Lex_OSort, LanguageModelFST, copts2);
  (*tInSample)[2].AddTimeSinceStartToDuration();

  // sample segmentation
  (*tInSample)[3].SetStart();
//   std::cout << std::endl << "Label2Index.size() before: " << LACF->GetMatcher1()->GetData()->Label2Index()->size() << std::endl;
//   FileReader::PrintFST("lattice_debug/in.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(*InputFst), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/lex.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(*LexiconTransducer), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/in_lex.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(Input_Unk_Lex_OSort), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/in_lex.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(Input_Unk_Lex), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/lm.orig.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(OrigLanguageModelFST), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/lm.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(LanguageModelFST), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/in_lex_lm.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(Input_Unk_Lex_LM), true, NAMESANDIDS);
//   std::cout << std::endl << "Label2Index.size() after: " << LACF->GetMatcher1()->GetData()->Label2Index()->size() << std::endl;
//   for (typename unordered_map<int, int>::const_iterator it = OLAM12->GetData()->Label2Index()->begin(); it != OLAM12->GetData()->Label2Index()->end(); ++it) {
//     std::cout << "r(" << it->first << ")=" << it->second << std::endl;
//   }
  if (!UseViterby) {
    if (beamWidth > 0) {
      SampGen(*beamSearchFst, SampledFst, 1);
    } else {
      SampGen(fst::VectorFst<fst::LogArc>(Input_Unk_Lex_LM), SampledFst, 1);
    }
  } else {
    fst::VectorFst<fst::StdArc> iStdFst;
    if (beamWidth > 0) {
      fst::Cast(*beamSearchFst, &iStdFst);
    } else {
      fst::Cast(fst::VectorFst<fst::LogArc>(Input_Unk_Lex_LM), &iStdFst);
    }
    fst::VectorFst<fst::StdArc> oStdFst;
//     std::cout << "Start Shortest Path" << std::endl << std::flush;
    fst::ShortestPath(iStdFst, &oStdFst);
//     std::cout << "End Shortest Path" << std::endl << std::flush;
    fst::Cast(oStdFst, SampledFst);
//     std::cout << "End Cast" << std::endl << std::flush;
  }
  (*tInSample)[3].AddTimeSinceStartToDuration();
  delete beamSearchFst;
//   std::cout << "Sampling done!" << std::endl;
}

vector< bool > SampleLib::GetActiveWordIdsInFst(const fst::Fst< fst::LogArc > &SegmentFST, int MaxNumWords)
{
//   int NumActiveWords = 0;
//   int NumKnownWords = LanguageModel.GetId2Word().size();
  std::vector<bool> ActiveWords(MaxNumWords, false);
  for (fst::StateIterator<fst::Fst<fst::LogArc> > siter(SegmentFST); !siter.Done(); siter.Next()) {
    for (fst::ArcIterator<fst::Fst<fst::LogArc> > aiter(SegmentFST, siter.Value()); !aiter.Done(); aiter.Next()) {
//       NumActiveWords += !ActiveWords[aiter.Value().olabel];
      ActiveWords[aiter.Value().olabel] = true;
    }
  }
//   std::cout << NumActiveWords << " of " << NumKnownWords << " words in fst!" << std::endl;
  return ActiveWords;
}

// Copyright 2010, Graham Neubig, modified by Jahn Heymann (2013) and Oliver Walter (2014) //
unsigned SampleLib::SampleWeights(vector< float > *ws)
{

  if (ws->size() == 0) {
    throw std::runtime_error("No final states found during sampling");
  } else if (ws->size() == 1) {
    return 0;
  }

  float minWeight = std::numeric_limits<float>::infinity(), weightTotal = 0;
  unsigned i;
  for (i = 0; i < ws->size(); i++) {
    if ((*ws)[i] < minWeight) {
      minWeight = (*ws)[i];
    }
  }

  for (i = 0; i < ws->size(); i++) {
    float &f = (*ws)[i];
    f = exp(minWeight - f);
    weightTotal += f;
  }

  //cout << "Total weight=" << weightTotal;
  weightTotal *= rand() / static_cast<double>(RAND_MAX);
  //cout << ", random weight=" << weightTotal << " (basis " << minWeight << ")"<<endl;
  for (i = 0; i < ws->size(); i++) {
    weightTotal -= (*ws)[i];
    //cout << " after weight " << i << ", " << weightTotal << endl;
    if (weightTotal <= 0) {
      break;
    }
  }

  if (i == ws->size()) {
    cout << "WARNING: Sampling failed, probability mass left at end of cycle";
    i--;
  }
  return i;
}

// Copyright 2010, Graham Neubig, modified by Jahn Heymann (2013) and Oliver Walter (2014) //
void SampleLib::SampGen(const fst::Fst< fst::LogArc > &ifst, fst::MutableFst< fst::LogArc > *ofst, unsigned int nbest)
{
  typedef fst::Fst<fst::LogArc> F;
  typedef typename F::Weight W;
  typedef typename fst::LogArc::StateId S;

  int Debug = 0;

  // sanity check
  if (ifst.Final(ifst.Start()) != std::numeric_limits<float>::infinity()) {
    throw std::runtime_error("Sampling FSTs where start states are final is not supported yet");
  }

  // the number of remaining incoming arcs, and total weights of each state
  std::vector< int > incomingArcs;
  std::vector< vector< fst::LogArc > > backArcs;
  std::vector< W > stateWeights;
  unsigned i, statesFinished = 0;

  // intialize the data values
  for (fst::StateIterator<fst::Fst<fst::LogArc> > siter(ifst); !siter.Done(); siter.Next()) {
    S s = siter.Value();
    // cout << "state: " << s << endl;
    for (fst::ArcIterator< F > aiter(ifst, s); !aiter.Done(); aiter.Next()) {
      const fst::LogArc &a = aiter.Value();
      // cout << " -> " << a.nextstate << endl;
      while (static_cast<unsigned>(a.nextstate) >= incomingArcs.size()) {
        incomingArcs.push_back(0);
        stateWeights.push_back(W::Zero());
        backArcs.push_back(std::vector<fst::LogArc>());
      }
      incomingArcs[a.nextstate]++;
      backArcs[a.nextstate].push_back(fst::LogArc(a.ilabel, a.olabel, a.weight, s));
    }
  }
  stateWeights[ifst.Start()] = W::One();
  incomingArcs[ifst.Start()] = 0;

  // calculate the number of arcs incoming to each state
  vector< S > stateQueue(1, ifst.Start());
  while (stateQueue.size() > 0) {
    unsigned s = stateQueue[stateQueue.size() - 1];
    stateQueue.pop_back();
    //cout << "state: " << s;
    for (fst::ArcIterator< F > aiter(ifst, s); !aiter.Done(); aiter.Next()) {
      const fst::LogArc &a = aiter.Value();
      if (std::isnan(a.weight.Value())) {
        cout << "stateWeights[" << a.nextstate << "]: (" << stateWeights[a.nextstate] << "+(" << stateWeights[s] << "*" << a.weight << "<-" << s << "[" << a.ilabel << "/" << a.olabel << "]" << "))" << endl;
      }
      stateWeights[a.nextstate] = fst::Plus(stateWeights[a.nextstate], fst::Times(stateWeights[s], a.weight));
      //cout << " -> " << stateWeights[a.nextstate] << " [" << a.olabel << "]" << endl;
      if (--incomingArcs[a.nextstate] == 0) {
        stateQueue.push_back(a.nextstate);
        //cout << " -> " << a.nextstate;
      }
    }
    //cout << endl;
    statesFinished++;
  }
  //cout << "statesFinished: " << statesFinished << " - incomingArcs.size(): " << incomingArcs.size() << endl;
  if (statesFinished != incomingArcs.size()) {
    throw std::runtime_error("Sampling cannot be performed on cyclic FSTs");
  }

  fflush(stdout);
  // sample the states backwards from the final state
  ofst->DeleteStates();
  ofst->AddState();
  ofst->SetStart(0);

  for (unsigned n = 0; n < nbest; n++) {

    // find the final states and sample a final state
    vector< float > stateCandWeights;
    vector< S > stateCandIds;
    for (fst::StateIterator<fst::Fst<fst::LogArc> > siter(ifst); !siter.Done(); siter.Next()) {
      S s = siter.Value();
      float w = fst::Times(ifst.Final(s), stateWeights[s]).Value();
      if (w != std::numeric_limits<float>::infinity()) {
        if (Debug) {
          cout << "Final state " << s << "," << w << endl;
        }
        stateCandWeights.push_back(w);
        stateCandIds.push_back(s);
      }
    }
    S currState = stateCandIds[SampleWeights(&stateCandWeights)];

    // add the final state
    S outState = (ifst.Start() != currState ? ofst->AddState() : 0);
    ofst->SetFinal(outState, ifst.Final(currState));

    // sample the values in order
    while (outState != 0) {
      const vector<fst::LogArc> &arcs = backArcs[currState];
      vector<float> arcWeights(arcs.size(), 0);
      for (i = 0; i < arcs.size(); i++) {
        arcWeights[i] = fst::Times(arcs[i].weight, stateWeights[arcs[i].nextstate]).Value();
      }
      const fst::LogArc &myArc = arcs[SampleWeights(&arcWeights)];
      S nextOutState = (myArc.nextstate != ifst.Start() ? ofst->AddState() : 0);
      // cout << "Adding arc " << nextOutState << "--"<<myArc.ilabel<<"/"<<myArc.olabel<<":"<<myArc.weight<<"-->"<<outState<<endl;
      ofst->AddArc(nextOutState, fst::LogArc(myArc.ilabel, myArc.olabel, myArc.weight, outState));
      outState = nextOutState;
      currState = myArc.nextstate;
    }
  }
}

void SampleLib::ParseSampleAndAddCharacterIdSequenceToDictionaryLexFstAndLM(const fst::Fst< fst::LogArc > &Sample, int SentEndWordId, NHPYLM *LanguageModel, LexFst *LexiconTransducer, vector< WordId > *ret)
{
//   std::cout << "Parsing: " << std::endl;
  ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst(Sample, LanguageModel, LexiconTransducer, ret);
  int WHPYLMContextLenght = LanguageModel->GetWHPYLMOrder() - 1;
  ret->insert(ret->begin(), WHPYLMContextLenght, SentEndWordId);

//   std::cout << "Adding:   ";
//   DebugLib::PrintSentence((*ret).begin(), (*ret).size(), LanguageModel->GetId2CharacterSequenceVector());
  LanguageModel->AddWordSequenceToLm(*ret);
}

// Copyright 2010, Graham Neubig, modified by Jahn Heymann (2013) and Oliver Walter (2014) //
void SampleLib::ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst(const fst::Fst< fst::LogArc > &Sample, Dictionary *Dict, LexFst *LexiconTransducer, vector< WordId > *ret)
{
  ret->clear();
  std::vector<CharId> charBuf;
  int sid = Sample.Start();
  int WordsBegin = Dict->GetWordsBegin();
  // continue until there are no more left
//   std::cout << "Parsing:   " << std::endl << std::flush;
  while (true) {
    fst::ArcIterator<fst::Fst<fst::LogArc> > ai(Sample, sid);
    if (ai.Done()) {
      break;
    }
    const fst::LogArc &arc = ai.Value();
    // add known words
    if (arc.olabel >= WordsBegin) {
      if (charBuf.size() > 0) {
        throw std::runtime_error("Word with non-empty buffer (/unk required)");
      }
      WordId wid = arc.olabel;
      ret->push_back(wid);
//       std::cout << "[Existing word: " << ret->back() << "] ";
    }
    // handle the end of unknown word symbol
    else if (arc.olabel == UNKEND_SYMBOLID) {
      if (charBuf.size() == 0) {
        throw std::runtime_error("End of word symbol with empty buffer");
      }
      ret->push_back(SampleLib::AddCharacterIdSequenceToDictionaryAndLexFST(charBuf, Dict, LexiconTransducer));
      charBuf.clear();
//       std::cout << "[New word: " << ret->back() << "] ";
    }
    // handle unknown word characters
    else if (arc.olabel > UNKEND_SYMBOLID) {
      charBuf.push_back(arc.olabel);
//       std::cout << charBuf.back() << " ";
    }
    sid = arc.nextstate;
  }
  // clean up the remaining buffer
  if (charBuf.size() > 0) {
    ret->push_back(SampleLib::AddCharacterIdSequenceToDictionaryAndLexFST(charBuf, Dict, LexiconTransducer));
  }
//   std::cout << std::endl;
}

void SampleLib::ParseSampleAndAddCharacterIdSequenceToDictionary(const fst::Fst< fst::LogArc > &Sample, Dictionary *Dict, std::vector<int> *ret)
{
  ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst(Sample, Dict, NULL, ret);
}

fst::VectorFst<fst::StdArc> SampleLib::logArcToStdArc(fst::Fst<fst::LogArc> const &iFst)
{
  fst::VectorFst<fst::StdArc> ret;
  for (fst::StateIterator< fst::Fst<fst::LogArc> > siter(iFst); !siter.Done(); siter.Next()) {
    ret.AddState();
  }
  for (fst::StateIterator< fst::Fst<fst::LogArc> > siter(iFst); !siter.Done(); siter.Next()) {
    int s = siter.Value();
    if (iFst.Final(s) != fst::LogArc::Weight::Zero()) {
      ret.SetFinal(s, 0);
    }
    //cout << s << " ";
    for (fst::ArcIterator< fst::Fst<fst::LogArc> > aiter(iFst, s); !aiter.Done(); aiter.Next()) {
      fst::LogArc arc = aiter.Value();
      ret.AddArc(s, fst::StdArc(arc.ilabel, arc.olabel, arc.weight.Value(), arc.nextstate));
    }
  }
  ret.SetStart(iFst.Start());
  return ret;
}
