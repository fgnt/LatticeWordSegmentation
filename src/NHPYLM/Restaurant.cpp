// ----------------------------------------------------------------------------
/**
   File: Restaurant.cpp
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
#include "Restaurant.hpp"

std::default_random_engine Restaurant::RandomGenerator(std::chrono::system_clock::now().time_since_epoch().count());
std::discrete_distribution<unsigned int> Restaurant::TableDistribution;
std::vector<double> Restaurant::TableProbabilities;
std::bernoulli_distribution Restaurant::BernoulliDistribution;
std::gamma_distribution<double> Restaurant::GammaDistribution;

Restaurant::Restaurant(const double &Discount_, const double &Concentration_) :
  Words(),
  TotalWordCount(0),
  TotalTableCount(0),
  Discount(Discount_),
  Concentration(Concentration_)
{
  Words.set_empty_key(EMPTY);
  Words.set_deleted_key(DELETED);
}

bool Restaurant::IncrementWordCount(int Word, double BaseProbability)
{
  /* find or create table group to add word to */
  WordsHashmap::iterator it = Words.find(Word);
  if (it == Words.end()) {
//     PrintDebugHeader << ": Creating new tablegroup for word/character id " << Word << std::endl;
    it = Words.insert(std::make_pair(Word, WordTableGroup())).first;
  }
  WordTableGroup &TableGroup = it->second;

  /* sample table for word */
  unsigned int SampledTable;
  if (TableGroup.GroupTableCount > 0) {
    /* adjust buffer for probabilities used for samling */
    if (TableGroup.GroupTableCount >= TableProbabilities.size()) {
      TableProbabilities.resize(TableGroup.GroupTableCount + 1);
    }

    /* probabilites for existing tables */
    for (unsigned int i = 0; i < TableGroup.GroupTableCount; i++) {
      TableProbabilities[i] = TableGroup.TableWordcount[i] - Discount;
    }
    /* probabilites for new table */
    TableProbabilities[TableGroup.GroupTableCount] = (Concentration + Discount * TotalTableCount) * BaseProbability;

    /* sample Table */
    SampledTable = TableDistribution(RandomGenerator, std::discrete_distribution<unsigned int>::param_type(TableProbabilities.begin(), TableProbabilities.begin() + TableGroup.GroupTableCount + 1));

    /* debug */
//     PrintDebugHeader << ": Sampling from table probabilties: |";
//     for(std::vector<double>::iterator ProbIt = TableProbabilities.begin(); ProbIt != TableProbabilities.begin() + TableGroup.GroupTableCount + 1; ++ProbIt) {
//       std::cout << *ProbIt << "|";
//     }
//     std::cout << std::endl;
  } else {
    SampledTable = 0;
  }

  /* increment counts and add tables, if needed */
  TableGroup.Wordcount++;
  TotalWordCount++;
  if (SampledTable == TableGroup.GroupTableCount) {
//     PrintDebugHeader << ": Creating table " << SampledTable << " for word/character id " << Word << std::endl;
    TableGroup.TableWordcount.push_back(1);
    TableGroup.GroupTableCount++;
    TotalTableCount++;
    return true;
  } else {
//     PrintDebugHeader << ": Incrementing existing table " << SampledTable << " for word/character id " << Word << std::endl;
    TableGroup.TableWordcount[SampledTable]++;
    return false;
  }
}

WordRemoveStatus Restaurant::DecrementWordCount(int Word)
{
  /* find table group for word to remove */
  WordsHashmap::iterator it = Words.find(Word);
  WordTableGroup &TableGroup = it->second;

  /* sample table to remove word from */
  WordRemoveStatus Removed;
  unsigned int SampledTable = TableDistribution(RandomGenerator, std::discrete_distribution<unsigned int>::param_type(TableGroup.TableWordcount.begin(), TableGroup.TableWordcount.end()));
  TableGroup.TableWordcount[SampledTable]--;
  if (TableGroup.TableWordcount[SampledTable] == 0) {
//     PrintDebugHeader << ": Removing table " << SampledTable << " for word/character " << Word << std::endl;
    TotalTableCount--;
    TableGroup.GroupTableCount--;
    TableGroup.TableWordcount.erase(TableGroup.TableWordcount.begin() + SampledTable);
    Removed = TABLE;
  } else {
//     PrintDebugHeader << ": Decrementing existing table " << SampledTable << " for word/character " << Word << std::endl;
    Removed = NONEREMOVED;
  }
  TotalWordCount--;
  TableGroup.Wordcount--;

  /* remove table group for word if empty */
  if (TableGroup.Wordcount == 0) {
//     PrintDebugHeader << ": Removing tablegroup for word/character " << Word << std::endl;
    Words.erase(it);
    Removed = TABLE_WORD;
    if (TotalWordCount == 0) {
      Removed = TABLE_WORD_RESTAURANT;
    }
  }
  return Removed;
}

