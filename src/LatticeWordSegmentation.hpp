// ----------------------------------------------------------------------------
/**
   File: LatticeWordSegmentation.hpp

   Status:         Version 1.0
   Language: C++

   License: UPB licence

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

   E-Mail: walter@nt.uni-paderborn.de

   Description: the word segmenter

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _LATTICEWORDSEGEMNTATION_HPP_
#define _LATTICEWORDSEGEMNTATION_HPP_

#include <thread>
#include "ParameterParser/ParameterParser.hpp"
#include "FileReader/FileData.hpp"
#include "NHPYLM/NHPYLM.hpp"
#include "LatticeWordSegmentationTimer.hpp"
#include "LexFst.hpp"

/* main class for the word segmentation */
class LatticeWordSegmentation {

  static std::default_random_engine RandomGenerator; // Uniform random generator

  /* parameter and input data structures */
  const ParameterStruct& Params; // struct with parameters
  const FileData& InputFileData; // class with input data

  /* some general variables */
  const std::size_t MaxNumThreads;    // Maximum number of thread to be used
  std::vector<std::thread> Threads;   // the thread objects
  LatticeWordSegmentationTimer Timer; // object to do some timing

  /* language model and dictionary */
  NHPYLM *LanguageModel;           // the language model
  std::size_t SentEndWordId;       // the sentence end word id
  std::size_t WHPYLMContextLength; // the context length for the whpylm with given order

  /* sampling data */
  std::size_t NumSampledSentences;                          // number of sentences in input
  std::vector<LogVectorFst > SampledFsts;                   // the sampled fsts
  std::vector<std::vector<int> > SampledSentences;          // the segmented sentences (parsed samples)
  std::vector<std::vector<ArcInfo> > TimedSampledSentences; // the segmented sentences (parsed samples with start/end times on word basis)

  /* init data */
  std::size_t NumInitializationSentences;                 // number of sentences for initialization
  std::vector<std::vector<int> > InitializationSentences; // initialization sentences for language model initialization


  /* internal functions */
  // initialize a language model
  void InitializeLanguageModel(
    int NewUnkN,
    int NewKnownN
  );
  
  // initialize with initiliazation fsts
  void ParseInitializationSentencesAndInitializeLanguageModel();
  
  // train the language model from given sentences
  void TrainLanguageModel(
    const std::vector<std::vector< int > > &Sentences,
    std::size_t MaxNumLMTrainIter
  );
  
  // iterate over sentences
  void DoWordSegmentationSentenceIterations(
    const std::vector< int > &ShuffledIndices,
    LexFst *LexiconTransducer,
    std::size_t IdxIter
  );

  // switch to a new language  model order
  void SwitchLanguageModelOrders(int NewUnkN, int NewKnownN);

public:
  /* constructor */
  LatticeWordSegmentation(
    const ParameterStruct& Params,
    const FileData& InputFileData
  );


  /* interface */
  // run the actual word segmentation iterations
  void DoWordSegmentation();
};

#endif
