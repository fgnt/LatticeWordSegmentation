// ----------------------------------------------------------------------------
/**
   File: HPYLM.cpp
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
#include <chrono>
#include "HPYLM.hpp"

std::default_random_engine HPYLM::RandomGenerator(std::chrono::system_clock::now().time_since_epoch().count());
std::gamma_distribution<double> HPYLM::GammaDistribution;
std::discrete_distribution<unsigned int> HPYLM::DiscreteDistribution;

HPYLM::HPYLM(int Order_) :
  Parameters(Order_, 0.5, 0.1),
  RestaurantTree(Parameters.Discount[0], Parameters.Concentration[0], NULL, 0, std::vector<int>()),
  Order(Order_),
  NextUnusedContextId(1),
  FreedIds(),
  SortFreedIds(false),
  ContextIdToContext()
{
  ContextIdToContext.set_empty_key(EMPTY);
  ContextIdToContext.set_deleted_key(DELETED);
  ContextIdToContext.insert(std::make_pair(RestaurantTree.ContextId, &RestaurantTree));
}

HPYLM::~HPYLM()
{
  DestructRestaurantTreeRecursively(&RestaurantTree);
}

void HPYLM::DestructRestaurantTreeRecursively(ContextRestaurant *CurrentRestaurant)
{
  for (ContextsHashmap::iterator NextContextIterator = CurrentRestaurant->NextContext.begin(); NextContextIterator != CurrentRestaurant->NextContext.end(); ++NextContextIterator) {
    DestructRestaurantTreeRecursively(NextContextIterator->second);
    delete NextContextIterator->second;
  }
}

bool HPYLM::AddWord(const const_witerator &Word, double BaseProbability)
{
//   PrintDebugHeader << ": Adding word/character id " << *Word << " with base probability " << BaseProbability << " recursively to LM" << std::endl;
  return AddWordRecursively(Word, 1, &RestaurantTree, BaseProbability);
}

bool HPYLM::AddWordRecursively(const const_witerator &Word, unsigned int level, ContextRestaurant *CurrentRestaurant, double BaseProbability)
{
  /* check if end of tree is reached */
  if (level < Order) {
    /* adjust base probability for word acording to current context */
    BaseProbability = CurrentRestaurant->ThisRestaurant.WordProbability(*Word, BaseProbability);

    /* find or create restaurant for given context */
    ContextsHashmap::iterator it = CurrentRestaurant->NextContext.find(*(Word - level));
    if (it == CurrentRestaurant->NextContext.end()) {
      /* get new contextid for resataurant */
      int ContextId = GetNextAvailableContextId();

//       /* debug */
//       PrintDebugHeader << ": Creating new restaurant" << " at level " << level + 1 << " for context id " << *(Word - level) << " with context id " << ContextId
//                        << " and context sequence |";
//       for(const_witerator it = Word - level; it != Word; ++it) {
//         std::cout << *it << "|";
//       }
//       std::cout << std::endl;

      /* create a new restaurant */
      ContextRestaurant *NextContext = new ContextRestaurant(Parameters.Discount[level], Parameters.Concentration[level], CurrentRestaurant, ContextId, std::vector<int>(Word - level, Word));
      ContextIdToContext.insert(std::make_pair(ContextId, NextContext));
      it = CurrentRestaurant->NextContext.insert(std::make_pair(*(Word - level), NextContext)).first;
    }

    /* recursively add word to tree */
    if (!AddWordRecursively(Word, level + 1, it->second, BaseProbability)) {
      /* finish recursive adding */
      return false;
    }
  }

  /* debug */
//   PrintDebugHeader << ": Adding word/character id " << *Word << " with base probability " << BaseProbability <<  " at level " << level;
//   if(level > 1) {
//     std::cout << " to context id " << *(Word - level + 1);
//   }
//   std::cout  << std::endl;

  /* add word within the tree if a new table was created */
  return CurrentRestaurant->ThisRestaurant.IncrementWordCount(*Word, BaseProbability);
}

int HPYLM::GetNextAvailableContextId()
{
  int NextAvailableContextId;

  if (FreedIds.empty()) {
    NextAvailableContextId = NextUnusedContextId++;
  } else {
    if (SortFreedIds) {
      FreedIds.sort();
      SortFreedIds = false;
    }
    NextAvailableContextId = FreedIds.front();
    FreedIds.pop_front();
  }

  return NextAvailableContextId;
}


