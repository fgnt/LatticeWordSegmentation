// ----------------------------------------------------------------------------
/**
   File: DebugLib.hpp

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

   Description: some debuging functions

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _DEBUGLIB_HPP_
#define _DEBUGLIB_HPP_

#include <fst/vector-fst.h>
#include <sys/stat.h>
#include "NHPYLM/NHPYLM.hpp"
#include "definitions.hpp"


/* library including some debug functions */
class DebugLib {
public:
  // print transitions output recursively
  static void PrintTransitions(
    const HPYLM::ContextToContextTransitions &Transitions,
    unsigned int CurrentContextId,
    std::vector< bool > &VisitedContextIds,
    const NHPYLM &LanguageModel,
    int SentEndWordId
  );
  
  // print sentence with character sequences
  static void PrintSentence(
    const const_witerator &SentenceBegin,
    int NumWords,
    const std::vector<std::string> &Id2CharacterSequence
  );
  
  static void PrintEditDistanceStatistics(
    const std::vector< int > &SegmentationInsDelSubCorrNFoundNRef,
    const std::string &Description,
    const std::string &Name
  );
  
  static void PrintLexiconStatistics(
    const std::vector<int> &LexiconCorrNFoundNRef
  );
  
  static void PrintSentencesPerplexity(
    const std::vector<std::vector<int> > &Sentences,
    const NHPYLM &LanguageModel
  );
  
  static void PrintLanguageModelStats(
    const NHPYLM &LanguageModel
  );
  
  static void PrintVectorOfInts(
    const std::vector<int> &VectorOfInts,
    int Width,
    const std::string &Description,
    const std::string &Postfix
  );
  
  static void PrintVectorOfDoubles(
    const std::vector<double> &VectorOfDoubles,
    int Width,
    const std::string &Description,
    const std::string &Postfix
  );
  
  // write openfst lattices
  static void WriteOpenFSTLattice(
    const fst::VectorFst<fst::LogArc> &fst,
    const string &FileName);
  
  // write symbols to symbolfile
  static void WriteSymbols(
    const string &fileName,
    const std::vector<std::string> &words,
    SymbolWriteModes SymbolWriteMode
  );
  
  // recursive directory creating
  static void CreateDirectoryRecursively(
    const string &DirectoryName,
    size_t PathSeperatorPosition = string::npos
  );
  
  // print given fst to file or generate pdf graphic for fst
  static void PrintFST(
    const string &pFileName,
    const std::vector<std::string> &words,
    const fst::VectorFst<fst::LogArc> &fst,
    bool printPDF,
    SymbolWriteModes SymbolWriteMode
  );
  
  // print given sentence to text file
  static void PrintSentencesToFile(
    const std::string &filename,
    const std::vector<std::vector<int> > &Sentences,
    const std::vector<std::string> &Id2CharacterSequence
  );
  
  // print given sentence with timings to text file
  static void PrintTimedSentencesToFile(
    const std::string &FileName,
    const std::vector<std::vector<ArcInfo> > &TimedSentences,
    const std::vector<std::string> &Id2CharacterSequence,
    const std::vector<std::string> &InputFileNames
  );
  
  static void GenerateSentencesOfWordsFromCharLM(
    const NHPYLM& LanguageModel
  );
  
  static void GenerateSentencesOfWordsFromWordLM(
    const NHPYLM& LanguageModel,
    int SentEndWordId
  );
};

#endif