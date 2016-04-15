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


   SampleWeights and SampGen: Modified according to own needs
   (by Jahn Heymann (2013) and Oliver Walter (2014))
*/
// ----------------------------------------------------------------------------
#include <iostream>
#include "fst/compose.h"
#include <fst/shortest-path.h>
#include "SampleLib.hpp"
#include <beam-search.h>
// #include "DebugLib.hpp"

std::mutex SampleLib::mtx;

void SampleLib::ComposeAndSampleFromInputLexiconAndLM(
  const fst::Fst< fst::LogArc > *InputFst,
  const fst::Fst< fst::LogArc > *LexiconTransducer,
  const NHPYLM *LanguageModel,
  int SentEndWordId,
  fst::VectorFst< fst::LogArc > *SampledFst,
  std::vector< LatticeWordSegmentationTimer::SimpleTimer > *tInSample,
  int beamWidth, bool UseViterby)
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
  (*tInSample)[0].AddTimeSinceStartToDuration();

  // instantiate language model fst
  (*tInSample)[1].SetStart();
  NHPYLMFst LanguageModelFST(*LanguageModel, SentEndWordId, GetActiveWordIdsInFst(Input_Unk_Lex, LanguageModel->GetMaxNumWords()));
  (*tInSample)[1].AddTimeSinceStartToDuration();

  // compose with language model
  (*tInSample)[2].SetStart();
  PM *PM12 = new PM(Input_Unk_Lex, fst::MATCH_NONE);
  PM *PM22 = new PM(LanguageModelFST, fst::MATCH_INPUT, PHI_SYMBOLID, false);
  fst::ComposeFstOptions<fst::LogArc, PM> copts2(fst::CacheOptions(), PM12, PM22);
  fst::ComposeFst<fst::LogArc> Input_Unk_Lex_LM(Input_Unk_Lex, LanguageModelFST, copts2);
//   fst::ComposeFst<fst::LogArc> Input_Unk_Lex_LM(Input_Unk_Lex_OSort, LanguageModelFST, copts2);
  (*tInSample)[2].AddTimeSinceStartToDuration();

  // print input, lexicon, language model and composition results
//   FileReader::PrintFST("lattice_debug/in.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(*InputFst), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/lex.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(*LexiconTransducer), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/in_lex.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(Input_Unk_Lex_OSort), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/in_lex.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(Input_Unk_Lex), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/lm.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(LanguageModelFST), true, NAMESANDIDS);
//   FileReader::PrintFST("lattice_debug/in_lex_lm.fst", LanguageModel->GetId2CharacterSequenceVector(), fst::VectorFst<fst::LogArc>(Input_Unk_Lex_LM), true, NAMESANDIDS);

  // use beamserach, if specified
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

  // sample segmentation
  (*tInSample)[3].SetStart();
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

vector< bool > SampleLib::GetActiveWordIdsInFst(
  const fst::Fst< fst::LogArc > &SegmentFST,
  int MaxNumWords)
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
    std::cout << "WARNING: Sampling failed, probability mass left at end of cycle";
    i--;
  }
  return i;
}

// Copyright 2010, Graham Neubig, modified by Jahn Heymann (2013) and Oliver Walter (2014) //
void SampleLib::SampGen(const fst::Fst< fst::LogArc > &ifst,
                        fst::MutableFst< fst::LogArc > *ofst,
                        unsigned int nbest)
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
        std::cout << "stateWeights[" << a.nextstate << "]: ("
                  << stateWeights[a.nextstate] << "+(" << stateWeights[s]
                  << "*" << a.weight << "<-" << s << "[" << a.ilabel
                  << "/" << a.olabel << "]" << "))" << endl;
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

  std::cout.flush();
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
