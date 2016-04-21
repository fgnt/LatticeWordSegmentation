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

#include <iomanip>
#include <numeric>
#include <chrono>
#include <fst/compose.h>
#include "LatticeWordSegmentation.hpp"
#include "SampleLib.hpp"
#include "ParseLib.hpp"
#include "DebugLib.hpp"
#include "NHPYLMFst.hpp"
#include "WordLengthProbCalculator.hpp"
#include "Evaluate/Evaluate.hpp"

std::default_random_engine LatticeWordSegmentation::RandomGenerator(
  std::chrono::system_clock::now().time_since_epoch().count()
);


LatticeWordSegmentation::LatticeWordSegmentation(const ParameterStruct& Params,
                                                 const FileData& InputFileData) :
  Params(Params),
  InputFileData(InputFileData),
  MaxNumThreads(Params.NoThreads),
  Threads(MaxNumThreads - 1),
  Timer(MaxNumThreads, 4)
{
}

/*****************************************************************************
 * Main functions to perform word segmentation
 * - Entry function: DoWordSegmentation
 *   - Do initializations of language model and output variables and run outer
 *     loop over iterations. Also reinitialized lexicon transducer before each
 *     iteration and upddates languge model after iteration (possibly changes
 *     language model orders). Outputs results and statistics after each
 *     iteration
 * - Perform main segmentation and inference steps:
 *   DoWordSegmentationSentenceIterations
 *   - Remove sentences from dictionary, language model and fsts
 *   - Sample new segmentation for imput lattices using lexicon and language
 *     model
 *   - Add segmentation result to dictionary, language model and fsts
 ******************************************************************************/

void LatticeWordSegmentation::DoWordSegmentation()
{
  std::cout << " Starting word segmentation!" << std::endl;

  // initialize empty language model and dictionary and perform
  // language model initialization from initialization sentences
  // (parse reference fsts, add to language model and retrain)
  InitializeLanguageModel(Params.UnkN, Params.KnownN);

  // initialize output vector for sampled sentences and fsts
  NumSampledSentences = InputFileData.GetInputFsts().size();
  SampledSentences.resize(
    NumSampledSentences,
    std::vector<int>(WHPYLMContextLength, SentEndWordId)
  );
  TimedSampledSentences.resize(NumSampledSentences);
  SampledFsts.resize(NumSampledSentences);

  //Create Evaluate object to perform measurements
  Evaluate Eval(Params, InputFileData, Timer, LanguageModel);

  // create index vector of shuffled sentence indices
  std::vector<int> ShuffledIndices(NumSampledSentences);
  std::iota(ShuffledIndices.begin(), ShuffledIndices.end(), 0);

  // run the actual iterations
  for (std::size_t IdxIter = 0; IdxIter < Params.NumIter; ++IdxIter) {
    std::cout << "  Iteration: " << IdxIter + 1
              << " of " << Params.NumIter << std::endl;

    // shuffle sentence indices for each iteration
    std::shuffle(ShuffledIndices.begin(), ShuffledIndices.end(),
                 RandomGenerator);

    // initialize lexicon transducer
    Timer.tLexFst.SetStart();
    LexFst LexiconTransducer(
      Params.Debug,
      InputFileData.GetInputIntToStringVector(),
      CHARACTERSBEGIN,
      LanguageModel->GetWHPYLMBaseProbabilitiesScale()
    );
    LexiconTransducer.BuildLexiconTansducer(LanguageModel->GetWord2Id());
    Timer.tLexFst.AddTimeSinceStartToDuration();

    // iterate over every sentence
    DoWordSegmentationSentenceIterations(
      ShuffledIndices, &LexiconTransducer, IdxIter);

    // calculate and update word length statistics
    WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
      LanguageModel,
      Params.WordLengthModulation);

    // resample hyperparameters of language model
    Timer.tHypSample.SetStart();
    LanguageModel->ResampleHyperParameters();
    Timer.tHypSample.AddTimeSinceStartToDuration();

    // set discount and concentration to zero for unigram word model
    // (no new words allowed)
    if (((IdxIter + 1) >= Params.DeactivateCharacterModel) &&
        Params.DeactivateCharacterModel > 0) {
      LanguageModel->SetParameter("WHPYLM", "Discount", 0, 0);
      LanguageModel->SetParameter("WHPYLM", "Concentration", 0, 0.00001);
    }

    // Evaluation of current iteration
    Eval.WriteSentencesToOutputFiles(
          SampledSentences, TimedSampledSentences, IdxIter);
    Eval.OutputMeasureStatistics(SampledSentences, SampledFsts, IdxIter);

    // switch language model order, if specified
    if ((IdxIter + 1) == Params.SwitchIter) {
      SwitchLanguageModelOrders(Params.NewUnkN, Params.NewKnownN);
    }
  }
  // cleanup
  delete LanguageModel;
}

