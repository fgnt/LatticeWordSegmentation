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

#include <iostream>
#include <iomanip>
#include <numeric>
#include <chrono>
#include <fst/compose.h>
#include <fst/arcsort.h>
#include "LatticeWordSegmentation.hpp"
#include "SampleLib.hpp"
#include "ParseLib.hpp"
#include "DebugLib.hpp"
#include "NHPYLMFst.hpp"
#include "WordLengthProbCalculator.hpp"
#include "Evaluate/Evaluate.hpp"

using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::cerr;

std::default_random_engine LatticeWordSegmentation::RandomGenerator(
  std::chrono::system_clock::now().time_since_epoch().count()
);


LatticeWordSegmentation::LatticeWordSegmentation(
  const ParameterStruct& Params,
  const FileData& InputFileData
) :
  Params(Params),
  InputFileData(InputFileData),
  MaxNumThreads(Params.NoThreads),
  Threads(MaxNumThreads - 1),
  Timer(MaxNumThreads, Params.AddCharN > 0 ? 5 : 4)
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
  InitializeLanguageModel(Params.UnkN, Params.KnownN, Params.AddCharN);

  // initialize output vector for sampled sentences and fsts
  NumSampledSentences = InputFileData.GetInputFsts().size();
  SampledSentences.resize(
    NumSampledSentences,
    std::vector<int>(WHPYLMContextLength, SentEndWordId)
  );
  TimedSampledSentences.resize(NumSampledSentences);
  SampledFsts.resize(NumSampledSentences);

  // create index vector of shuffled sentence indices
  std::vector<int> ShuffledIndices(NumSampledSentences);
  std::iota(ShuffledIndices.begin(), ShuffledIndices.end(), 0);

  // run the actual iterations
  for (std::size_t IdxIter = 0; IdxIter < Params.NumIter; ++IdxIter) {
    std::cout << "  Iteration: " << IdxIter + 1
              << " of " << Params.NumIter << std::endl;

    // shuffle sentence indices for each iteration
    std::shuffle(
      ShuffledIndices.begin(),
      ShuffledIndices.end(),
      RandomGenerator
    );

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
      ShuffledIndices, &LexiconTransducer, IdxIter
    );

    // calculate and update word length statistics
    WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
      LanguageModel,
      Params.WordLengthModulation
    );

    // resample hyperparameters of language model
    Timer.tHypSample.SetStart();
    LanguageModel->ResampleHyperParameters();
    if (CharacterLanguageModel != nullptr) {
      CharacterLanguageModel->ResampleHyperParameters();
    }
    Timer.tHypSample.AddTimeSinceStartToDuration();

    // set discount and concentration to zero for unigram word model
    // (no new words allowed)
    if (((IdxIter + 1) >= Params.DeactivateCharacterModel) &&
        Params.DeactivateCharacterModel > 0) {
      LanguageModel->SetParameter("WHPYLM", "Discount", 0, 0);
      LanguageModel->SetParameter("WHPYLM", "Concentration", 0, 0.00001);
    }

    // Evaluation of current iteration
    Evaluate Eval(
      Params,
      InputFileData,
      Timer,
      LanguageModel,
      CharacterLanguageModel
    );
    Eval.WriteSentencesToOutputFiles(
      SampledSentences, TimedSampledSentences, IdxIter
    );
    Eval.OutputMeasureStatistics(SampledSentences, SampledFsts, IdxIter);

    // switch language model order, if specified
    if ((IdxIter + 1) == Params.SwitchIter) {
      SwitchLanguageModelOrders(
        Params.NewUnkN,
        Params.NewKnownN,
        Params.NewAddCharN
      );
    }
  }

  if (Params.WriteRescoredLattices) {
    WriteRescoredLattices();
  }

  // cleanup
  delete LanguageModel;
  delete CharacterLanguageModel;
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

    std::cerr << "\r   Sentence: " << IdxSentence + 1
              << " of " << NumSampledSentences;

    // remove words from lexicon, fst and lm
    Timer.tRemove.SetStart();
    for (std::size_t IdxThread = 0; IdxThread < NumThreads; ++IdxThread) {
      std::size_t CurrentIndex = ShuffledIndices[IdxSentence + IdxThread];
      if (CharacterLanguageModel != nullptr) {
        ParseLib::RemoveWordSequenceFromAddCharLM(
          SampledSentences.at(CurrentIndex).begin() + WHPYLMContextLength,
          SampledSentences.at(CurrentIndex).size() - WHPYLMContextLength,
          *LanguageModel,
          CharacterLanguageModel
        );
      }
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

    std::unique_ptr<NHPYLMFst> CharacterLanguageModelFST;
    if (CharacterLanguageModel != nullptr) {
      CharacterLanguageModelFST = std::unique_ptr<NHPYLMFst>(new NHPYLMFst(
          *CharacterLanguageModel, EOW, std::vector<bool>(), true, AvailChars));
    }

    auto SampleFn = [&](std::size_t IdxSentence, std::size_t IdxThread) {
      std::size_t CurrentIndex = ShuffledIndices[IdxSentence + IdxThread];
      LogVectorFst const *InputFst;
      std::unique_ptr<LogVectorFst> CharFst;

      if (CharacterLanguageModel != nullptr) {
        CharFst = std::unique_ptr<LogVectorFst>(new LogVectorFst);
        SampleLib::ComposeAndSampleFromInputAndAddCharLM(
          &InputFileData.GetInputFsts().at(CurrentIndex),
          CharacterLanguageModelFST.get(),
          CharFst.get(),
          &InputFileData.GetWordEndTransducer(),
          &Timer.tInSamples[IdxThread]
        );
        InputFst = CharFst.get();
      } else {
        InputFst = &InputFileData.GetInputFsts().at(CurrentIndex);
      }

      SampleLib::ComposeAndSampleFromInputLexiconAndLM(
        InputFst,
        LexiconTransducer,
        LanguageModel,
        SentEndWordId,
        &SampledFsts[CurrentIndex],
        &Timer.tInSamples[IdxThread],
        Params.BeamWidth,
        UseViterby
      );
    };

    for (std::size_t IdxThread = 0; IdxThread < (NumThreads - 1); ++IdxThread) {
      Threads[IdxThread] = std::thread(SampleFn, IdxSentence, IdxThread);
    }
    SampleFn(IdxSentence, NumThreads - 1);

    // join threads
    for (std::size_t IdxThread = 0; IdxThread < (NumThreads - 1); ++IdxThread) {
      Threads[IdxThread].join();
    }
    Timer.tSample.AddTimeSinceStartToDuration();
//     std::cout << "End compose and sample from input lexicon and lm" << std::endl << std::flush;

    // parse and add sample
    Timer.tParseAndAdd.SetStart();
    for (std::size_t IdxThread = 0; IdxThread < NumThreads; ++IdxThread) {
      std::size_t CurrentIndex = ShuffledIndices[IdxSentence + IdxThread];
//       std::cout << SampledFsts[CurrentIndex].NumStates() << " States" << std::endl << std::flush;
      ParseLib::ParseSampleAndAddCharacterIdSequenceToDictionaryLexFstAndLM(
        SampledFsts[CurrentIndex],
        SentEndWordId,
        LanguageModel,
        LexiconTransducer,
        &SampledSentences[CurrentIndex],
        &TimedSampledSentences[CurrentIndex],
        InputFileData.GetInputArcInfos()
      );
      if (CharacterLanguageModel != nullptr) {
        ParseLib::AddWordSequenceToAddCharLM(
          SampledSentences.at(CurrentIndex).begin() + WHPYLMContextLength,
          SampledSentences.at(CurrentIndex).size() - WHPYLMContextLength,
          *LanguageModel,
          CharacterLanguageModel
        );
      }
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
  int NewKnownN,
  int NewAddCharN
)
{
  std::cout << " Switching to KnownN=" << NewKnownN
            << ", UnkN=" << NewUnkN;
  if (NewAddCharN > 0) {
    std::cout << ", AddCharN=" << NewAddCharN;
  }
  std::cout << std::endl;

  // save previous language model and delete additional character language model
  delete CharacterLanguageModel;
  CharacterLanguageModel = nullptr;
  NHPYLM *OldLanguageModel = LanguageModel;
  std::size_t OldWHPYLMContextLength = WHPYLMContextLength;

  // instantiate new language model and initialize
  InitializeLanguageModel(NewUnkN, NewKnownN, NewAddCharN);

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

    LanguageModel->AddWordSequenceToLm(Sentence);
    if (CharacterLanguageModel != nullptr) {
      ParseLib::AddWordSequenceToAddCharLM(
        Sentence.begin() + WHPYLMContextLength,
        Sentence.size() - WHPYLMContextLength,
        *LanguageModel,
        CharacterLanguageModel
      );
    }
  }

  // retrain language model for some iterations
  TrainLanguageModel(SampledSentences, Params.NewLMNumIters);

  // calculate and update word length statistics
  WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
    LanguageModel,
    Params.WordLengthModulation
  );

  // resample hyperparameters of language model
  LanguageModel->ResampleHyperParameters();

  // cleanup: delete old language model
  delete OldLanguageModel;
}

