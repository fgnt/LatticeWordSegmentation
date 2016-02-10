// ----------------------------------------------------------------------------
/**
   File: FileReader.hpp

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

   Description: io class for lattice reading, processing and writing

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _FILEREADER_HPP_
#define _FILEREADER_HPP_

#include <fst/vector-fst.h>
#include "StringToIntMapper.hpp"

/* class to read input files */
class FileReader {
  StringToIntMapper GlobalStringToInt;                // a global string to int mapper which is updated with each reading process

  StringToIntMapper InitStringToInt;                  // character sequence to int mapper (for input characters)
  vector<fst::VectorFst<fst::LogArc> > InitFsts;      // stores the init Fst
  std::vector<std::string> InitFileNames;             // filename of read initialization files

  StringToIntMapper InputStringToInt;                 // character sequence to int mapper (for input characters)
  vector<fst::VectorFst<fst::LogArc> > InputFsts;     // stores the input Fst
  std::vector<std::string> InputFileNames;            // filename of read input files

  StringToIntMapper ReferenceStringToInt;             // character sequence to int mapper (for input characters)
  vector<fst::VectorFst<fst::LogArc> > ReferenceFsts; // stores the reference Fst
  std::vector<std::string> ReferenceFileNames;        // filenames of read reference files

  /* internal functions: input */
  void ReadPosteriorBin(const std::vector<std::string> &InputFiles, double PruningFactor);  // read posteriors from lstm-toolkit
  void ReadCMULattices(const std::vector<std::string> &InputFiles, double PruningFactor);     // read lattices in cmu format
  void ReadHTKLattices(const std::vector<std::string> &InputFiles, double PruningFactor, double HTKLMScale);     // read lattices in htk format
  void ReadOpenFSTLattices(const std::vector<std::string> &InputFiles, double PruningFactor); // read lattices in openfst format
  void ReadTextFiles(const std::vector<std::string > &InputFiles, std::vector<fst::VectorFst<fst::LogArc> > *InputFsts, std::vector<std::string> *FileNames); // read from text files

  /* internal functions: output */
  inline static void WriteOpenFSTLattice(const fst::VectorFst<fst::LogArc> &fst, const string &FileName);                           // write openfst lattices
  inline static void WriteSymbols(const string &fileName, const std::vector<std::string> &words, SymbolWriteModes SymbolWriteMode); // write symbols to symbolfile
  inline static void CreateDirectoryRecursively(const string &DirectoryName, size_t PathSeperatorPosition = string::npos);          // recursive directory creating

public:
  /* constructor */
  FileReader(); // set some predefined symbols in string to int map

  /* interface */
  void ReadSymbols(const std::string &SymbolFile);                                                                                                                               // read the symbols file mapping strings to integers
  void ReadInitTranscription(const std::string &InitFile);                                                                                                                       // read reference transcription from file InitFile
  void ReadInputFilesFromList(const std::vector<std::string> &InputFiles, LatticeFileTypes LatticeFileType, double PruningFactor, double HTKLMScale);                            // read files from list of files
  void PruneLattices(double PruningFactor);                                                                                                                                      // apply prining to all lattices
  void ReadReferenceTranscription(const std::string &ReferenceFile);                                                                                                             // read reference transcription from file ReferenceFile

  void WriteOpenFSTLattices(const string &Prefix, const string &Postfix) const;                                                                                                  // write lattices to output files in openfst format

  void ApplyAcousticModelScalingFactor(double AcousticModelScalingFactor);                                                                                                       // do the acoustic model scaling
  void ApplyWordEndTransducer();                                                                                                                                                 // apply the word end transducer to all fsts
  void ApplySentEndTransducer();                                                                                                                                                 // apply the sentence end transduer to all fsts

  const std::vector<std::string> &GetInputIntToStringVector() const;                                                                                                             // get vector mapping integer to string (characters)
  const std::vector<std::string> &GetReferenceIntToStringVector() const;                                                                                                         // get vector mapping integer to string for the refernce transcription (characters)
  const std::vector<std::string> &GetInitIntToStringVector() const;                                                                                                              // get vector mapping integer to string for the init transcription (characters)
  const std::vector<fst::VectorFst<fst::LogArc>> &GetInputFsts() const;                                                                                                          // return all input fsts
  const std::vector<fst::VectorFst<fst::LogArc>> &GetReferenceFsts() const;                                                                                                      // return all reference fsts
  const std::vector<fst::VectorFst<fst::LogArc>> &GetInitFsts() const;                                                                                                           // return all init fsts
  const std::vector<std::string> &GetInputFileNames() const;                                                                                                                     // return filenames of input fsts
  const std::vector<std::string> &GetReferenceFileNames() const;                                                                                                                 // return filenames of reference fsts
  const std::vector<std::string> &GetInitFileNames() const;                                                                                                                      // return filenames of init fsts

  /* some helper functions */
  static void PrintFST(const string &pFileName, const std::vector<std::string> &words, const fst::VectorFst<fst::LogArc> &fst, bool printPDF, SymbolWriteModes SymbolWriteMode); // print given fst to file or generate pdf graphic for fst
  static void PrintSentencesToFile(const std::string &filename, const std::vector<std::vector<int> > &Sentences, const std::vector<std::string> &Id2CharacterSequence);          // print given sentence to text file
};

#endif