void LatticeWordSegmentation::DoWordSegmentationSentenceIterations(
  const vector< int > &ShuffledIndices,
  LexFst *LexiconTransducer,
  std::size_t IdxIter
)
{
  for (std::size_t IdxSentence = 0; IdxSentence < NumSampledSentences;
       IdxSentence += MaxNumThreads) {
    std::size_t NumThreads =
        std::min(MaxNumThreads, NumSampledSentences - IdxSentence);

    std::cout << "\r   Sentence: " << IdxSentence + 1
              << " of " << NumSampledSentences;

    // remove words from lexicon, fst and lm
    Timer.tRemove.SetStart();
    for (std::size_t IdxThread = 0; IdxThread < NumThreads; ++IdxThread) {
      std::size_t CurrentIndex = ShuffledIndices[IdxSentence + IdxThread];
      ParseLib::RemoveWordsFromDictionaryLexFSTAndLM(
        SampledSentences.at(CurrentIndex).begin() + WHPYLMContextLength,
        SampledSentences.at(CurrentIndex).size() - WHPYLMContextLength,
        LanguageModel,
        LexiconTransducer,
        SentEndWordId
      );
    }
    Timer.tRemove.AddTimeSinceStartToDuration();

    // start composing and sampling threads
    // the last thread will run in the main program since this is
    // more effective if running only one thread
    Timer.tSample.SetStart();
    bool UseViterby =
      (Params.UseViterby > 0) && ((IdxIter + 1) >= Params.UseViterby);

    for (std::size_t IdxThread = 0; IdxThread < (NumThreads - 1); ++IdxThread) {
      std::size_t CurrentIndex = ShuffledIndices[IdxSentence + IdxThread];
      Threads[IdxThread] =
        std::thread(SampleLib::ComposeAndSampleFromInputLexiconAndLM,
                    &InputFileData.GetInputFsts().at(CurrentIndex),
                    LexiconTransducer,
                    LanguageModel,
                    SentEndWordId,
                    &SampledFsts[CurrentIndex],
                    &Timer.tInSamples[IdxThread],
                    Params.BeamWidth,
                    UseViterby
                   );
    }

    std::size_t CurrentIndex = ShuffledIndices[IdxSentence + (NumThreads - 1)];
    SampleLib::ComposeAndSampleFromInputLexiconAndLM(
      &InputFileData.GetInputFsts().at(CurrentIndex),
      LexiconTransducer,
      LanguageModel,
      SentEndWordId,
      &SampledFsts[CurrentIndex],
      &Timer.tInSamples[(NumThreads - 1)],
      Params.BeamWidth,
      UseViterby
    );

    // join threads
    for (std::size_t IdxThread = 0; IdxThread < (NumThreads - 1); ++IdxThread) {
      Threads[IdxThread].join();
    }
    Timer.tSample.AddTimeSinceStartToDuration();
//     std::cout << "End compose and sample from input lexicon and lm" << std::endl << std::flush;

    // parse and add sample
    Timer.tParseAndAdd.SetStart();
    for (std::size_t IdxThread = 0; IdxThread < NumThreads; ++IdxThread) {
//       std::cout << SampledFsts[ShuffledIndices[IdxSentence + IdxThread]].NumStates() << " States" << std::endl << std::flush;
      ParseLib::ParseSampleAndAddCharacterIdSequenceToDictionaryLexFstAndLM(
        SampledFsts[ShuffledIndices[IdxSentence + IdxThread]],
        SentEndWordId,
        LanguageModel,
        LexiconTransducer,
        &SampledSentences[ShuffledIndices[IdxSentence + IdxThread]],
        &TimedSampledSentences[ShuffledIndices[IdxSentence + IdxThread]],
        InputFileData.GetInputArcInfos()
      );
    }
    Timer.tParseAndAdd.AddTimeSinceStartToDuration();
//     std::cout << "End parse sample and add charactrer id sequence to dictionary" << std::endl << std::flush;
  }
  std::cout << std::endl << std::endl;
}

/***********************************************************
 * Functions for language  model modifications:
 * - SwitchLanguageModelOrders
 * - InitializeLanguageModel
 * - ParseInitializationSentencesAndInitializeLanguageModel
 * - TrainLanguageModel
************************************************************/

void LatticeWordSegmentation::SwitchLanguageModelOrders(
  int NewUnkN,
  int NewKnownN
)
{
  std::cout << " Switching to KnownN=" << NewKnownN
            << ", UnkN=" << NewUnkN << std::endl;

  // instantiate new language model and initialize
  NHPYLM *OldLanguageModel = LanguageModel;
  std::size_t OldWHPYLMContextLength = WHPYLMContextLength;
  InitializeLanguageModel(NewUnkN, NewKnownN);

  // parse words in sampled sentences and add to new dictionary
  // and language model. Also update sentences with new word ids
  for (auto& Sentence : SampledSentences) {
    std::size_t TempSampledSentenceSize =
      Sentence.size() - OldWHPYLMContextLength + WHPYLMContextLength;
    std::vector<int> TempSampledSentence;
    TempSampledSentence.reserve(TempSampledSentenceSize);
    TempSampledSentence.assign(WHPYLMContextLength, SentEndWordId);
    for (auto Id = Sentence.begin() + OldWHPYLMContextLength;
         Id != Sentence.end(); ++Id) {
      WordBeginLengthPair Word = OldLanguageModel->GetWordBeginLength(*Id);
      TempSampledSentence.push_back(
        LanguageModel->AddCharacterIdSequenceToDictionary(
          Word.first, Word.second).first
      );
    }
    Sentence = TempSampledSentence;
    LanguageModel->AddWordSequenceToLm(TempSampledSentence);
  }

  // retrain language model for some iterations
  TrainLanguageModel(SampledSentences, Params.NewLMNumIters);
  
  // calculate and update word length statistics
  WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
    LanguageModel,
    Params.WordLengthModulation);
  
  // resample hyperparameters of language model
  LanguageModel->ResampleHyperParameters();
  
  // cleanup: delete old language model
  delete OldLanguageModel;
}