void LatticeWordSegmentation::InitializeLanguageModel(
  int UnkN,
  int KnownN,
  int AddCharN
)
{
  std::cout << " Initializing empty language model with KnownN="
            << KnownN << ", UnkN=" << UnkN;
  if (AddCharN > 0) {
    std::cout << ", AddCharN=" << AddCharN;
  }
  std::cout << "!" << std::endl << std::endl;

  LanguageModel = new NHPYLM(UnkN, KnownN,
      InputFileData.GetInputIntToStringVector(), CHARACTERSBEGIN);

  AvailChars.resize(
    InputFileData.GetInputIntToStringVector().size() - SENTEND_SYMBOLID);

  std::iota(AvailChars.begin(), AvailChars.end(), SENTEND_SYMBOLID);

  LanguageModel->AddCharacterIdSequenceToDictionary(
      std::vector<int>(1, SENTEND_SYMBOLID).begin(), 1);

  SentEndWordId = LanguageModel->GetWordId(
      std::vector<int>(1, SENTEND_SYMBOLID).begin(), 1);

  WHPYLMContextLength = LanguageModel->GetWHPYLMOrder() - 1;

  CharacterLanguageModel = nullptr;
  if (AddCharN > 0) {
    auto NumCharacters = InputFileData.GetInputIntToStringVector().size()
                         - CHARACTERSBEGIN;
    CharacterLanguageModel = new NHPYLM(0, AddCharN,
        InputFileData.GetInputIntToStringVector(), CHARACTERSBEGIN,
        1.0/(NumCharacters + 2));
  }

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
    std::cerr << "\r  Sentence: " << IdxInitFst + 1
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
    if (CharacterLanguageModel != nullptr) {
      ParseLib::AddWordSequenceToAddCharLM(
        InitializationSentence.begin() + WHPYLMContextLength,
        InitializationSentence.size() - WHPYLMContextLength,
        *LanguageModel,
        CharacterLanguageModel
      );
    }

    IdxInitFst++;
  }
  std::cout << std::endl << std::endl;

  // calculate and update word length statistics
  WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
    LanguageModel,
    Params.WordLengthModulation
  );

  // resample hyperparameters of language model
  LanguageModel->ResampleHyperParameters();
  if (CharacterLanguageModel != nullptr) {
    CharacterLanguageModel->ResampleHyperParameters();
  }

  // get perplexity
  DebugLib::PrintSentencesPerplexity(InitializationSentences, *LanguageModel);

  // output some language model stats
  DebugLib::PrintLanguageModelStats(*LanguageModel);
  if (CharacterLanguageModel != nullptr) {
    DebugLib::PrintLanguageModelStats(*CharacterLanguageModel);
  }
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
    std::shuffle(
      ShuffledIndices.begin(),
      ShuffledIndices.end(),
      RandomGenerator
    );

    // remove and add sentences again
    for (auto IdxInitFst : ShuffledIndices) {
      std::cerr << "\r  Iteration: " << LMTrainIter + 1
                << " of " << MaxNumLMTrainIter << ", Sentence: "
                << IdxInitFst + 1 << " of " << Sentences.size();

      LanguageModel->RemoveWordSequenceFromLm(Sentences.at(IdxInitFst));
      LanguageModel->AddWordSequenceToLm(Sentences.at(IdxInitFst));

      if (CharacterLanguageModel != nullptr) {
        ParseLib::RemoveWordSequenceFromAddCharLM(
          Sentences.at(IdxInitFst).begin() + WHPYLMContextLength,
          Sentences.at(IdxInitFst).size() - WHPYLMContextLength,
          *LanguageModel,
          CharacterLanguageModel
        );
        ParseLib::AddWordSequenceToAddCharLM(
          Sentences.at(IdxInitFst).begin() + WHPYLMContextLength,
          Sentences.at(IdxInitFst).size() - WHPYLMContextLength,
          *LanguageModel,
          CharacterLanguageModel
        );
      }
    }
    std::cout << std::endl;

    // calculate and update word length statistics
    WordLengthProbCalculator::UpdateWHPYLMBaseProbabilitiesScale(
      LanguageModel,
      Params.WordLengthModulation
    );

    // resample hyperparameters of language model
    LanguageModel->ResampleHyperParameters();
    if (CharacterLanguageModel != nullptr) {
      CharacterLanguageModel->ResampleHyperParameters();
    }

    // get perplexity
    DebugLib::PrintSentencesPerplexity(Sentences, *LanguageModel);

    // output some language model stats
    DebugLib::PrintLanguageModelStats(*LanguageModel);
    if (CharacterLanguageModel != nullptr) {
      DebugLib::PrintLanguageModelStats(*CharacterLanguageModel);
    }
  }
  std::cout << std::endl;
}