WordRemoveStatus HPYLM::RemoveWord(const const_witerator &Word)
{
//   PrintDebugHeader << ": Removing word/character " << *Word << " recursively from LM" << std::endl;
  return RemoveWordRecursively(Word, 1, &RestaurantTree);
}

WordRemoveStatus HPYLM::RemoveWordRecursively(const const_witerator &Word, unsigned int level, ContextRestaurant *CurrentRestaurant)
{
  /* check if end of tree is reached */
  if (level < Order) {
    /* find restaurant for given context and recursively remove word from the tree */
    ContextsHashmap::iterator it = CurrentRestaurant->NextContext.find(*(Word - level));
    if (RemoveWordRecursively(Word, level + 1, it->second) == NONEREMOVED) {
      /* finish recursive removing */
      return NONEREMOVED;
    }
  }
//   /* debug */
//   PrintDebugHeader << ": Removing word/character " << *Word << " at level " << level;
//   if(level > 1) {
//     std::cout << " with context " << *(Word - level + 1) << std::endl;
//   } else {
//     std::cout << " without context" << std::endl;
//   }

  /* remove word within the tree if the table for the word was removed */
//   PrintDebugHeader << ": Decrementing WordCount for Word " << *Word << " in ContextId " << CurrentRestaurant->ContextId << std::endl;
  WordRemoveStatus Removed = CurrentRestaurant->ThisRestaurant.DecrementWordCount(*Word);

  /* remove current context (and the reference to it from the previous one) if it became empty */
  if ((Removed == TABLE_WORD_RESTAURANT) && (level != 1)) {
//     PrintDebugHeader << ": Removing restaurant" << " at level " << level << " with context " << *(Word - level + 1) << std::endl;
    CurrentRestaurant->PreviousContext->NextContext.erase(*(Word - level + 1));
    ContextIdToContext.erase(CurrentRestaurant->ContextId);
    FreedIds.push_back(CurrentRestaurant->ContextId);
    SortFreedIds = true;
    delete CurrentRestaurant;
  }
  return Removed;
}

double HPYLM::WordProbability(const const_witerator &Word, double BaseProbability) const
{
  return WordProbabilityRecursively(Word, 1, RestaurantTree, BaseProbability);
}

double HPYLM::WordProbabilityRecursively(const const_witerator &Word, unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, double BaseProbability) const
{
  /* adjust base probability for word acording to current context */
  BaseProbability = CurrentRestaurant.ThisRestaurant.WordProbability(*Word, BaseProbability);

  /* check if end of tree is reached */
  if (level < Order) {
    /* find restaurant for given context */
    ContextsHashmap::const_iterator it = CurrentRestaurant.NextContext.find(*(Word - level));
    if (it != CurrentRestaurant.NextContext.end()) {
      /* adjust base probability for word according to found context */
      BaseProbability = WordProbabilityRecursively(Word, level + 1, *(it->second), BaseProbability);
    }
  }
  return BaseProbability;
}

void HPYLM::WordVectorProbability(const std::vector< int > &ContextSequence, const std::vector< int > &Words, std::vector< double > *BaseProbabilities) const
{
  WordVectorProbabilityRecursively(ContextSequence.end(), Words, 1, ContextSequence.size(), RestaurantTree, BaseProbabilities);
}

void HPYLM::WordVectorProbabilityRecursively(const const_witerator &Word, const std::vector< int > &Words, unsigned int level, unsigned int ContextLenght, const HPYLM::ContextRestaurant &CurrentRestaurant, std::vector< double > *BaseProbabilities) const
{
  /* adjust base probabilities for the words acording to current context */
  CurrentRestaurant.ThisRestaurant.WordVectorProbability(Words, BaseProbabilities);

  /* check if end of tree is reached */
  if (level <= ContextLenght) {
    /* find restaurant for given context */
    ContextsHashmap::const_iterator it = CurrentRestaurant.NextContext.find(*(Word - level));
    if (it != CurrentRestaurant.NextContext.end()) {
      /* adjust base probabilities for the words according to found context */
      WordVectorProbabilityRecursively(Word, Words, level + 1, ContextLenght, *(it->second), BaseProbabilities);
    }
  }
}