void LatticeWordSegmentation::InitializeLanguageModel(int UnkN, int KnownN)
{
  std::cout << " Initializing empty language model with KnownN="
            << KnownN << ", UnkN=" << UnkN << "!"
            << std::endl << std::endl;

  LanguageModel = new NHPYLM(UnkN, KnownN,
                             InputFileData.GetInputIntToStringVector(),
                             CHARACTERSBEGIN);

  LanguageModel->AddCharacterIdSequenceToDictionary(
    std::vector<int>(1, SENTEND_SYMBOLID).begin(), 1);

  SentEndWordId = LanguageModel->GetWordId(
                    std::vector<int>(1, SENTEND_SYMBOLID).begin(), 1);

  WHPYLMContextLength = LanguageModel->GetWHPYLMOrder() - 1;

  if (Params.InitLM) {
    ParseInitializationSentencesAndInitializeLanguageModel();
    TrainLanguageModel(InitializationSentences, Params.InitLmNumIterations);
  }
}

void LatticeWordSegmentation::
ParseInitializationSentencesAndInitializeLanguageModel()
{
  std::cout << " Parsing initialization sentences "
            << "and initializing language model!" << std::endl;

  // parse words in initialization sentences and add to new dictionary
  // and language model. Also update initialization sentences with new word ids
  NumInitializationSentences = InputFileData.GetInitFsts().size();
  InitializationSentences.resize(NumInitializationSentences);
  std::size_t IdxInitFst = 0;
  for (const LogVectorFst &InitializationFst : InputFileData.GetInitFsts()) {
    std::cout << "\r  Sentence: " << IdxInitFst + 1
              << " of " << NumInitializationSentences;

    auto& InitializationSentence = InitializationSentences.at(IdxInitFst);

    ParseLib::ParseSampleAndAddCharacterIdSequenceToDictionary(
      InitializationFst,
      LanguageModel,
      &InitializationSentence
    );

    // insert enough sentence end words at the beginning to fill out the
    // LM context size
    InitializationSentence.insert(InitializationSentence.begin(),
                        WHPYLMContextLength, SentEndWordId);

    LanguageModel->AddWordSequenceToLm(InitializationSentence);
    IdxInitFst++;
  }
  std::cout << std::endl << std::endl;
  
  // calculate and update word length statistics
  WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
    LanguageModel,
    Params.WordLengthModulation);
  
  // resample hyperparameters of language model
  LanguageModel->ResampleHyperParameters();
  
  // get perplexity
  DebugLib::PrintSentencesPerplexity(InitializationSentences, *LanguageModel);

  // output some language model stats
  DebugLib::PrintLanguageModelStats(*LanguageModel);
}

void LatticeWordSegmentation::TrainLanguageModel(
  const vector< vector< int > > &Sentences,
  std::size_t MaxNumLMTrainIter
)
{
  std::cout << " Training language model!" << std::endl;

  // create index vector of shuffled sentence indices
  std::vector<int> ShuffledIndices(Sentences.size());
  std::iota(ShuffledIndices.begin(), ShuffledIndices.end(), 0);

  // do language model retraining
  for (std::size_t LMTrainIter = 0;
       LMTrainIter < MaxNumLMTrainIter; LMTrainIter++) {
    // shuffle sentence indices for each iteration
    std::shuffle(ShuffledIndices.begin(),
                 ShuffledIndices.end(), RandomGenerator);

    // remove and add sentences again
    for (auto IdxInitFst : ShuffledIndices) {
      std::cout << "\r  Iteration: " << LMTrainIter + 1
                << " of " << MaxNumLMTrainIter << ", Sentence: "
                << IdxInitFst + 1 << " of " << Sentences.size();

      LanguageModel->RemoveWordSequenceFromLm(Sentences.at(IdxInitFst));
      LanguageModel->AddWordSequenceToLm(Sentences.at(IdxInitFst));
    }
    std::cout << std::endl;
    
    // calculate and update word length statistics
    WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
      LanguageModel,
      Params.WordLengthModulation);
    
    // resample hyperparameters of language model
    LanguageModel->ResampleHyperParameters();

    // get perplexity
    DebugLib::PrintSentencesPerplexity(Sentences, *LanguageModel);

    // output some language model stats
    DebugLib::PrintLanguageModelStats(*LanguageModel);
  }
  std::cout << std::endl;
}
