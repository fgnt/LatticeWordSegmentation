// ----------------------------------------------------------------------------
/**
   File: NHPYLM.cpp
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
#include <iomanip>
#include <iostream>
#include "NHPYLM.hpp"

NHPYLM::NHPYLM(
  unsigned int CHPYLMOrder_,
  unsigned int WHPYLMOrder_,
  const std::vector<std::string> &Symbols_,
  int CharactersBegin_,
  const double WordBaseProbability_
) :
  Dictionary(CHPYLMOrder_ - 1, Symbols_),
  CHPYLM(CHPYLMOrder_),
  WHPYLM(WHPYLMOrder_),
  CHPYLMOrder(CHPYLMOrder_),
  WHPYLMOrder(WHPYLMOrder_),
  CharactersBegin(CharactersBegin_),
  CharactersEnd(Symbols_.size()),
  NumCharacters(CharactersEnd - CharactersBegin),
  Parameters(CHPYLM.GetHPYLMParameters().Discount,
             CHPYLM.GetHPYLMParameters().Concentration,
             WHPYLM.GetHPYLMParameters().Discount,
             WHPYLM.GetHPYLMParameters().Concentration),
  WordBaseProbability(WordBaseProbability_),
  CHPYLMBaseProbabilities(),
  WHPYLMBaseProbabilities(),
  mtx()
{
  CHPYLMBaseProbabilities.set_deleted_key(DELETED);
  CHPYLMBaseProbabilities.set_empty_key(EMPTY);
  WHPYLMBaseProbabilities.set_deleted_key(DELETED);
  WHPYLMBaseProbabilities.set_empty_key(EMPTY);

  /* initialize base probabilities for character
   * hierarchical pitman yor language model */
  CHPYLMBaseProbabilities[EOW] = 1.0 / (NumCharacters + 2);
  CHPYLMBaseProbabilities[EOS] = 1.0 / (NumCharacters + 2);
  for (int CharacterId = CharactersBegin; CharacterId < CharactersEnd; CharacterId++) {
    CHPYLMBaseProbabilities[CharacterId] = 1.0 / (NumCharacters + 2);
  }
}

void NHPYLM::SetCharBaseProb(const int CharId, const double prob)
{
    CHPYLMBaseProbabilities[CharId] = prob;
}

void NHPYLM::AddWordToLm(const const_witerator &Word)
{
  /* debug */
//   PrintDebugHeader << ": Adding word id " << *Word << " with context "<< "|";
//   for(std::vector<int>::const_iterator it = Word - WHPYLMOrder + 1; it != Word; it++) {
//     std::cout << *it << "|";
//   }
//   std::cout << " to LM " << std::endl;

  /* get base probability for character sequence represting word */
  const std::vector<int> *CharacterSequence = nullptr;
  if (NumCharacters > 0) {
    CharacterSequence = &GetWordVector(*Word);
  }
//  for(const_citerator it = CharacterSequence.begin() + CHPYLMOrder - 1; it != CharacterSequence.end(); ++it) {
//     std::cout << *it << "|";
//   }
  double BaseProbability;
  if ((WordBaseProbability == 0.0) && (NumCharacters > 0) && (CHPYLMOrder > 0)) {
     BaseProbability = exp(CHPYLM.WordSequenceLoglikelihood(*CharacterSequence, CHPYLMBaseProbabilities));
   } else {
     BaseProbability = WordBaseProbability;
   }

  /* debug */
//   PrintDebugHeader << ": Adding word id " << *Word
//                    << " with character id sequence |";
//   for(const_citerator it = CharacterSequence.begin() + CHPYLMOrder - 1; it != CharacterSequence.end(); ++it) {
//     std::cout << *it << "|";
//   }
//   std::cout << " and base probability " << BaseProbability << " to LM "<< std::endl;

  /* add the word to the nested hierarchical pitman yor language model */
  if (WHPYLM.AddWord(Word, BaseProbability)) {
    if ((NumCharacters > 0) && (CHPYLMOrder > 0)) {
      AddCharacterSequenceToCHPYLM(*CharacterSequence);
    }
  }
}

