// ----------------------------------------------------------------------------
/**
   File: WordLengthProbCalculator.cpp
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
#include "WordLengthProbCalculator.hpp"
#include "NHPYLM/NHPYLM.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <iomanip>

double WordLengthProbCalculator::factorial(double n) const
{
  return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

double WordLengthProbCalculator::WordLengthProbability(int WordLength) const
{
  if (WordLength < 2) {
    return 0;
  } else {
    return exp(-(MeanObservedLength - 2)) * (pow(MeanObservedLength - 2, WordLength - 2) / factorial(WordLength - 2));
  }
}

double WordLengthProbCalculator::GetScaledWordLengthProbability(int WordLength) const
{
  return WordLengthProbability(WordLength) / GeneratedLengthDistribution.at(WordLength);
}

void WordLengthProbCalculator::SetGeneratedLengthDistribution(const std::vector< double > &GeneratedLengthDistribution_)
{
  GeneratedLengthDistribution = GeneratedLengthDistribution_;
  UpdateScaledWordLengthProbabilityVector = true;
}

void WordLengthProbCalculator::SetObservedLengthDistribution(const std::vector< double > &ObervedLengthDistribution_)
{
  ObervedLengthDistribution = ObervedLengthDistribution_;
  MeanObservedLength = 0;
  double NumObservations = 0;
  int ObservedLength = 0;
  for (const double & ObservedLengthProbability : ObervedLengthDistribution) {
    MeanObservedLength += ObservedLengthProbability * ObservedLength;
    NumObservations += ObservedLengthProbability;
    ObservedLength++;
  }
  MeanObservedLength /= NumObservations;
  UpdateScaledWordLengthProbabilityVector = true;
}

void WordLengthProbCalculator::SetDesiredLengthMean(double DesiredLengthMean_)
{
  MeanObservedLength = DesiredLengthMean_;
  UpdateScaledWordLengthProbabilityVector = true;
}

const std::vector<double> &WordLengthProbCalculator::GetScaledWordLengthProbabilityVector()
{
  if (UpdateScaledWordLengthProbabilityVector) {
    ScaledWordLengthProbabilityVector.resize(GeneratedLengthDistribution.size(), 0);
    for (unsigned int WordLength = 0; WordLength < GeneratedLengthDistribution.size(); WordLength++) {
      ScaledWordLengthProbabilityVector.at(WordLength) = WordLengthProbability(WordLength) / GeneratedLengthDistribution.at(WordLength);
      if ((ScaledWordLengthProbabilityVector.at(WordLength) == std::numeric_limits<double>::infinity()) || std::isnan(ScaledWordLengthProbabilityVector.at(WordLength))) {
        ScaledWordLengthProbabilityVector.at(WordLength) = 0;
      }
      std::cout << "s(" << WordLength << ")=" << WordLengthProbability(WordLength) << "/" << GeneratedLengthDistribution.at(WordLength) << "=" << ScaledWordLengthProbabilityVector.at(WordLength) << std::endl;
    }
    UpdateScaledWordLengthProbabilityVector = false;
  }

  return ScaledWordLengthProbabilityVector;
}


void WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
  NHPYLM *LanguageModel,
  int WordLengthModulation
)
{
  std::vector<int> WordLengths(LanguageModel->GetWordLengthVector());

  int NumWords = 0;
  double MeanWordLength = 0;
  std::vector<double> ObservedWordLengthProbabilities;
  for (unsigned int WordId = 0; WordId < WordLengths.size(); ++WordId) {
    if (WordLengths[WordId] > 0) {
      WordLengths[WordId]++;
      int TablesPerWord = LanguageModel->GetWHPYLBaseTablesPerWord(WordId);
      NumWords += TablesPerWord;
      MeanWordLength += TablesPerWord * WordLengths[WordId];
      ObservedWordLengthProbabilities.resize(
        std::max(
          static_cast<size_t>(WordLengths[WordId] + 1),
          ObservedWordLengthProbabilities.size()
        )
      );
      ObservedWordLengthProbabilities.at(WordLengths[WordId]) += TablesPerWord;
    }
  }

  int WordLength = 0;
  for (double & WordLengthProbability : ObservedWordLengthProbabilities) {
    WordLengthProbability /= NumWords;
    std::cout << std::setprecision(4) << std::fixed << "Word length: "
              << WordLength++ << ", word length probability: "
              << WordLengthProbability << std::endl;
  }
  MeanWordLength /= NumWords;
  std::cout << "Mean word length: " << MeanWordLength << ", number of word: "
            << NumWords << std::endl << std::endl;

  if (WordLengthModulation > -1) {
    std::vector<double> GeneratedWordLengthProbabilities;
    LanguageModel->Generate("CHPYLM", 100000, -1,
                            &GeneratedWordLengthProbabilities);

    WordLengthProbCalculator WLPVectorCalculator;
    WLPVectorCalculator.SetGeneratedLengthDistribution(
      GeneratedWordLengthProbabilities);
    if (WordLengthModulation == 0) {
      WLPVectorCalculator.SetObservedLengthDistribution(
        ObservedWordLengthProbabilities);
    } else {
//       std::cout << "Using: " << Params.WordLengthModulation << std::endl;
      WLPVectorCalculator.SetDesiredLengthMean(WordLengthModulation);
    }
    LanguageModel->SetWHPYLMBaseProbabilitiesScale(
      WLPVectorCalculator.GetScaledWordLengthProbabilityVector());
  }
}
