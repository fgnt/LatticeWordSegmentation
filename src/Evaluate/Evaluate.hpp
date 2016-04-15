// ----------------------------------------------------------------------------
/**
   File: Evaluate.hpp

   Status:         Version 1.0
   Language: C++

   License: UPB licence

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

   E-Mail: glarner@nt.uni-paderborn.de

   Description: io class for lattice reading, processing and writing

   Limitations: -

   Change History:
   Date         Author       Description
   2016         Glarner      Initial
*/
// ----------------------------------------------------------------------------
#ifndef _EVALUATE_HPP_
#define _EVALUATE_HPP_

#include "../FileReader/FileData.hpp"
#include "../ParameterParser/ParameterParser.hpp"
#include "../LatticeWordSegmentationTimer.hpp"
#include "../NHPYLM/NHPYLM.hpp"

class Evaluate{
  const ParameterStruct& Params;
  const FileData& InputFileData;
  LatticeWordSegmentationTimer& Timer;
  const NHPYLM* LanguageModel;

  /* internal functions */
  void OutputPhonemeErrorRate(
    const std::vector<LogVectorFst>& SampledFsts,
    bool OutputEditOperations,
    std::size_t IdxIter
  );

  void OutputWordErrorRate(
    const std::vector<std::vector<int>>& SampledSentences,
    bool OutputEditOperations,
    std::size_t IdxIter
  );

  std::string BuildPrefix(
    std::string specifier,
    std::size_t IdxIter
  );

public:
  /* constructor */
  Evaluate(
    const ParameterStruct& Params,
    const FileData& InputFileData,
    LatticeWordSegmentationTimer& Timer,
    const NHPYLM* LanguageModel
  ):
    Params(Params),
    InputFileData(InputFileData),
    Timer(Timer),
    LanguageModel(LanguageModel) {};


  /* interface */
  void WriteSentencesToOutputFiles(
    const std::vector<std::vector<int>>& SampledSentences,
    const std::vector<std::vector<ArcInfo>>& TimedSampledSentences,
    std::size_t IdxIter
  );

  void OutputMeasureStatistics(
    const std::vector<std::vector<int>>& SampledSentences,
    const std::vector<LogVectorFst>& SampledFsts,
    std::size_t IdxIter
  );
};


#endif // _EVALUATE_HPP_