void NHPYLM::AddWordSequenceToLm(const std::vector< int > &WordSequence)
{
  /* debug */
//   PrintDebugHeader << ": Adding word id sequence |";
//   for(const_witerator it = WordSequence.begin() + WHPYLMOrder - 1; it != WordSequence.end(); it++) {
//     std::cout << *it << "|";
//   }
//   std::cout << " to LM " << std::endl;

  for (const_witerator it = WordSequence.begin() + WHPYLMOrder - 1; it != WordSequence.end(); ++it) {
    AddWordToLm(it);
  }
}

void NHPYLM::RemoveWordSequenceFromLm(const std::vector< int > &WordSequence)
{
  for (const_witerator it = WordSequence.begin() + WHPYLMOrder - 1; it != WordSequence.end(); ++it) {
    RemoveWordFromLm(it);
  }
}


void NHPYLM::AddCharacterSequenceToCHPYLM(const std::vector<int> &CharacterSequence)
{
//   /* debug */
//   PrintDebugHeader << ": Adding Character Sequence to LM: ";
//   for(std::vector<int>::iterator it = CharacterSequence.begin(); it != CharacterSequence.end(); it++) {
//     std::cout << *it << "|";
//   }
//   std::cout << std::endl;

  /* add each character of a word to the character language model */
  for (const_citerator it = CharacterSequence.begin() + CHPYLMOrder - 1; it != CharacterSequence.end(); ++it) {
    CHPYLM.AddWord(it, CHPYLMBaseProbabilities[*it]);
  }

  /* reset word base probabilities */
  WHPYLMBaseProbabilities.clear();
}


bool NHPYLM::RemoveWordFromLm(const const_witerator &Word)
{
//   /* debug */
//   PrintDebugHeader << ": Removing Word with context from LM " << *Word << ":|";
//   for(std::vector<int>::iterator it = Word - WHPYLMOrder + 1; it != Word; it++) {
//     std::cout << *it << "|";
//   }
//   std::cout << std::endl;

  /* remove word from the nested hierarchical pitman yor language model */
  WordRemoveStatus Removed = WHPYLM.RemoveWord(Word);
  if (Removed != NONEREMOVED) {
//     std::cout << "Removed Word: " << *Word << std::endl;
    if ((NumCharacters > 0) && (CHPYLMOrder > 0)) {
      RemoveCharacterSequenceFromCHPYLM(GetWordVector(*Word));
    }
    if (Removed != TABLE) {
      return true;
    }
  }
  return false;
}

void NHPYLM::RemoveCharacterSequenceFromCHPYLM(const std::vector<int> &CharacterSequence)
{
//   /* debug */
//   PrintDebugHeader << ": Removing Character Sequence from LM: ";
//   for(std::vector<int>::iterator it = CharacterSequence.begin(); it != CharacterSequence.end(); it++) {
//     std::cout << *it << "|";
//   }
//   std::cout << std::endl;

  /* remove each character of a word from the character language model */
  for (const_citerator it = CharacterSequence.begin() + CHPYLMOrder - 1; it != CharacterSequence.end(); ++it) {
    CHPYLM.RemoveWord(it);
  }

  /* reset word base probabilities */
  WHPYLMBaseProbabilities.clear();
}

double NHPYLM::WordProbability(const const_witerator &Word) const
{
  mtx.lock();
  /* get base probability for character sequence represting word and calculate word probability */

  double BaseProbability;
  if ((WordBaseProbability == 0.0) && (NumCharacters > 0) && (CHPYLMOrder > 0)) {
      google::dense_hash_map<int, double>::const_iterator it = WHPYLMBaseProbabilities.find(*Word);
      if (it == WHPYLMBaseProbabilities.end()) {
        it = WHPYLMBaseProbabilities.insert(std::make_pair(*Word, exp(CHPYLM.WordSequenceLoglikelihood(GetWordVector(*Word), CHPYLMBaseProbabilities)))).first;
      }
    BaseProbability = it->second;
  } else {
    BaseProbability = WordBaseProbability;
  }

  mtx.unlock();

  return WHPYLM.WordProbability(Word, BaseProbability);
}

