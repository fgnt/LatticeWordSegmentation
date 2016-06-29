// ----------------------------------------------------------------------------
/**
   File: NHPYLM.hpp

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

   Description: nested hierarchical pitman yor language model

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _NHPYLM_HPP_
#define _NHPYLM_HPP_

#include <mutex>
#include "HPYLM.hpp"
#include "Dictionary.hpp"

/* nested hierarchical pitman yor language model */
class NHPYLM: public Dictionary {
  // character hierarchical pitman yor language model
  HPYLM CHPYLM;
  // word hierarchical pitman yor language model
  HPYLM WHPYLM;
  // order of character hierarchical pitman yor language model
  const unsigned int CHPYLMOrder;
  // order of word hierarchical pitman yor language model
  const unsigned int WHPYLMOrder;
  // Begin of characters (first character id)
  const int CharactersBegin;
  // End of characters (1 + last character)
  const int CharactersEnd;
  // Number of characters
  const unsigned int NumCharacters;
  // parameters for character and word pitman yor language model
  const NHPYLMParameters Parameters;
  const double WordBaseProbability;

  // base probabilities for characters
  mutable google::dense_hash_map<int, double> CHPYLMBaseProbabilities;
  // base probabilities for words
  mutable google::dense_hash_map<int, double> WHPYLMBaseProbabilities;
  // mutex to allow multi threading
  mutable std::mutex mtx;

  /* some internal functions */
  // Add the character sequence of a word to the character language model
  void AddCharacterSequenceToCHPYLM(
    const std::vector<int> &CharacterSequence
  );
  
  // remove the character sequence of a word ftom the character language model
  void RemoveCharacterSequenceFromCHPYLM(
    const std::vector<int> &CharacterSequence
  );

public:
  /* constructor */
  // construct nested hierarchical pitman yor language model
  NHPYLM(
    unsigned int CHPYLMOrder_,
    unsigned int WHPYLMOrder_,
    const std::vector< std::string > &Symbols_,
    int CharactersBegin_,
    const double WordBaseProbability_ = 0.0
  );

  /* interface: language model */
  // add word to language model
  void AddWordToLm(
    const const_witerator &Word
  );

  // add sequence of words to language model
  void AddWordSequenceToLm(
    const std::vector<int> &WordSequence
  );

  // remove sequence of words from language model
  void RemoveWordSequenceFromLm(
    const std::vector<int> &WordSequence
  );

  // remove word from language model
  bool RemoveWordFromLm(
    const const_witerator &Word
  );

  // calculate probability of a word
  double WordProbability(
    const const_witerator &Word
  ) const;

  // calculate probabilities of all words in given vector in given context
  std::vector<double> WordVectorProbability(
    const std::vector< int > &ContextSequence,
    const std::vector< int > &Words
  ) const;

  // calculate log likelihood of a word sequence
  double WordSequenceLoglikelihood(
    const std::vector< int > &WordSequence
  ) const;
  
  // Resample hyper parameters of the hierarchical models
  void ResampleHyperParameters();
  
  // Get the parameters of the CHPYLM and WHPYLM
  const NHPYLMParameters &GetNHPYLMParameters() const;
  
  // return id of given word context
  int GetContextId(
    const std::vector<int> &ContextSequence
  ) const;
  
  // Get possible transitions from one context to another
  ContextToContextTransitions GetTransitions(
    int ContextId,
    int SentEndWordId,
    const std::vector< bool > &ActiveWords,
    int ReturnToContextId = -1
  ) const;

  // get the final state (sentence end)
  int GetFinalContextId() const;

  // get start state (sentence start)
  int GetRootContextId() const;

  // get the character hierarchical language model order
  int GetCHPYLMOrder() const;

  // get the word hierarchical language model order
  int GetWHPYLMOrder() const;

  // get total count per level for given LM ("CHPYLM"|"WHPYLM")
  // and name ("Context"|"Table"|"Word")
  std::vector<int> GetTotalCountPerLevelFor(
    const std::string &LM,
    const std::string &CountName
  ) const;

  // generate character or word sequences from the language models
  std::vector<std::vector<int> > Generate(
    std::string Mode,
    int NumWorsdOrCharacters,
    int SentEndWordId,
    std::vector<double> *GeneratedWordLengthDistribution_
  ) const;
  
  void SetWHPYLMBaseProbabilitiesScale(
    const std::vector<double> &WHPYLMBaseProbabilitiesScale_
  );
  
  void SetCharBaseProb(
    const int CharId,
    const double prob
  );

  const std::vector<double> &GetWHPYLMBaseProbabilitiesScale() const;
  
  int GetWHPYLBaseTablesPerWord(
    int WordId
  ) const;

  void SetParameter(
    const std::string &LM,
    const std::string &Parameter,
    int Level,
    double Value
  );
};

#endif