double HPYLM::WordSequenceLoglikelihood(const std::vector< int > &WordSequence, const google::dense_hash_map< int, double > &BaseProbabilities) const
{
  double Loglikelihood = 0;
  for (const_witerator Word = WordSequence.begin() + Order - 1; Word != WordSequence.end(); ++Word) {
    Loglikelihood += log(WordProbability(Word, BaseProbabilities.find(*Word)->second));
  }
  if (BaseProbabilitiesScale.empty()) {
    return Loglikelihood;
  } else if (BaseProbabilitiesScale.size() > (WordSequence.size() - Order + 1)) {
    return Loglikelihood + log(BaseProbabilitiesScale[WordSequence.size() - Order + 1]);
  } else {
    return log(0);
  }
}

int HPYLM::GetContextId(const std::vector< int > &ContextSequence) const
{
//   PrintDebugHeader << ": Getting contextid for context sequence ";
//   for(const_iiterator it = ContextSequence.begin(); it != ContextSequence.end(); it++) {
//     std::cout << *it << " ";
//   }
//   std::cout << std::endl;
//   PrintDebugHeader << ": Recursively getting contextid for context sequence [ ";
  return GetContextIdRecursively(ContextSequence.end(), 1, ContextSequence.size(), RestaurantTree);
}

int HPYLM::GetContextIdRecursively(const const_witerator &Word, unsigned int level, unsigned int ContextLength, const HPYLM::ContextRestaurant &CurrentRestaurant) const
{
  /* check if end of tree is reached */
  if (level <= ContextLength) {
    /* find restaurant for given context */
    ContextsHashmap::const_iterator it = CurrentRestaurant.NextContext.find(*(Word - level));
    if (it != CurrentRestaurant.NextContext.end()) {
//        std::cout << *(Word - level) << " ";
      /* search for longer context */
      return GetContextIdRecursively(Word, level + 1, ContextLength, *(it->second));
    }
  }
  /* return contextid */
//   std::cout << "]" << std::endl;
  return CurrentRestaurant.ContextId;
}

void HPYLM::ResampleHyperParameters()
{
  PosteriorParameters UpdatedPosteriorParameters(Order);
  GetUpdatedPosteriorParametersRecursively(1, RestaurantTree, &UpdatedPosteriorParameters);
  for (unsigned int level = 0; level < Order; level++) {
    double u = GammaDistribution(RandomGenerator, std::gamma_distribution<double>::param_type(UpdatedPosteriorParameters.a[level], 1));
    double v = GammaDistribution(RandomGenerator, std::gamma_distribution<double>::param_type(UpdatedPosteriorParameters.b[level], 1));
    Parameters.Discount[level] = u / (u + v);
    Parameters.Concentration[level] = GammaDistribution(RandomGenerator, std::gamma_distribution<double>::param_type(UpdatedPosteriorParameters.alpha[level], 1 / UpdatedPosteriorParameters.beta[level]));
//     PrintDebugHeader << ": Resampled Discount[" << level + 1 << "]" << Discount[level] << " from Beta(" << UpdatedPosteriorParameters.a[level] << "," << UpdatedPosteriorParameters.b[level] << ")" << std::endl;
//     PrintDebugHeader << ": Resampled Concentration[" << level + 1 << "]" << Concentration[level] << " from Gamma(" << UpdatedPosteriorParameters.alpha[level] << "," << 1/UpdatedPosteriorParameters.beta[level] << ")" << std::endl;
  }
}

void HPYLM::GetUpdatedPosteriorParametersRecursively(unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, HPYLM::PosteriorParameters *UpdatedPosteriorParameters) const
{
  for (ContextsHashmap::const_iterator NextContextIterator = CurrentRestaurant.NextContext.begin(); NextContextIterator != CurrentRestaurant.NextContext.end(); ++NextContextIterator) {
    GetUpdatedPosteriorParametersRecursively(level + 1, *(NextContextIterator->second), UpdatedPosteriorParameters);
  }
  UpdatedPosteriorParameters->a[level - 1] += CurrentRestaurant.ThisRestaurant.GetOneMinusYuiSum();
  UpdatedPosteriorParameters->b[level - 1] += CurrentRestaurant.ThisRestaurant.GetOneMinusZuwkjSum();
  UpdatedPosteriorParameters->alpha[level - 1] += CurrentRestaurant.ThisRestaurant.GetYuiSum();
  UpdatedPosteriorParameters->beta[level - 1] -= CurrentRestaurant.ThisRestaurant.GetLogXu();
}

