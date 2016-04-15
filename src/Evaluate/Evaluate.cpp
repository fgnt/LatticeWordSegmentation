// ----------------------------------------------------------------------------
/**
   File: Evaluate.cpp
   Copyright (c) <2016> <University of Paderborn>
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


   Author: Thomas Glarner
*/
// ----------------------------------------------------------------------------
#include "Evaluate.hpp"
#include "../DebugLib.hpp"
#include "../EditDistanceCalculator/WERCalculator.hpp"
#include "../EditDistanceCalculator/PERCalculator.hpp"
#include "../EditDistanceCalculator/LPERCalculator.hpp"



void Evaluate::WriteSentencesToOutputFiles(
    const std::vector<std::vector<int>>& SampledSentences,
    const std::vector<std::vector<ArcInfo>>& TimedSampledSentences,
    std::size_t IdxIter)
{
  // write to output
  DebugLib::PrintSentencesToFile(
    BuildPrefix("Sentences", IdxIter),
    SampledSentences,
    LanguageModel->GetId2CharacterSequenceVector()
  );

  if (!InputFileData.GetInputArcInfos().empty()) {
    DebugLib::PrintTimedSentencesToFile(
      BuildPrefix("TimedSentences", IdxIter),
      TimedSampledSentences,
      LanguageModel->GetId2CharacterSequenceVector(),
      InputFileData.GetInputFileNames()
    );
  }
}

/******************************************************************************
 * Function to output some evaluation statistics:
 * - Sentences perplexity
 * - Word error rate and segmentation performance including Lexicon performance
 *   (F-score, recall, precision, and more)
 * - Phoneme error rate
 * - Language model statistics
 * - Timing information
 ******************************************************************************/
void Evaluate::OutputMeasureStatistics(
    const std::vector<std::vector<int>>& SampledSentences,
    const std::vector<LogVectorFst>& SampledFsts,
    std::size_t IdxIter)
{
    // get perplexity
    Timer.tCalcPerplexity.SetStart();
    DebugLib::PrintSentencesPerplexity(SampledSentences, *LanguageModel);
    Timer.tCalcPerplexity.AddTimeSinceStartToDuration();

    bool OutputEditOperations =
      Params.OutputEditOperations && ((IdxIter == (Params.NumIter - 1)) ||
      (IdxIter == (Params.DeactivateCharacterModel - 1)) ||
      (IdxIter == (Params.UseViterby - 2)));

    // calculate word error rate
    if (Params.CalculateWER && ((IdxIter % Params.EvalInterval) == 0)) {
      Timer.tCalcWER.SetStart();
      OutputWordErrorRate(SampledSentences, OutputEditOperations, IdxIter);
      Timer.tCalcWER.AddTimeSinceStartToDuration();
    }

    // calculate phoneme error rate
    if (Params.CalculatePER && ((IdxIter % Params.EvalInterval) == 0)) {
      Timer.tCalcPER.SetStart();
      OutputPhonemeErrorRate(SampledFsts, OutputEditOperations, IdxIter);
      Timer.tCalcPER.AddTimeSinceStartToDuration();
    }

    // output some language model stats
    DebugLib::PrintLanguageModelStats(*LanguageModel);

    // output some timing statistics
    Timer.PrintTimingStatistics();
}

void Evaluate::OutputWordErrorRate(
    const std::vector<std::vector<int>>& SampledSentences,
    bool OutputEditOperations,
    std::size_t IdxIter)
{
//       std::cout << "Start WER calculation" << std::endl;
  std::size_t WHPYLMContextLength = LanguageModel->GetWHPYLMOrder() - 1;

  WERCalculator SegStatsCalculator(
    SampledSentences,
    InputFileData.GetReferenceFsts(),
    *LanguageModel,
    WHPYLMContextLength,
    1,
    InputFileData.GetInputFileNames(),
    BuildPrefix("WER", IdxIter) + "_",
    OutputEditOperations
  );
  //       std::cout << "Finished WER calculation" << std::endl;
  DebugLib::PrintEditDistanceStatistics(
      SegStatsCalculator.GetInsDelSubCorrNFoundNRef(),
      "Word error rate",
      "WER");

  DebugLib::PrintLexiconStatistics(
      SegStatsCalculator.GetLexiconCorrNFoundNRef());
}

void Evaluate::OutputPhonemeErrorRate(
      const std::vector<LogVectorFst>& SampledFsts,
      bool OutputEditOperations,
      std::size_t IdxIter)
{
    PERCalculator PhonemeStatsCalculator(
      SampledFsts,
      InputFileData.GetReferenceFsts(),
      InputFileData.GetReferenceIntToStringVector(),
      Params.NoThreads,
      InputFileData.GetInputFileNames(),
      BuildPrefix("PER", IdxIter) + "_",
      OutputEditOperations,
      InputFileData.GetInputArcInfos()
    );

    DebugLib::PrintEditDistanceStatistics(
      PhonemeStatsCalculator.GetInsDelSubCorrNFoundNRef(),
      "Phoneme error rate",
      "PER"
    );
}

std::string Evaluate::BuildPrefix(std::string specifier, std::size_t IdxIter)
{
  return Params.OutputDirectoryBasename +
      "KnownN_" + std::to_string(LanguageModel->GetWHPYLMOrder()) +
      "_UnkN_" + std::to_string(LanguageModel->GetCHPYLMOrder()) +
      "/" + Params.OutputFilesBasename + specifier +
      "_Iter_" + std::to_string(IdxIter + 1);
}
