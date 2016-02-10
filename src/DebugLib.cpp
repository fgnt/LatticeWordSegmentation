// ----------------------------------------------------------------------------
/**
   File: DebugLib.cpp
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
#include "DebugLib.hpp"
#include <iomanip>
#include <iostream>

void DebugLib::PrintTransitions(const HPYLM::ContextToContextTransitions &Transitions, unsigned int CurrentContextId, std::vector< bool > &VisitedContextIds, const NHPYLM &LanguageModel, int SentEndWordId)
{
  if (VisitedContextIds.size() <= CurrentContextId) {
    VisitedContextIds.resize(CurrentContextId + 1, false);
  }
  VisitedContextIds[CurrentContextId] = true;

  std::cout << "Transition from " << CurrentContextId << ":";
  for (unsigned int TransitionIndex = 0; TransitionIndex < Transitions.Words.size(); TransitionIndex++) {
    std::cout << " [" << Transitions.Words.at(TransitionIndex) << "," << Transitions.NextContextIds.at(TransitionIndex) << "," << Transitions.Probabilities.at(TransitionIndex) << "]";
  }
  std::cout << "Has transition to WordEnd: " << Transitions.HasTransitionToSentEnd << std::endl;
  for (unsigned int TransitionIndex = 0; TransitionIndex < Transitions.Words.size(); TransitionIndex++) {
    if ((static_cast<int>(VisitedContextIds.size()) <= Transitions.NextContextIds.at(TransitionIndex)) || !VisitedContextIds[Transitions.NextContextIds.at(TransitionIndex)]) {
      PrintTransitions(LanguageModel.GetTransitions(Transitions.NextContextIds.at(TransitionIndex), SentEndWordId, std::vector<bool>(LanguageModel.GetMaxNumWords())), Transitions.NextContextIds.at(TransitionIndex), VisitedContextIds, LanguageModel, SentEndWordId);
    }
  }
}

void DebugLib::PrintSentence(const const_witerator &SentenceBegin, int NumWords, const std::vector<std::string> &Id2CharacterSequence)
{
  for (const_witerator Word = SentenceBegin; Word != SentenceBegin + NumWords; ++Word) {
    std::cout << Id2CharacterSequence.at(*Word) << "[" << *Word << "] ";
  }
  std::cout << std::endl;
}

void DebugLib::PrintEditDistanceStatistics(const std::vector< int > &SegmentationInsDelSubCorrNFoundNRef, const std::string &Description, const std::string &Name)
{
  // output some error statistics (segmentation)
  int Insertions = SegmentationInsDelSubCorrNFoundNRef[0];
  int Deletions = SegmentationInsDelSubCorrNFoundNRef[1];
  int Substitutions = SegmentationInsDelSubCorrNFoundNRef[2];
  int SegmentationCorrect = SegmentationInsDelSubCorrNFoundNRef[3];
  int SegmentationNumFound = SegmentationInsDelSubCorrNFoundNRef[4];
  int SegmentationNumRef = SegmentationInsDelSubCorrNFoundNRef[5];
  double WER = (Insertions + Deletions + Substitutions) / static_cast<double>(SegmentationNumRef);
  double SegmentationPrecision = SegmentationCorrect / static_cast<double>(SegmentationNumFound);
  double SegmentationRecall = SegmentationCorrect / static_cast<double>(SegmentationNumRef);
  double SegmentationFScore = 2 * (SegmentationPrecision * SegmentationRecall) / (SegmentationPrecision + SegmentationRecall);
  std::cout << std::setprecision(2) << std::fixed << " " << Description << ":" << std::endl;
  std::cout << "  " << Name << ": " << 100 * WER << " %,"
            << " Precision: " << 100 * SegmentationPrecision << " %,"
            << " Recall: " << 100 * SegmentationRecall << " %,"
            << " F-score: " << 100 * SegmentationFScore << " %"
            << std::endl;
  std::cout << "  Ins: " << Insertions << ","
            << " Del: " << Deletions << ","
            << " Sub: " << Substitutions << ","
            << " Corr: " << SegmentationCorrect << ","
            << " NFound: " << SegmentationNumFound << ","
            << " NRef: " << SegmentationNumRef
            << std::endl << std::endl;
}

void DebugLib::PrintLexiconStatistics(const std::vector< int > &LexiconCorrNFoundNRef)
{
  // output some error statistics (lexicon)
  int LexiconCorrect = LexiconCorrNFoundNRef[0];
  int LexiconNumFound = LexiconCorrNFoundNRef[1];;
  int LexiconNumRef = LexiconCorrNFoundNRef[2];;
  double LexiconPrecision = LexiconCorrect / static_cast<double>(LexiconNumFound);
  double LexiconRecall = LexiconCorrect / static_cast<double>(LexiconNumRef);
  double LexiconFScore = 2 * (LexiconPrecision * LexiconRecall) / (LexiconPrecision + LexiconRecall);
  std::cout << std::setprecision(2) << std::fixed << " Lexicon:" << std::endl;
  std::cout << "  Precision: " << 100 * LexiconPrecision << " %,"
            << " Recall: " << 100 * LexiconRecall << " %,"
            << " F-score: " << 100 * LexiconFScore << " %"
            << std::endl;
  std::cout << "  Corr: " << LexiconCorrect << ","
            << " NFound: " << LexiconNumFound << ","
            << " NRef: " << LexiconNumRef
            << std::endl << std::endl;
}

void DebugLib::PrintSentencesPerplexity(const std::vector<std::vector<int> > &Sentences, const NHPYLM &LanguageModel)
{
  int WHPYLMContextLenght = LanguageModel.GetWHPYLMOrder() - 1;
  double LoglikelihoodSum = 0;
  int NumWords = 0;
  for (const std::vector<int> &Sentence : Sentences) {
    LoglikelihoodSum += LanguageModel.WordSequenceLoglikelihood(Sentence);
    NumWords += Sentence.size() - WHPYLMContextLenght;
  }
  std::cout << std::setprecision(2) << std::fixed << " Perplexity: " << exp(-LoglikelihoodSum / NumWords) << std::endl << std::endl;
}

void DebugLib::PrintLanguageModelStats(const NHPYLM &LanguageModel)
{
  std::cout << std::setprecision(2) << " CHPYLM statistics:";
  PrintVectorOfInts(LanguageModel.GetTotalCountPerLevelFor("CHPYLM", "Context"), 8, "\n  Contexts:      ", "");
  PrintVectorOfInts(LanguageModel.GetTotalCountPerLevelFor("CHPYLM", "Table"),   8, "\n  Tables:        ", "");
  PrintVectorOfInts(LanguageModel.GetTotalCountPerLevelFor("CHPYLM", "Word"),    8, "\n  Characters:    ", "");
  PrintVectorOfDoubles(LanguageModel.GetNHPYLMParameters().CHPYLMConcentration,  8, "\n  Concentration: ", "");
  PrintVectorOfDoubles(LanguageModel.GetNHPYLMParameters().CHPYLMDiscount,       8, "\n  Discount:      ", "");
  std::cout << "\n WHPYLM statistics:";
  PrintVectorOfInts(LanguageModel.GetTotalCountPerLevelFor("WHPYLM", "Context"), 8, "\n  Contexts:      ", "");
  PrintVectorOfInts(LanguageModel.GetTotalCountPerLevelFor("WHPYLM", "Table"),   8, "\n  Tables:        ", "");
  PrintVectorOfInts(LanguageModel.GetTotalCountPerLevelFor("WHPYLM", "Word"),    8, "\n  Words:         ", "");
  PrintVectorOfDoubles(LanguageModel.GetNHPYLMParameters().WHPYLMConcentration,  8, "\n  Concentration: ", "");
  PrintVectorOfDoubles(LanguageModel.GetNHPYLMParameters().WHPYLMDiscount,       8, "\n  Discount:      ", "");
  std::cout << std::endl << std::endl;
}

void DebugLib::PrintVectorOfInts(const std::vector< int > &VectorOfInts, int Width, const std::string &Description, const std::string &Postfix)
{
  std::cout << Description;
  for (const int & Value : VectorOfInts) {
    std::cout << std::right << std::setw(Width) << Value << Postfix;
  }
}

void DebugLib::PrintVectorOfDoubles(const std::vector< double > &VectorOfDoubles, int Width, const std::string &Description, const std::string &Postfix)
{
  std::cout << Description;
  for (const double & Value : VectorOfDoubles) {
    std::cout << std::right << std::setw(Width) << Value << Postfix;
  }
}