std::vector<double> NHPYLM::WordVectorProbability(const std::vector< int > &ContextSequence, const std::vector< int > &Words) const
{
  /* get base probability for character sequences represting words and calculate word probabilities */
  std::vector<double> BaseProbabilites;
  BaseProbabilites.reserve(Words.size());
  mtx.lock();
  for (std::vector<int>::const_iterator Word = Words.begin(); Word != Words.end(); ++Word) {
    double BaseProbability;
    if (*Word != PHI) {
      if ((WordBaseProbability == 0.0) && (NumCharacters > 0) && (CHPYLMOrder > 0)) {
        google::dense_hash_map<int, double>::const_iterator it = WHPYLMBaseProbabilities.find(*Word);
        if (it == WHPYLMBaseProbabilities.end()) {
  //    std::cout << *Word << ":" << dict.get_word_vector(*Word).size() << std::endl;
          BaseProbability = exp(CHPYLM.WordSequenceLoglikelihood(GetWordVector(*Word), CHPYLMBaseProbabilities));
          WHPYLMBaseProbabilities.insert(std::make_pair(*Word, BaseProbability));
        } else {
          BaseProbability = it->second;
        }
      } else {
        BaseProbability = WordBaseProbability;
      }
      BaseProbabilites.push_back(BaseProbability);
    } else {
      BaseProbabilites.push_back(0);
    }
  }
  mtx.unlock();
  WHPYLM.WordVectorProbability(ContextSequence, Words, &BaseProbabilites);
  return BaseProbabilites;
}

double NHPYLM::WordSequenceLoglikelihood(const std::vector< int > &WordSequence) const
{
  std::lock_guard<std::mutex> lck(mtx);

  /* calculate base probabilities */
  for (const_witerator Word = WordSequence.begin() + WHPYLMOrder - 1; Word != WordSequence.end(); ++Word) {
    if ((WordBaseProbability == 0.0) && (NumCharacters > 0) && (CHPYLMOrder > 0)) {
      google::dense_hash_map<int, double>::const_iterator it = WHPYLMBaseProbabilities.find(*Word);
      if (it == WHPYLMBaseProbabilities.end()) {
        WHPYLMBaseProbabilities.insert(std::make_pair(*Word, exp(CHPYLM.WordSequenceLoglikelihood(GetWordVector(*Word), CHPYLMBaseProbabilities))));
      }
    } else {
      WHPYLMBaseProbabilities.insert(std::make_pair(*Word, WordBaseProbability));
    }
  }

  /* calculate word sequence likelihood */
  return WHPYLM.WordSequenceLoglikelihood(WordSequence, WHPYLMBaseProbabilities);
}

void NHPYLM::ResampleHyperParameters()
{
  if ((WordBaseProbability == 0.0) && (NumCharacters > 0) && (CHPYLMOrder > 0)) {
    CHPYLM.ResampleHyperParameters();
    WHPYLMBaseProbabilities.clear();
  }
  WHPYLM.ResampleHyperParameters();
}

const NHPYLMParameters &NHPYLM::GetNHPYLMParameters() const
{
  return Parameters;
}

int NHPYLM::GetContextId(const std::vector< int > &ContextSequence) const
{
  return WHPYLM.GetContextId(ContextSequence) + GetRootContextId();
}

int NHPYLM::GetRootContextId() const
{
  if ((WordBaseProbability == 0.0) && (NumCharacters > 0) && (CHPYLMOrder > 0)) {
      return CHPYLM.GetNextUnusedContextId();
  } else {
    return 0;
  }
}

