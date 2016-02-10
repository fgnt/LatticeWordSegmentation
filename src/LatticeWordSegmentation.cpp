// ----------------------------------------------------------------------------
/**
   File: LatticeWordSegmentation.cpp
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
#include <fst/compose.h>
#include <iomanip>
#include <numeric>
#include "LatticeWordSegmentation.hpp"
#include "SampleLib.hpp"
#include "DebugLib.hpp"
#include "EditDistanceCalculator/WERCalculator.hpp"
#include "NHPYLMFst.hpp"
#include "EditDistanceCalculator/PERCalculator.hpp"
#include "EditDistanceCalculator/LPERCalculator.hpp"
#include "WordLengthProbCalculator.hpp"

std::default_random_engine LatticeWordSegmentation::RandomGenerator(std::chrono::system_clock::now().time_since_epoch().count());

LatticeWordSegmentation::LatticeWordSegmentation(int argc, const char **argv) :
  Parser(argc, argv),
  MaxNumThreads(Parser.GetParameters().NoThreads),
  Threads(MaxNumThreads - 1),
  Timer(MaxNumThreads, 4)
{
}


void LatticeWordSegmentation::DoWordSegmentation()
{
  // read the initialization, input and reference data,
  // export data if specified and do some prepocessing
  // of the input lattices (acoustic model scaling and
  // application of word end and sentence end transducers
  if (!Parser.GetParameters().SymbolFile.empty()) {
    Reader.ReadSymbols(Parser.GetParameters().SymbolFile);
  }

  if (Parser.GetParameters().InitLM) {
    Reader.ReadInitTranscription(Parser.GetParameters().InitTranscription);
  }

  Reader.ReadInputFilesFromList(Parser.GetParameters().InputFiles, Parser.GetParameters().LatticeFileType, Parser.GetParameters().PruneFactor, Parser.GetParameters().HTKLMScale);
  if (Parser.GetParameters().ExportLattices) {
    Reader.WriteOpenFSTLattices(Parser.GetParameters().ExportLatticesDirectoryName, "_" + std::to_string(Parser.GetParameters().PruneFactor));
  }

  if (Parser.GetParameters().UseReferenceTranscription) {
    Reader.ReadReferenceTranscription(Parser.GetParameters().ReferenceTranscription);
    if (Parser.GetParameters().CalculateLPER) {
      std::cout << " Calculating LPER with pruning from " << Parser.GetParameters().PruningStart << " with step size " << Parser.GetParameters().PruningStep << " to " << Parser.GetParameters().PruningEnd << "!" << std::endl << std::endl;
      for (double PruningFactor = Parser.GetParameters().PruningStart; PruningFactor >= Parser.GetParameters().PruningEnd; PruningFactor -= Parser.GetParameters().PruningStep) {
        Reader.PruneLattices(PruningFactor);
        std::cout << "  Pruning factor: " << PruningFactor << std::endl;
        LPERCalculator InputLatticeStatistics(Reader.GetInputFsts(), Reader.GetReferenceFsts(), Reader.GetReferenceIntToStringVector(), MaxNumThreads, Reader.GetInputFileNames(), Parser.GetParameters().OutputDirectoryBasename + "KnownN_" + std::to_string(Parser.GetParameters().KnownN) + "_UnkN_" + std::to_string(Parser.GetParameters().UnkN) + "/" + Parser.GetParameters().OutputFilesBasename + "LPER_", Parser.GetParameters().OutputEditOperations);
        DebugLib::PrintEditDistanceStatistics(InputLatticeStatistics.GetInsDelSubCorrNFoundNRef(), "Lattice phoneme error rate", "PER");
        if (Parser.GetParameters().PruningStart == std::numeric_limits<double>::infinity()) {
          break;
        }
      }
    }
  }

  if (Parser.GetParameters().AmScale != 1) {
    Reader.ApplyAcousticModelScalingFactor(Parser.GetParameters().AmScale);
  }
  Reader.ApplyWordEndTransducer();
  Reader.ApplySentEndTransducer();


  // initialize empty language model and dictionary and perform
  // language model initialization from initialization sentences
  // (parse reference fsts, add to language model and retrain)
  InitializeLanguageModel(Parser.GetParameters().UnkN, Parser.GetParameters().KnownN);

//   // Id2 Character sequence vector for printing
//   std::vector<std::string> Id2CharacterSequenceVector(LanguageModel->GetId2CharacterSequenceVector());

//   // generate sentences of words from the Character language model
//   std::vector<std::vector<int> > GeneratedSentencesFromCHPYLM = LanguageModel->Generate("CHPYLM", 100000, -1, NULL);
//   for (auto &Sentence : GeneratedSentencesFromCHPYLM) {
//     for (auto &Character : Sentence) {
//       if (Character == UNKEND_SYMBOLID) {
//         std::cout << " ";
//       } else if (Character == SENTEND_SYMBOLID) {
//         std::cout << std::endl;
//       } else {
// //         std::cout << Id2CharacterSequenceVector.at(Character) << "[" << Character << "] ";
//         std::cout << Id2CharacterSequenceVector.at(Character);
//       }
//     }
//     std::cout << std::endl;
//   }

//   // generate sentences of words from the Character language model
//   std::vector<std::vector<int> > GeneratedSentencesFromWHPYLM = LanguageModel->Generate("WHPYLM", 10000, SentEndWordId);
//   for (auto &Sentence : GeneratedSentencesFromWHPYLM) {
//     for (auto &Word : Sentence) {
//       if (Word == SentEndWordId) {
//         std::cout << std::endl;
//       } else {
// //         std::cout << Id2CharacterSequenceVector.at(Word) << "[" << Word << "] ";
//         std::cout << Id2CharacterSequenceVector.at(Word) << " ";
//       }
//     }
//     std::cout << std::endl;
//   }


  // run word segmentation iterations
  DoWordSegmentationIterations(Parser.GetParameters().NumIter);

  // cleanup
  delete LanguageModel;
}

void LatticeWordSegmentation::InitializeLanguageModel(int UnkN, int KnownN)
{
  std::cout << " Intializing empty language model with KnownN=" << KnownN << ", UnkN=" << UnkN << "!" << std::endl << std::endl;

  LanguageModel = new NHPYLM(UnkN, KnownN, Reader.GetInputIntToStringVector(), CHARACTERSBEGIN);
  LanguageModel->AddCharacterIdSequenceToDictionary(std::vector<int>(1, SENTEND_SYMBOLID).begin(), 1);
  SentEndWordId = LanguageModel->GetWordId(std::vector<int>(1, SENTEND_SYMBOLID).begin(), 1);
  WHPYLMContextLenght = LanguageModel->GetWHPYLMOrder() - 1;

  if (Parser.GetParameters().InitLM) {
    ParseInitializationSentencesAndInitializeLanguageModel();
    TrainLanguageModel(InitializationSentences, Parser.GetParameters().InitLmNumIterations);
  }
}

void LatticeWordSegmentation::ParseInitializationSentencesAndInitializeLanguageModel()
{
  std::cout << " Parsing initialization sentences and initializing language model!" << std::endl;

  NumInitializationSentences = Reader.GetInitFsts().size();
  int IdxInitFst = 0;
  InitializationSentences.resize(NumInitializationSentences);
  for (const fst::VectorFst< fst::LogArc > &InitializationFst : Reader.GetInitFsts()) {
    std::cout << "\r  Sentence: " << IdxInitFst + 1 << " of " << NumInitializationSentences;
    SampleLib::ParseSampleAndAddCharacterIdSequenceToDictionary(InitializationFst, LanguageModel, &InitializationSentences.at(IdxInitFst));
    InitializationSentences.at(IdxInitFst).insert(InitializationSentences.at(IdxInitFst).begin(), WHPYLMContextLenght, SentEndWordId);
    LanguageModel->AddWordSequenceToLm(InitializationSentences.at(IdxInitFst));
    IdxInitFst++;
  }
  std::cout << std::endl << std::endl;
  CalcWordLengthStats();
  LanguageModel->ResampleHyperParameters();
  DebugLib::PrintSentencesPerplexity(InitializationSentences, *LanguageModel);
  DebugLib::PrintLanguageModelStats(*LanguageModel);
}

void LatticeWordSegmentation::TrainLanguageModel(const vector< vector< int > > &Sentences, int MaxNumLMTrainIter)
{
  std::cout << " Training language model!" << std::endl;

  // create index vector of shuffled sentence indices
  std::vector<int> ShuffledIndices(Sentences.size());
  std::iota(ShuffledIndices.begin(), ShuffledIndices.end(), 0);

  // do language model retraining
  for (int LanguageModelTrainIter = 0; LanguageModelTrainIter < MaxNumLMTrainIter; LanguageModelTrainIter++) {
    // shuffele sentence indices for each iteration
    std::shuffle(ShuffledIndices.begin(), ShuffledIndices.end(), RandomGenerator);

    for (int & IdxInitFst : ShuffledIndices) {
      std::cout << "\r  Iteration: " << LanguageModelTrainIter + 1 << " of " << MaxNumLMTrainIter << ", Sentence: " << IdxInitFst + 1 << " of " << Sentences.size();

      LanguageModel->RemoveWordSequenceFromLm(Sentences.at(IdxInitFst));
      LanguageModel->AddWordSequenceToLm(Sentences.at(IdxInitFst));
    }
    std::cout << std::endl;
    CalcWordLengthStats();
    LanguageModel->ResampleHyperParameters();
    DebugLib::PrintSentencesPerplexity(Sentences, *LanguageModel);
    DebugLib::PrintLanguageModelStats(*LanguageModel);
  }
  std::cout << std::endl;
}

void LatticeWordSegmentation::DoWordSegmentationIterations(unsigned int MaxNumSegIter)
{
  std::cout << " Starting word segmentation!" << std::endl;

  // initialize output vector for sampled sentences and fsts
  NumSampledSentences = Reader.GetInputFsts().size();
  SampledSentences.resize(NumSampledSentences, std::vector<int>(WHPYLMContextLenght, SentEndWordId));
  SampledFsts.resize(NumSampledSentences);

  // create index vector of shuffled sentence indices
  std::vector<int> ShuffledIndices(NumSampledSentences);
  std::iota(ShuffledIndices.begin(), ShuffledIndices.end(), 0);

  for (unsigned int IdxIter = 0; IdxIter < MaxNumSegIter; ++IdxIter) {
    std::cout << "  Iteration: " << IdxIter + 1 << " of " << MaxNumSegIter << std::endl;

    // shuffele sentence indices for each iteration
    std::shuffle(ShuffledIndices.begin(), ShuffledIndices.end(), RandomGenerator);

    // initialize lexicon transducer
    Timer.tLexFst.SetStart();
    LexFst LexiconTransducer(Parser.GetParameters().Debug, Reader.GetInputIntToStringVector(), CHARACTERSBEGIN, LanguageModel->GetWHPYLMBaseProbabilitiesScale());
    LexiconTransducer.BuildLexiconTansducer(LanguageModel->GetWord2Id());
    Timer.tLexFst.AddTimeSinceStartToDuration();
//     FileReader::PrintFST("lattice_debug/LaxiconTransducer.fst", LanguageModel->GetId2CharacterSequenceVector(), LexiconTransducer, true, NAMESANDIDS);

    // iterate over every sentence
    DoWordSegmentationSentenceIterations(ShuffledIndices, &LexiconTransducer, IdxIter);

    // calculate word length statistics
    CalcWordLengthStats();

    // resample hyperparameters of languge model
    Timer.tHypSample.SetStart();
    LanguageModel->ResampleHyperParameters();
    Timer.tHypSample.AddTimeSinceStartToDuration();

    // set discount and concentration to zero for unigram word model (no new words allowed)
    if (((IdxIter + 1) >= Parser.GetParameters().DeactivateCharacterModel) && Parser.GetParameters().DeactivateCharacterModel > 0) {
      LanguageModel->SetParameter("WHPYLM", "Discount", 0, 0);
      LanguageModel->SetParameter("WHPYLM", "Concentration", 0, 0.00001);
    }

    // write to output
    FileReader::PrintSentencesToFile(Parser.GetParameters().OutputDirectoryBasename + "KnownN_" + std::to_string(LanguageModel->GetWHPYLMOrder()) + "_UnkN_" + std::to_string(LanguageModel->GetCHPYLMOrder()) + "/" + Parser.GetParameters().OutputFilesBasename + "Sentences_Iter_" + std::to_string(IdxIter + 1), SampledSentences, LanguageModel->GetId2CharacterSequenceVector());

    // get perplexity
    Timer.tCalcPerplexity.SetStart();
    DebugLib::PrintSentencesPerplexity(SampledSentences, *LanguageModel);
    Timer.tCalcPerplexity.AddTimeSinceStartToDuration();

    // calculate word error rate
    bool OutputEditOperations = ((IdxIter == (MaxNumSegIter - 1)) || (IdxIter == (Parser.GetParameters().DeactivateCharacterModel - 1)) || (IdxIter == (Parser.GetParameters().UseViterby - 2)));
    if (Parser.GetParameters().CalculateWER && ((IdxIter % Parser.GetParameters().EvalInterval) == 0)) {
      Timer.tCalcWER.SetStart();
//       std::cout << "Start WER calculation" << std::endl;
      WERCalculator SegmentationStatisticsCalculator(SampledSentences, Reader.GetReferenceFsts(), *LanguageModel, WHPYLMContextLenght, 1, Reader.GetInputFileNames(), Parser.GetParameters().OutputDirectoryBasename + "KnownN_" + std::to_string(LanguageModel->GetWHPYLMOrder()) + "_UnkN_" + std::to_string(LanguageModel->GetCHPYLMOrder()) + "/" + Parser.GetParameters().OutputFilesBasename + "WER_Iter_" + std::to_string(IdxIter + 1) + "_", Parser.GetParameters().OutputEditOperations && OutputEditOperations);
//       std::cout << "Finished WER calculation" << std::endl;
      DebugLib::PrintEditDistanceStatistics(SegmentationStatisticsCalculator.GetInsDelSubCorrNFoundNRef(), "Word error rate", "WER");
      DebugLib::PrintLexiconStatistics(SegmentationStatisticsCalculator.GetLexiconCorrNFoundNRef());
      Timer.tCalcWER.AddTimeSinceStartToDuration();
    }

    if (Parser.GetParameters().CalculatePER && ((IdxIter % Parser.GetParameters().EvalInterval) == 0)) {
      Timer.tCalcPER.SetStart();
      PERCalculator PhonemeStatisticsCalculator(SampledFsts, Reader.GetReferenceFsts(), Reader.GetReferenceIntToStringVector(), MaxNumThreads, Reader.GetInputFileNames(), Parser.GetParameters().OutputDirectoryBasename + "KnownN_" + std::to_string(LanguageModel->GetWHPYLMOrder()) + "_UnkN_" + std::to_string(LanguageModel->GetCHPYLMOrder()) + "/" + Parser.GetParameters().OutputFilesBasename + "PER_Iter_" + std::to_string(IdxIter + 1) + "_", Parser.GetParameters().OutputEditOperations && OutputEditOperations);
      DebugLib::PrintEditDistanceStatistics(PhonemeStatisticsCalculator.GetInsDelSubCorrNFoundNRef(), "Phoneme error rate", "PER");
      Timer.tCalcPER.AddTimeSinceStartToDuration();
    }

    // output some language model stats
    DebugLib::PrintLanguageModelStats(*LanguageModel);

    // switch language model order
    if ((IdxIter + 1) == Parser.GetParameters().SwitchIter) {
      SwitchLanguageModelOrders(Parser.GetParameters().NewUnkN, Parser.GetParameters().NewKnownN);
      TrainLanguageModel(SampledSentences, Parser.GetParameters().NewLMNumIters);
    }

    // output some timing statistics
    Timer.PrintTimingStatistics();
  }
}

void LatticeWordSegmentation::DoWordSegmentationSentenceIterations(const vector< int > &ShuffledIndices, LexFst *LexiconTransducer, unsigned int IdxIter)
{
  for (int IdxSentence = 0; IdxSentence < NumSampledSentences; IdxSentence += MaxNumThreads) {
    int NumThreads = std::min(MaxNumThreads, NumSampledSentences - IdxSentence);
    std::cout << "\r   Sentence: " << IdxSentence + 1 << " of " << NumSampledSentences;

    // remove words from lexicon, fst and lm
    Timer.tRemove.SetStart();
    for (int IdxThread = 0; IdxThread < NumThreads; ++IdxThread) {
      SampleLib::RemoveWordsFromDictionaryLexFSTAndLM(SampledSentences.at(ShuffledIndices[IdxSentence + IdxThread]).begin() + WHPYLMContextLenght, SampledSentences.at(ShuffledIndices[IdxSentence + IdxThread]).size() - WHPYLMContextLenght, LanguageModel, LexiconTransducer, SentEndWordId);
    }
    Timer.tRemove.AddTimeSinceStartToDuration();

    // start composing and sampling threads (the last thread will run in the main programm since this is more effiective if running only one thread
    Timer.tSample.SetStart();
    bool UseViterby = (Parser.GetParameters().UseViterby > 0) && ((IdxIter + 1) >= Parser.GetParameters().UseViterby);
    for (int IdxThread = 0; IdxThread < (NumThreads - 1); ++IdxThread) {
      Threads[IdxThread] = std::thread(SampleLib::ComposeAndSampleFromInputLexiconAndLM, &Reader.GetInputFsts().at(ShuffledIndices[IdxSentence + IdxThread]), LexiconTransducer, LanguageModel, SentEndWordId, &SampledFsts[ShuffledIndices[IdxSentence + IdxThread]], &Timer.tInSamples[IdxThread], Parser.GetParameters().BeamWidth, UseViterby);
    }
    SampleLib::ComposeAndSampleFromInputLexiconAndLM(&Reader.GetInputFsts().at(ShuffledIndices[IdxSentence + (NumThreads - 1)]), LexiconTransducer, LanguageModel, SentEndWordId, &SampledFsts[ShuffledIndices[IdxSentence + (NumThreads - 1)]], &Timer.tInSamples[(NumThreads - 1)], Parser.GetParameters().BeamWidth, UseViterby);

    // join threads
    for (int IdxThread = 0; IdxThread < (NumThreads - 1); ++IdxThread) {
      Threads[IdxThread].join();
    }
    Timer.tSample.AddTimeSinceStartToDuration();
//     std::cout << "End compose and sample from input lexicon and lm" << std::endl << std::flush;

    // parse and add sample
    Timer.tParseAndAdd.SetStart();
    for (int IdxThread = 0; IdxThread < NumThreads; ++IdxThread) {
//       std::cout << SampledFsts[ShuffledIndices[IdxSentence + IdxThread]].NumStates() << " States" << std::endl << std::flush;
      SampleLib::ParseSampleAndAddCharacterIdSequenceToDictionaryLexFstAndLM(SampledFsts[ShuffledIndices[IdxSentence + IdxThread]], SentEndWordId, LanguageModel, LexiconTransducer, &SampledSentences[ShuffledIndices[IdxSentence + IdxThread]]);
    }
    Timer.tParseAndAdd.AddTimeSinceStartToDuration();
//     std::cout << "End parse sample and add charactrer id sequence to dictionary" << std::endl << std::flush;
  }
  std::cout << std::endl << std::endl;
}


void LatticeWordSegmentation::CalcWordLengthStats()
{
  std::vector<double> GeneratedWordLengthProbabilities;
  LanguageModel->Generate("CHPYLM", 100000, -1, &GeneratedWordLengthProbabilities);

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
      ObservedWordLengthProbabilities.resize(std::max(static_cast<size_t>(WordLengths[WordId] + 1), ObservedWordLengthProbabilities.size()));
      ObservedWordLengthProbabilities.at(WordLengths[WordId]) += TablesPerWord;
    }
  }

  int WordLength = 0;
  for (double & WordLengthProbability : ObservedWordLengthProbabilities) {
    WordLengthProbability /= NumWords;
    std::cout << std::setprecision(4) << std::fixed << "Word length: " << WordLength++ << ", word length probability: " << WordLengthProbability << std::endl;
  }
  MeanWordLength /= NumWords;
  std::cout << "Mean word length: " << MeanWordLength << ", number of word: " << NumWords << std::endl << std::endl;

  if (Parser.GetParameters().WordLengthModulation > -1) {
    WordLengthProbCalculator WordLengthProbabilityVectorCalculator;
    WordLengthProbabilityVectorCalculator.SetGeneratedLengthDistribution(GeneratedWordLengthProbabilities);
    if (Parser.GetParameters().WordLengthModulation == 0) {
      WordLengthProbabilityVectorCalculator.SetObservedLengthDistribution(ObservedWordLengthProbabilities);
    } else {
//       std::cout << "Using: " << Parser.GetParameters().WordLengthModulation << std::endl;
      WordLengthProbabilityVectorCalculator.SetDesiredLengthMean(Parser.GetParameters().WordLengthModulation);
    }
    LanguageModel->SetWHPYLMBaseProbabilitiesScale(WordLengthProbabilityVectorCalculator.GetScaledWordLengthProbabilityVector());
  }
}


void LatticeWordSegmentation::SwitchLanguageModelOrders(int NewUnkN, int NewKnownN)
{
  std::cout << " Switching to KnownN=" << NewKnownN << ", UnkN=" << NewUnkN << std::endl;

  NHPYLM *OldLanguageModel = LanguageModel;
  int OldWHPYLMContextLength = WHPYLMContextLenght;
  InitializeLanguageModel(NewUnkN, NewKnownN);

  int IdxSampledSentence = 0;
  for (const std::vector<int> &Sentence : SampledSentences) {
    std::vector<int> TempSampledSentence;
    TempSampledSentence.reserve(Sentence.size() - OldWHPYLMContextLength + WHPYLMContextLenght);
    TempSampledSentence.assign(WHPYLMContextLenght, SentEndWordId);
    for (std::vector<int>::const_iterator Id = Sentence.begin() + OldWHPYLMContextLength; Id != Sentence.end(); ++Id) {
      WordBeginLengthPair Word = OldLanguageModel->GetWordBeginLength(*Id);
      TempSampledSentence.push_back(LanguageModel->AddCharacterIdSequenceToDictionary(Word.first, Word.second).first);
    }
    SampledSentences[IdxSampledSentence] = TempSampledSentence;
    LanguageModel->AddWordSequenceToLm(TempSampledSentence);
    ++IdxSampledSentence;
  }
  CalcWordLengthStats();
  LanguageModel->ResampleHyperParameters();
  delete OldLanguageModel;
}