double Restaurant::WordProbability(int Word, double BaseProbability) const
{
  /* check if word is present in current context, else return scaled base probability */
  WordsHashmap::const_iterator it = Words.find(Word);
  if (it == Words.end()) {
    if (Word != PHI) {
      return BaseProbability * (Concentration + Discount * TotalTableCount) / (Concentration + TotalWordCount);
    } else {
      return (Concentration + Discount * TotalTableCount) / (Concentration + TotalWordCount);
    }
  } else {
    const WordTableGroup &TableGroup = it->second;
    return (TableGroup.Wordcount - Discount * TableGroup.GroupTableCount + BaseProbability * (Concentration + Discount * TotalTableCount)) / (Concentration + TotalWordCount);
  }
}

void Restaurant::WordVectorProbability(const std::vector< int > &WordVector, std::vector< double > *BaseProbabilities) const
{
  for (unsigned int IdxWord = 0; IdxWord < WordVector.size(); IdxWord++) {
    /* check if word is present in current context, else return scaled base probability */
    WordsHashmap::const_iterator it = Words.find(WordVector[IdxWord]);
    if (it == Words.end()) {
      if (WordVector[IdxWord] != PHI) {
        (*BaseProbabilities)[IdxWord] = (*BaseProbabilities)[IdxWord] * (Concentration + Discount * TotalTableCount) / (Concentration + TotalWordCount);
      } else {
        (*BaseProbabilities)[IdxWord] = (Concentration + Discount * TotalTableCount) / (Concentration + TotalWordCount);
      }
    } else {
      const WordTableGroup &TableGroup = it->second;
      (*BaseProbabilities)[IdxWord] = (TableGroup.Wordcount - Discount * TableGroup.GroupTableCount + (*BaseProbabilities)[IdxWord] * (Concentration + Discount * TotalTableCount)) / (Concentration + TotalWordCount);
    }
  }
}

unsigned int Restaurant::GetOneMinusYuiSum() const
{
  unsigned int OneMinusYuiSum = 0;
  for (unsigned int i = 1; i < TotalTableCount; i++) {
    if (!BernoulliDistribution(RandomGenerator, std::bernoulli_distribution::param_type(Concentration / (Concentration + Discount * i)))) {
      OneMinusYuiSum++;
    };
  }
  return OneMinusYuiSum;
}

unsigned int Restaurant::GetOneMinusZuwkjSum() const
{
  unsigned int OneMinusZuwkjSum = 0;
  for (WordsHashmap::const_iterator it = Words.begin(); it != Words.end(); ++it) {
    for (unsigned int k = 0; k < it->second.TableWordcount.size(); k++) {
      for (unsigned int j = 1; j < it->second.TableWordcount[k]; j++) {
        if (!BernoulliDistribution(RandomGenerator, std::bernoulli_distribution::param_type((j - 1) / (j - Discount)))) {
          OneMinusZuwkjSum++;
        }
      }
    }
  }
  return OneMinusZuwkjSum;
}

double Restaurant::GetLogXu() const
{
  if (TotalTableCount > 1) {
    double u = GammaDistribution(RandomGenerator, std::gamma_distribution<double>::param_type(Concentration + 1, 1));
    double v = GammaDistribution(RandomGenerator, std::gamma_distribution<double>::param_type(TotalWordCount - 1, 1));
    return log(u / (u + v));
  } else {
    return 0;
  }
}

double Restaurant::GetYuiSum() const
{
  double YuiSum = 0;
  for (unsigned int i = 1; i < TotalTableCount; i++) {
    if (BernoulliDistribution(RandomGenerator, std::bernoulli_distribution::param_type(Concentration / (Concentration + Discount * i)))) {
      YuiSum++;
    }
  }
  return YuiSum;
}

std::vector<int> Restaurant::GetWords(const std::vector<bool> &ActiveWords) const
{
  std::vector<int> WordsInContext;
  WordsInContext.reserve(Words.size());
  for (WordsHashmap::const_iterator Word = Words.begin(); Word != Words.end(); ++Word) {
    if (ActiveWords.empty() || ActiveWords[Word->first]) {
      WordsInContext.push_back(Word->first);
    }
  }
  return WordsInContext;
}

double Restaurant::GetTotalWordCount() const
{
  return TotalWordCount;
}

double Restaurant::GetTotalTableCount() const
{
  return TotalTableCount;
}

int Restaurant::GetTablesPerWord(int WordId) const
{
//   std::cout << "GetTablesPerWord(" << WordId << ") = ";
  WordsHashmap::const_iterator it = Words.find(WordId);
  if (it != Words.end()) {
//     std::cout << it->second.GroupTableCount << std::endl;
    return it->second.GroupTableCount;
  } else {
//     std::cout << 0 << " (not found, Words.size() = " << Words.size() << std::endl;
    return 0;
  }
}

Restaurant::WordTableGroup::WordTableGroup() :
  Wordcount(0),
  TableWordcount(),
  GroupTableCount(0)
{
}