ContextToContextTransitions NHPYLM::GetTransitions(
  int ContextId,
  int SentEndWordId,
  const std::vector<bool> &ActiveWords,
  int ReturnToContextId
) const
{
  int WordContextIdOffset = GetRootContextId();
  int FinalContextId = GetFinalContextId();
//   PrintDebugHeader << ": Word context id offset: " << WordContextIdOffset << ", Next unused word context id: " << NextUnusedWordContextId << std::endl;

//   PrintDebugHeader << ": " << ContextId;
  ContextToContextTransitions Transitions;
  if (ContextId < WordContextIdOffset) {
//     std::cout << " (character id)" << std::endl;
    Transitions = CHPYLM.GetTransitions(ContextId, EOW, ActiveWords);

    if (ContextId == 0) {
      if (Transitions.Words.size() < (NumCharacters + 2)) {
        std::vector<int> AvailableCharacters;
        AvailableCharacters.reserve(NumCharacters + 1);
        AvailableCharacters.push_back(EOW);
        for (int Character = CharactersBegin; Character < CharactersEnd; Character++) {
          AvailableCharacters.push_back(Character);
        }

        std::vector<int> PresentCharacters(Transitions.Words);
        std::sort(PresentCharacters.begin(), PresentCharacters.end());

        std::vector<int> MissingCharacters;
        MissingCharacters.resize(NumCharacters + 1);

        std::vector<int>::const_iterator MissingCharactersEnd;
        MissingCharactersEnd = std::set_difference(AvailableCharacters.begin(), AvailableCharacters.end(), PresentCharacters.begin(), PresentCharacters.end(), MissingCharacters.begin());
        std::vector<int> MissingCharacterContextSequence(1);
        for (std::vector<int>::iterator MissingCharacter = MissingCharacters.begin(); MissingCharacter != MissingCharactersEnd; ++MissingCharacter) {
          Transitions.Words.push_back(*MissingCharacter);
          if (*MissingCharacter != EOW) {
            MissingCharacterContextSequence[0] = *MissingCharacter;
            Transitions.NextContextIds.push_back(CHPYLM.GetContextId(MissingCharacterContextSequence));
          } else {
            Transitions.NextContextIds.push_back(WordContextIdOffset);
          }
        }
      }
    }

    Transitions.Probabilities.reserve(Transitions.Words.size());
    for (std::vector<int>::iterator Word = Transitions.Words.begin(); Word != Transitions.Words.end(); ++Word) {
      if (*Word != PHI) {
        Transitions.Probabilities.push_back(CHPYLMBaseProbabilities.find(*Word)->second);
      } else {
        Transitions.Probabilities.push_back(0);
      }
    }
    CHPYLM.WordVectorProbability(CHPYLM.GetContextSequence(ContextId), Transitions.Words, &Transitions.Probabilities);
  } else if (ContextId < FinalContextId) {
//     std::cout << " (word id)" << std::endl;
    Transitions = WHPYLM.GetTransitions(ContextId - WordContextIdOffset, SentEndWordId, ActiveWords);

    for (iiterator NextContextId = Transitions.NextContextIds.begin(); NextContextId != Transitions.NextContextIds.end(); ++NextContextId) {
      *NextContextId += WordContextIdOffset;
      if ((ReturnToContextId > -1) && (*NextContextId == FinalContextId)) {
        *NextContextId = ReturnToContextId;
      }
    }

    if (ContextId == WordContextIdOffset) {
      if ((WordBaseProbability == 0.0) && (NumCharacters > 0) && (CHPYLMOrder > 0)) {
        /* add fallback to character model */
        Transitions.Words.push_back(PHI);
        std::vector<int> CharacterStartContextSequence(CHPYLMOrder - 1, EOW);
        Transitions.NextContextIds.push_back(CHPYLM.GetContextId(CharacterStartContextSequence));
      }

      /* add end of sentence in case it is missing (for example an empty language model) */
      if (!Transitions.HasTransitionToSentEnd) {
        Transitions.Words.push_back(SentEndWordId);
        if (ReturnToContextId < 0) {
          Transitions.NextContextIds.push_back(FinalContextId);
        } else {
          Transitions.NextContextIds.push_back(ReturnToContextId);
        }
      }
    }
//     std::cout << "WHPYLMContextId: " << ContextId - WordContextIdOffset << std::endl;
    Transitions.Probabilities = WordVectorProbability(WHPYLM.GetContextSequence(ContextId - WordContextIdOffset), Transitions.Words);
  } else {
//     std::cout << " (sent end id)" << std::endl;
  }
  return Transitions;
}

