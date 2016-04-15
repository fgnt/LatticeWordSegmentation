// ----------------------------------------------------------------------------
/**
   File: ParseLib.cpp
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


   ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst: Modified
   according to own needs (by Jahn Heymann (2013) and Oliver Walter (2014))
*/
// ----------------------------------------------------------------------------
#include "ParseLib.hpp"
// #include "DebugLib.hpp"

int ParseLib::AddCharacterIdSequenceToDictionaryAndLexFST(
  const vector< int > &Characters,
  Dictionary *LanguageModel,
  LexFst *LexiconTransducer)
{
  WordIdAddedPair wIdAddedPair = LanguageModel->AddCharacterIdSequenceToDictionary(Characters.begin(), Characters.size());
  if (wIdAddedPair.second && (LexiconTransducer != NULL)) {
    LexiconTransducer->addWord(Characters.begin(), Characters.size(), wIdAddedPair.first);
  }
  return wIdAddedPair.first;
}

void ParseLib::RemoveWordsFromDictionaryLexFSTAndLM(
  const const_witerator &Word,
  int NumWords,
  NHPYLM *LanguageModel,
  LexFst *LexiconTransducer,
  int SentEndWordId)
{
//   std::cout << "Removing: ";
//   DebugLib::PrintSentence(Word, NumWords, LanguageModel->GetId2CharacterSequenceVector());
  for (int IdxWord = 0; IdxWord < NumWords; ++IdxWord) {
    RemoveWordFromDictionaryLexFSTAndLM(Word + IdxWord, LanguageModel, LexiconTransducer, SentEndWordId);
  }
}

void ParseLib::RemoveWordFromDictionaryLexFSTAndLM(
  const const_witerator &Word,
  NHPYLM *LanguageModel,
  LexFst *LexiconTransducer,
  int SentEndWordId)
{
//   std::cout << "Removing: " << *Word << " from LM" << std::endl;
  if (LanguageModel->RemoveWordFromLm(Word) && (*Word != SentEndWordId)) {
//     std::cout << "Removing: " << *Word << " from Lexicon and Transducer" << std::endl;
    WordBeginLengthPair WordBeginLengh = LanguageModel->GetWordBeginLength(*Word);
    LexiconTransducer->rmWord(WordBeginLengh.first, WordBeginLengh.second);
    LanguageModel->RemoveWordFromDictionary(*Word);
  }
}

void ParseLib::ParseSampleAndAddCharacterIdSequenceToDictionaryLexFstAndLM(
  const fst::Fst< fst::LogArc > &Sample,
  int SentEndWordId,
  NHPYLM *LanguageModel,
  LexFst *LexiconTransducer,
  vector< WordId > *Sentence,
  vector< ArcInfo > *TimedSentence,
  const vector< ArcInfo > &InputArcInfos)
{
//   std::cout << "Parsing: " << std::endl;
  ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst(
    Sample, LanguageModel, LexiconTransducer,
    Sentence, TimedSentence, InputArcInfos);
  int WHPYLMContextLenght = LanguageModel->GetWHPYLMOrder() - 1;
  Sentence->insert(Sentence->begin(), WHPYLMContextLenght, SentEndWordId);

//   std::cout << "Adding:   ";
//   DebugLib::PrintSentence((*ret).begin(), (*ret).size(), LanguageModel->GetId2CharacterSequenceVector());
  LanguageModel->AddWordSequenceToLm(*Sentence);
}

// Copyright 2010, Graham Neubig, modified by Jahn Heymann (2013) and Oliver Walter (2014) //
void ParseLib::ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst(
  const fst::Fst< fst::LogArc > &Sample,
  Dictionary *Dict,
  LexFst *LexiconTransducer,
  vector< WordId > *Sentence,
  vector< ArcInfo > *TimedSentence,
  const vector< ArcInfo > &InputArcInfos)
{
  // reset sentences and initialize some variables
  Sentence->clear();
  if (!InputArcInfos.empty()) {
    TimedSentence->clear();
  }
  std::vector<CharId> charBuf;
  float WordStartTime = -1;
  float WordEndTime = -1;
  int sid = Sample.Start();
  int WordsBegin = Dict->GetWordsBegin();

  // continue with next arc until there are no more arcs left
//   std::cout << "Parsing:   " << std::endl << std::flush;
  while (true) {
    // get next arc
    fst::ArcIterator<fst::Fst<fst::LogArc> > ai(Sample, sid);
    if (ai.Done()) {
      break;
    }
    const fst::LogArc &arc = ai.Value();
    
    // update start and end times of word
    if (!InputArcInfos.empty()) {
      if (WordStartTime == -1) {
        WordStartTime = InputArcInfos[arc.ilabel].start;
      }
      if (InputArcInfos[arc.ilabel].end != -1) {
        WordEndTime = InputArcInfos[arc.ilabel].end;
      }
      if (WordStartTime > WordEndTime) {
        throw std::runtime_error("Word end time is before word start time");
      }
    }

    // add known words
    if (arc.olabel >= WordsBegin) {
      if (charBuf.size() > 0) {
        throw std::runtime_error("Word with non-empty buffer (/unk required)");
      }
      WordId wid = arc.olabel;
      Sentence->push_back(wid);
      if (!InputArcInfos.empty()) {
        TimedSentence->push_back(ArcInfo(wid, WordStartTime, WordEndTime));
        WordStartTime = -1;
      }
//       std::cout << "[Existing word: " << ret->back() << "] ";
    }

    // handle the end of unknown word symbol
    else if (arc.olabel == UNKEND_SYMBOLID) {
      if (charBuf.size() == 0) {
        throw std::runtime_error("End of word symbol with empty buffer");
      }
      WordId wid = AddCharacterIdSequenceToDictionaryAndLexFST(
        charBuf, Dict, LexiconTransducer);
      Sentence->push_back(wid);
      if (!InputArcInfos.empty()) {
        TimedSentence->push_back(ArcInfo(wid, WordStartTime, WordEndTime));
        WordStartTime = -1;
      }
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
    WordId wid = AddCharacterIdSequenceToDictionaryAndLexFST(
      charBuf, Dict, LexiconTransducer);
    Sentence->push_back(wid);
    if (!InputArcInfos.empty()) {
      TimedSentence->push_back(ArcInfo(wid, WordStartTime, WordEndTime));
      WordStartTime = -1;
    }
//     std::cout << "[New word: " << ret->back() << "] ";
  }
//   std::cout << std::endl;
}

void ParseLib::ParseSampleAndAddCharacterIdSequenceToDictionary(
  const fst::Fst< fst::LogArc > &Sample,
  Dictionary *Dict,
  std::vector<int> *Sentence,
  vector< ArcInfo > *TimedSentence,
  const vector< ArcInfo > &InputArcInfos)
{
  ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst(
    Sample, Dict, nullptr, Sentence, TimedSentence, InputArcInfos);
}