std::vector< int > HPYLM::GetTotalWordcountPerLevel() const
{
  std::vector<int> TotalWordcountPerLevel(Order, 0);
  GetTotalWordcountPerLevelRecursively(1, RestaurantTree, &TotalWordcountPerLevel);
  return TotalWordcountPerLevel;
}

void HPYLM::GetTotalWordcountPerLevelRecursively(unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, std::vector< int > *TotalWordcountPerLevel) const
{
  for (ContextsHashmap::const_iterator NextContextIterator = CurrentRestaurant.NextContext.begin(); NextContextIterator != CurrentRestaurant.NextContext.end(); ++NextContextIterator) {
    GetTotalWordcountPerLevelRecursively(level + 1, *(NextContextIterator->second), TotalWordcountPerLevel);
  }
  (*TotalWordcountPerLevel)[level - 1] += CurrentRestaurant.ThisRestaurant.GetTotalWordCount();
}

std::vector< int > HPYLM::GetTotalTablecountPerLevel() const
{
  std::vector<int> TotalTablecountPerLevel(Order, 0);
  GetTotalTablecountPerLevelRecursively(1, RestaurantTree, &TotalTablecountPerLevel);
  return TotalTablecountPerLevel;
}

void HPYLM::GetTotalTablecountPerLevelRecursively(unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, std::vector< int > *TotalTablecountPerLevel) const
{
  for (ContextsHashmap::const_iterator NextContextIterator = CurrentRestaurant.NextContext.begin(); NextContextIterator != CurrentRestaurant.NextContext.end(); ++NextContextIterator) {
    GetTotalTablecountPerLevelRecursively(level + 1, *(NextContextIterator->second), TotalTablecountPerLevel);
  }
  (*TotalTablecountPerLevel)[level - 1] += CurrentRestaurant.ThisRestaurant.GetTotalTableCount();
}

std::vector<int> HPYLM::GetTotalContextCountPerLevel() const
{
  std::vector<int> TotalContextcountPerLevel(Order, 0);
  GetTotalContextcountPerLevelRecursively(1, RestaurantTree, &TotalContextcountPerLevel);
  return TotalContextcountPerLevel;
}

void HPYLM::GetTotalContextcountPerLevelRecursively(unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, std::vector< int > *TotalContextcountPerLevel) const
{
  for (ContextsHashmap::const_iterator NextContextIterator = CurrentRestaurant.NextContext.begin(); NextContextIterator != CurrentRestaurant.NextContext.end(); ++NextContextIterator) {
    GetTotalContextcountPerLevelRecursively(level + 1, *(NextContextIterator->second), TotalContextcountPerLevel);
  }
  (*TotalContextcountPerLevel)[level - 1]++;
//   PrintDebugHeader << "Visited context id : " << CurrentRestaurant.ContextId << " at level " << level << " count " << (*TotalContextcountPerLevel)[level - 1] << std::endl;
}

const HPYLM::HPYLMParameters &HPYLM::GetHPYLMParameters() const
{
  return Parameters;
}

HPYLM::ContextToContextTransitions HPYLM::GetTransitions(int ContextId, int SentEndSymbolId, const std::vector<bool> &ActiveWords) const
{
  ContextToContextTransitions Transitions;

  /* find context for context id */
  ContextsHashmap::const_iterator it = ContextIdToContext.find(ContextId);
  if (it == ContextIdToContext.end()) {
    return Transitions;
  }

  /* get words in given context */
  Transitions.Words = it->second->ThisRestaurant.GetWords(ActiveWords);

  /* extract context sequence and remove last word, if we have the longest context */
  std::vector<int> ContextSequence;
  if (Order > 1) {
    ContextSequence = it->second->ContextSequence;
    if (ContextSequence.size() == (Order - 1)) {
      ContextSequence.erase(ContextSequence.begin());
    }
  }

  /* generate next context and get context id */
  ContextSequence.resize(ContextSequence.size() + 1);
  Transitions.NextContextIds.reserve(Transitions.Words.size() + 1);
  for (std::vector<int>::const_iterator Word = Transitions.Words.begin(); Word != Transitions.Words.end(); ++Word) {
    if (*Word != SentEndSymbolId) {
      ContextSequence[ContextSequence.size() - 1] = *Word;
      Transitions.NextContextIds.push_back(GetContextId(ContextSequence));
    } else {
      Transitions.NextContextIds.push_back(NextUnusedContextId);
      Transitions.HasTransitionToSentEnd = true;
    }
  }

  /* add fallback transitions */
  if (it->second->ContextId > 0) {
    Transitions.Words.push_back(PHI);
    Transitions.NextContextIds.push_back(it->second->PreviousContext->ContextId);
  }

  return Transitions;
}