int NHPYLM::GetFinalContextId() const
{
  return WHPYLM.GetNextUnusedContextId() + GetRootContextId();
}

int NHPYLM::GetCHPYLMOrder() const
{
  return CHPYLMOrder;
}

int NHPYLM::GetWHPYLMOrder() const
{
  return WHPYLMOrder;
}

std::vector< int > NHPYLM::GetTotalCountPerLevelFor(const std::string &LM, const std::string &CountName) const
{
  const HPYLM *LMPointer;
  if (LM == "CHPYLM") {
    LMPointer = &CHPYLM;
  } else if (LM == "WHPYLM") {
    LMPointer = &WHPYLM;
  } else {
    return std::vector<int>();
  }

  if (CountName == "Context") {
    return LMPointer->GetTotalContextCountPerLevel();
  } else if (CountName == "Table") {
    return LMPointer->GetTotalTablecountPerLevel();
  } else if (CountName == "Word") {
    return LMPointer->GetTotalWordcountPerLevel();
  } else {
    return std::vector<int>();
  }
}

std::vector< std::vector< int > > NHPYLM::Generate(std::string Mode, int NumWorsdOrCharacters, int SentEndWordId, std::vector<double> *GeneratedWordLengthDistribution_) const
{
  if (Mode == "CHPYLM") {
    /* build vector of character ids */
    std::vector<int> CharacterIds(CharactersEnd - CharactersBegin, 0);
    std::iota(CharacterIds.begin(), CharacterIds.end(), CharactersBegin);
    CharacterIds.push_back(EOW);
    CharacterIds.push_back(EOS);
    CharacterIds.push_back(PHI);

    /* initialize base probabilities */
    std::vector<double> BaseProbabilites;
    BaseProbabilites.reserve(CharacterIds.size());
    for (std::vector<int>::iterator Character = CharacterIds.begin(); Character != CharacterIds.end(); ++Character) {
      if (*Character != PHI) {
        BaseProbabilites.push_back(CHPYLMBaseProbabilities.find(*Character)->second);
      } else {
        BaseProbabilites.push_back(0);
      }
    }

    /* sample the sentences */
    int WordLength = 0;
    int NumWords = 0;
    double MeanWordLength = 0;
    std::vector<double> GeneratedWordLengthDistribution;
    std::vector<double> *WordLengthsProbabilities;
    if (GeneratedWordLengthDistribution_) {
      WordLengthsProbabilities = GeneratedWordLengthDistribution_;
      WordLengthsProbabilities->clear();
    } else {
      WordLengthsProbabilities = &GeneratedWordLengthDistribution;
    }
    std::vector<int> CurrentContext(CHPYLMOrder - 1, EOW);
    std::vector<int> GeneratedSentence;
    std::vector<std::vector<int> > GeneratedSentences;
    while (NumWords < NumWorsdOrCharacters) {
      GeneratedSentence.push_back(CHPYLM.GenerateWord(CurrentContext, CharacterIds, BaseProbabilites, true));
      WordLength++;
      if ((CHPYLMOrder > 1) && (GeneratedSentence.back() != EOW) && (GeneratedSentence.back() != EOS)) {
        CurrentContext.erase(CurrentContext.begin());
        CurrentContext.push_back(GeneratedSentence.back());
      } else {
        if (GeneratedSentence.back() == EOW) {
          NumWords++;
          MeanWordLength += WordLength;
          WordLengthsProbabilities->resize(std::max(static_cast<size_t>(WordLength + 1), WordLengthsProbabilities->size()), 0);
          WordLengthsProbabilities->at(WordLength)++;
          WordLength = 0;
        }
        if (GeneratedSentence.back() == EOS) {
          GeneratedSentence.push_back(EOW);
          GeneratedSentences.push_back(GeneratedSentence);
          GeneratedSentence.clear();
        }
        CurrentContext.assign(CHPYLMOrder - 1, EOW);
      }
    }

    for (unsigned int WordLength = 0; WordLength < WordLengthsProbabilities->size(); WordLength++) {
      WordLengthsProbabilities->at(WordLength) /= NumWords;
      std::cout << std::setprecision(4) << std::fixed << "Word length: " << WordLength << ", word length probability: " << WordLengthsProbabilities->at(WordLength) << std::endl;
    }
    MeanWordLength /= NumWords;
    std::cout << "Mean word length: " << MeanWordLength << ", number of words: " << NumWords << std::endl << std::endl;

    return GeneratedSentences;
  } else if (Mode == "WHPYLM") {
    /* build vector of word ids */
    std::vector<int> WordIds;
    WordIds.reserve(GetId2Word().size());
    for (Id2WordHashmap::const_iterator Id2Word = GetId2Word().begin(); Id2Word != GetId2Word().end(); ++Id2Word) {
      WordIds.push_back(Id2Word->first);
    }
    WordIds.push_back(PHI);

    /* initialize base probabilities */
    std::vector<double> BaseProbabilites;
    BaseProbabilites.reserve(WordIds.size());
    for (std::vector<int>::iterator Word = WordIds.begin(); Word != WordIds.end(); ++Word) {
      if (*Word != PHI) {
        BaseProbabilites.push_back(exp(CHPYLM.WordSequenceLoglikelihood(GetWordVector(*Word), CHPYLMBaseProbabilities)));
      } else {
        BaseProbabilites.push_back(0);
      }
    }

    /* sample the sentences */
    std::vector<int> CurrentContext(WHPYLMOrder - 1, SentEndWordId);
    std::vector<int> GeneratedSentence;
    std::vector<std::vector<int> > GeneratedSentences;
    for (int NumWords = 0; NumWords < NumWorsdOrCharacters; NumWords++) {
      GeneratedSentence.push_back(WHPYLM.GenerateWord(CurrentContext, WordIds, BaseProbabilites, false));
      if ((WHPYLMOrder > 1) && (GeneratedSentence.back() != SentEndWordId)) {
        CurrentContext.erase(CurrentContext.begin());
        CurrentContext.push_back(GeneratedSentence.back());
      } else {
        if (GeneratedSentence.back() == SentEndWordId) {
          CurrentContext.assign(WHPYLMOrder - 1, SentEndWordId);
          GeneratedSentences.push_back(GeneratedSentence);
          GeneratedSentence.clear();
        }
      }
    }
    return GeneratedSentences;
  } else {
    return std::vector<std::vector<int> >();
  }
}

