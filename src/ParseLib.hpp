// ----------------------------------------------------------------------------
/**
   File: ParseLib.hpp

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

   Description: functions used for sampling a segmentation from an input lattice

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial


   Note: License for function
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


   Note Change History: for functions SampleWeights, SampGen and
   ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst:
   Date         Author       Description
   2010         Neubig       Inital
   2013         Heymann      Modifications to own needs
   2014         Walter       Modifications to own needs
*/
// ----------------------------------------------------------------------------
#ifndef _PARSELIB_HPP_
#define _PARSELIB_HPP_

#include "NHPYLMFst.hpp"
#include "LexFst.hpp"

/* library for parsing samples from input lattice */
class ParseLib {
  // remove given word from dictionary, lexicon transducer and language model
  inline static void RemoveWordFromDictionaryLexFSTAndLM(
    const const_witerator &Word,
    NHPYLM *LanguageModel,
    LexFst *LexiconTransducer,
    int SentEndWordId
  );

  // parse sampled fst and add character id sequence to dictionary, also add
  // new character id sequences to lexicon transducer
  inline static void ParseSampleAndAddCharacterIdSequenceToDictionaryAndLexFst(
    const fst::Fst< fst::LogArc >& Sample,
    Dictionary* Dict,
    LexFst* LexiconTransducer,
    vector< WordId >* Sentence,
    vector< ArcInfo >* TimedSentence,
    const vector< ArcInfo >& InputArcInfos
  );

  // add character id sequence to dictionary and add new sewuences to lexicon
  // transducer lexicon fst
  inline static int AddCharacterIdSequenceToDictionaryAndLexFST(
    const std::vector<int> &Characters,
    Dictionary *Dict,
    LexFst *LexiconTransducer
  );

public:
  // remove words from language model and from lexicon fst and dictionary if
  // word count is zero
  static void RemoveWordsFromDictionaryLexFSTAndLM(
    const const_witerator &Word,
    int NumWords,
    NHPYLM *LanguageModel,
    LexFst *LexiconTransducer,
    int SentEndWordId
  );

  // parse sampled fst and add character id sequence to dictionary and language
  // model, also add new character id sequences to lexicon transducer
  static void ParseSampleAndAddCharacterIdSequenceToDictionaryLexFstAndLM(
    const fst::Fst< fst::LogArc >& Sample,
    int SentEndWordId,
    NHPYLM* LanguageModel,
    LexFst* LexiconTransducer,
    vector< WordId >* Sentence,
    vector< ArcInfo >* TimedSentence,
    const vector< ArcInfo >& InputArcInfos
  );

  // parse character lattice and add word ids to dictionary and return vector of
  // word Ids
  static void ParseSampleAndAddCharacterIdSequenceToDictionary(
    const fst::Fst< fst::LogArc > &Sample,
    Dictionary *Dict,
    vector< WordId >* Sentence,
    vector< ArcInfo >* TimedSentence = nullptr,
    const vector< ArcInfo >& InputArcInfos = std::vector<ArcInfo>()
  );
};

#endif