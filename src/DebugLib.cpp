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

using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::cerr;

void DebugLib::PrintTransitions(const ContextToContextTransitions &Transitions, unsigned int CurrentContextId, std::vector< bool > &VisitedContextIds, const NHPYLM &LanguageModel, int SentEndWordId)
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

void DebugLib::WriteOpenFSTLattice(const fst::VectorFst< fst::LogArc > &fst, const string &FileName)
{
  CreateDirectoryRecursively(FileName);
  fst.Write(FileName.c_str());
}

void DebugLib::WriteSymbols(const string &fileName, const vector< string > &words, SymbolWriteModes SymbolWriteMode)
{
  std::cout << "  Writing symbols to " << fileName << std::endl;
  std::ofstream symOut(fileName.c_str());
  for (unsigned i = 0; i < words.size(); i++) {
    if (SymbolWriteMode == NAMESANDIDS) {
      symOut << words.at(i) << "[" << i << "]\t" << i << endl;
    } else {
      symOut << words.at(i) << "\t" << i << endl;
    }
  }
  symOut.close();
}

void DebugLib::CreateDirectoryRecursively(const std::string &DirectoryName, size_t PathSeperatorPosition)
{
  PathSeperatorPosition = DirectoryName.find_last_of('/', PathSeperatorPosition);
  if ((PathSeperatorPosition != string::npos) && (access(DirectoryName.substr(0, PathSeperatorPosition).c_str(), 0) != 0)) {
    CreateDirectoryRecursively(DirectoryName, PathSeperatorPosition - 1);
    mkdir(DirectoryName.substr(0, PathSeperatorPosition).c_str(), 0755);
  }
}

void DebugLib::PrintFST(const string &pFileName, const vector< string > &words, const fst::VectorFst< fst::LogArc > &fst, bool printPDF, SymbolWriteModes SymbolWriteMode)
{
  WriteOpenFSTLattice(fst, pFileName);

  string fileNameSymI = pFileName + "_sym_i.txt";
  string fileNameSymO = pFileName + "_sym_o.txt";
  if (SymbolWriteMode != NONE) {
    WriteSymbols(pFileName + "_sym_i.txt", words, SymbolWriteMode);
    WriteSymbols(pFileName + "_sym_o.txt", words, SymbolWriteMode);
  }

  string fileNameBat = pFileName + "_draw.sh";
  FILE *batOut = std::fopen(fileNameBat.c_str(), "w");
  if (!batOut) {
    exit(10);
  }

  std::fprintf(batOut, "#!/bin/bash\n");
  if (SymbolWriteMode != NONE) {
    std::fprintf(batOut, "fstdraw --portrait=true --title=\"%s\" --isymbols=%s --osymbols=%s %s %s.dot\n", pFileName.c_str(), fileNameSymI.c_str(), fileNameSymO.c_str(), pFileName.c_str(), pFileName.c_str());
  } else {
    std::fprintf(batOut, "fstdraw --portrait=true --title=\"%s\" %s %s.dot\n", pFileName.c_str(), pFileName.c_str(), pFileName.c_str());
  }
  std::fprintf(batOut, "dot -Gcharset=latin1 -Gdpi=1200 -Tpdf %s.dot >%s.pdf", pFileName.c_str(), pFileName.c_str());
  std::fclose(batOut);
  chmod(fileNameBat.c_str(), 0744);

  std::printf("Printing %s: ", pFileName.c_str());
  if (printPDF) {
    int ret1 = system(("./" + fileNameBat).c_str());
    int ret2 = system(("rm " + fileNameBat).c_str());
    int ret3 = system(("rm " + fileNameSymI).c_str());
    int ret4 = system(("rm " + fileNameSymO).c_str());
    int ret5 = system(("rm " + pFileName).c_str());
    int ret6 = system(("rm " + pFileName + ".dot").c_str());
    std::printf("%i %i %i %i %i %i\n", ret1, ret2, ret3, ret4, ret5, ret6);
  }
}

void DebugLib::PrintSentencesToFile(const std::string &FileName, const std::vector<std::vector<int> > &Sentences, const std::vector<std::string> &Id2CharacterSequence)
{
  CreateDirectoryRecursively(FileName);

  std::ofstream myfile;
  myfile.open(FileName);
  for (const auto &Sentence : Sentences) {
//     DebugLib::PrintSentence(sit->begin(), sit->size(), Id2CharacterSequence);
    for (const auto &WordId : Sentence) {
      myfile << Id2CharacterSequence.at(WordId) << " ";
    }
    myfile << std::endl;
  }
  myfile.close();
}

void DebugLib::PrintTimedSentencesToFile(
  const std::string &FileName,
  const std::vector<std::vector<ArcInfo> > &TimedSentences,
  const std::vector<std::string> &Id2CharacterSequence,
  const std::vector<std::string> &InputFileNames)
{
  CreateDirectoryRecursively(FileName);

  std::ofstream myfile;
  myfile.open(FileName);
  auto InputFilenName = InputFileNames.begin();
  for (const auto &TimedSentence : TimedSentences) {
    for (const auto &WordId : TimedSentence) {
      myfile << *InputFilenName << " "
             << Id2CharacterSequence.at(WordId.label) << " "
             << WordId.start << " "
             << WordId.end << std::endl;
    }
    InputFilenName++;
  }
  myfile.close();
}

void DebugLib::GenerateSentencesOfWordsFromCharLM(
  const NHPYLM &LanguageModel)
{
  // Id2 Character sequence vector for printing
  std::vector<std::string> Id2CharacterSequenceVector(
    LanguageModel.GetId2CharacterSequenceVector());

  // generate sentences of words from the Character language model
  std::vector<std::vector<int> > GeneratedSentencesFromCHPYLM = LanguageModel.Generate("CHPYLM", 100000, -1, NULL);
  for (auto &Sentence : GeneratedSentencesFromCHPYLM) {
    for (auto &Character : Sentence) {
      if (Character == UNKEND_SYMBOLID) {
        std::cout << " ";
      } else if (Character == SENTEND_SYMBOLID) {
        std::cout << std::endl;
      } else {
//         std::cout << Id2CharacterSequenceVector.at(Character) << "[" << Character << "] ";
        std::cout << Id2CharacterSequenceVector.at(Character);
      }
    }
    std::cout << std::endl;
  }
}

void DebugLib::GenerateSentencesOfWordsFromWordLM(
  const NHPYLM &LanguageModel, int SentEndWordId)
{
  // Id2 word sequence vector for printing
  std::vector<std::string> Id2WordSequenceVector(LanguageModel.GetId2CharacterSequenceVector());

  // generate sentences of words from the word language model
  std::vector<std::vector<int> > GeneratedSentencesFromWHPYLM = LanguageModel.Generate("WHPYLM", 10000, SentEndWordId, NULL);
  for (auto &Sentence : GeneratedSentencesFromWHPYLM) {
    for (auto &Word : Sentence) {
      if (Word == SentEndWordId) {
        std::cout << std::endl;
      } else {
//         std::cout << Id2CharacterSequenceVector.at(Word) << "[" << Word << "] ";
        std::cout << Id2WordSequenceVector.at(Word) << " ";
      }
    }
    std::cout << std::endl;
  }
}