int HPYLM::GetNextUnusedContextId() const
{
  return NextUnusedContextId;
}

const std::vector<int> &HPYLM::GetContextSequence(int ContextId) const
{
  ContextsHashmap::const_iterator it = ContextIdToContext.find(ContextId);
  if (it != ContextIdToContext.end()) {
    return it->second->ContextSequence;
  } else {
    return RestaurantTree.ContextSequence;
  }
}

int HPYLM::GenerateWord(const std::vector< int > &ContextSequence, const std::vector< int > &Words, const std::vector< double > &BaseProbabilities, bool SampleFromBase) const
{
  int WordId = GenerateWordRecursively(ContextSequence.end(), Words, 1, ContextSequence.size(), RestaurantTree, BaseProbabilities);
  if ((WordId == PHI) && SampleFromBase) {
    return Words.at(DiscreteDistribution(RandomGenerator, std::discrete_distribution<unsigned int>::param_type(BaseProbabilities.begin(), BaseProbabilities.end())));
  } else {
    return WordId;
  }
}

int HPYLM::GenerateWordRecursively(const const_witerator &Word, const std::vector< int > &Words, unsigned int level, unsigned int ContextLenght, const HPYLM::ContextRestaurant &CurrentRestaurant, const std::vector< double > &BaseProbabilities) const
{
  /* adjust base probabilities for the words acording to current context */
  std::vector<double> WordProbabilities(BaseProbabilities);
  CurrentRestaurant.ThisRestaurant.WordVectorProbability(Words, &WordProbabilities);

  /* used for word generation */
  int WordId;
  bool EndOfTree = true;

  /* check if end of tree is reached */
  if (level <= ContextLenght) {
    /* find restaurant for given context */
    ContextsHashmap::const_iterator it = CurrentRestaurant.NextContext.find(*(Word - level));
    if (it != CurrentRestaurant.NextContext.end()) {
      /* adjust base probabilities for the words according to found context */
      EndOfTree = false;
      WordId = GenerateWordRecursively(Word, Words, level + 1, ContextLenght, *(it->second), WordProbabilities);
    }
  }

  if (EndOfTree || WordId == PHI) {
    return Words.at(DiscreteDistribution(RandomGenerator, std::discrete_distribution<unsigned int>::param_type(WordProbabilities.begin(), WordProbabilities.end())));
  } else {
    return WordId;
  }
}

int HPYLM::GetBaseTablesPerWord(int WordId) const
{
  return RestaurantTree.ThisRestaurant.GetTablesPerWord(WordId);
}

void HPYLM::SetBaseProbabilitiesScale(const std::vector< double > &BaseProbabilitiesScale_)
{
  BaseProbabilitiesScale = BaseProbabilitiesScale_;
}

const std::vector< double > &HPYLM::GetBaseProbabilitiesScale() const
{
  return BaseProbabilitiesScale;
}

void HPYLM::SetConcentration(int Level, double Value)
{
  Parameters.Concentration[Level] = Value;
}

void HPYLM::SetDiscount(int Level, double Value)
{
  Parameters.Discount[Level] = Value;
}

HPYLM::ContextRestaurant::ContextRestaurant(const double &Discount_, const double &Concentration_, ContextRestaurant *PreviousContext_, int ContextId_, const std::vector< int > &ContextSequence_) :
  ContextId(ContextId_),
  ContextSequence(ContextSequence_),
  NextContext(),
  PreviousContext(PreviousContext_),
  ThisRestaurant(Discount_, Concentration_)
{
  NextContext.set_empty_key(EMPTY);
  NextContext.set_deleted_key(DELETED);
}

HPYLM::PosteriorParameters::PosteriorParameters(int order_) :
  a(order_, 1),
  b(order_, 1),
  alpha(order_, 1),
  beta(order_, 1)
{
}

HPYLM::HPYLMParameters::HPYLMParameters(unsigned int Order_, double Discount_, double Concentration_) :
  Discount(Order_, Discount_),
  Concentration(Order_, Concentration_)
{
}

HPYLM::ContextToContextTransitions::ContextToContextTransitions() :
  Words(),
  NextContextIds(),
  Probabilities(),
  HasTransitionToSentEnd(false)
{
}