void NHPYLM::SetWHPYLMBaseProbabilitiesScale(const std::vector< double > &WHPYLMBaseProbabilitiesScale)
{
  CHPYLM.SetBaseProbabilitiesScale(WHPYLMBaseProbabilitiesScale);
}

const std::vector< double > &NHPYLM::GetWHPYLMBaseProbabilitiesScale() const
{
  return CHPYLM.GetBaseProbabilitiesScale();
}

int NHPYLM::GetWHPYLBaseTablesPerWord(int WordId) const
{
  return WHPYLM.GetBaseTablesPerWord(WordId);
}

void NHPYLM::SetParameter(const std::string &LM, const std::string &Parameter, int Level, double Value)
{
  HPYLM *LMPointer;
  if (LM == "CHPYLM") {
    LMPointer = &CHPYLM;
  } else if (LM == "WHPYLM") {
    LMPointer = &WHPYLM;
  } else {
    return;
  }

  if (Parameter == "Discount") {
    LMPointer->SetDiscount(Level, Value);
  } else if (Parameter == "Concentration") {
    LMPointer->SetConcentration(Level, Value);
  } else {
    return;
  }
}

NHPYLMParameters::NHPYLMParameters(const std::vector< double > &CHPYLMDiscount_, const std::vector< double > &CHPYLMConcentration_, const std::vector< double > &WHPYLMDiscount_, const std::vector< double > &WHPYLMConcentration_) :
  CHPYLMDiscount(CHPYLMDiscount_),
  CHPYLMConcentration(CHPYLMConcentration_),
  WHPYLMDiscount(WHPYLMDiscount_),
  WHPYLMConcentration(WHPYLMConcentration_)
{
}