void LatticeWordSegmentation::WriteRescoredLattices() {

  NHPYLMFst CharLMFst(*CharacterLanguageModel, EOW,
      std::vector<bool>(), true, AvailChars);

  const auto kInputSize = InputFileData.GetInputFsts().size();
  for (std::size_t inputIdx = 0; inputIdx < kInputSize; inputIdx++) {

    // compose input with language model
    auto InputFst = InputFileData.GetInputFsts()[inputIdx];
    fst::ArcSort(&InputFst, fst::OLabelCompare<fst::LogArc>());
    PM* PM1 = new PM(InputFst, fst::MATCH_NONE);
    PM* PM2 = new PM(CharLMFst, fst::MATCH_INPUT, PHI_SYMBOLID, false);
    fst::ComposeFstOptions<fst::LogArc, PM> copts2(fst::CacheOptions(), PM1, PM2);
    fst::ComposeFst<fst::LogArc> RescoredInput (InputFst, CharLMFst, copts2);

    // clean up output lattice: Remove wordend marker and epsilons

    // build filename and write lattice in fst format
    // It is easier to take care of the text format convertion from shell level.
    auto RescoredFilename = DebugLib::BuildFilename(
        Params.RescoredLatticesDirectoryName,
        InputFileData.GetInputFileNames()[inputIdx],
        "_rescored.fst");
    DebugLib::WriteOpenFSTLattice(fst::VectorFst<fst::LogArc>(RescoredInput),
                                  RescoredFilename);
  }
}
