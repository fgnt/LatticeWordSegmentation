// ----------------------------------------------------------------------------
/**
   File: ParameterParser.hpp

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

   Description: simple parameter parser

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _PARAMETERPARSER_HPP_
#define _PARAMETERPARSER_HPP_

#include "definitions.hpp"

/* Struct holding values of input parameters */
struct ParameterStruct {
  unsigned int NumBurnIn;
  unsigned int TrimRate;
  unsigned int NumSamples;
  unsigned int SampleRate;
  unsigned int SampleInputRate;
  unsigned int DecodeMethod;
  unsigned int KnownN;                 // order of word hierarchical language model (Parameter: -KnownN N (1))
  unsigned int UnkN;                   // order of character hierarchical language model (Parameter: -UnkN N (1))
  unsigned int NoThreads;              // number of threads used for sampling (Parameter: -NoThreads N (1))
  double PruneFactor;                  // prune paths that have an PruneFactor times higher score that the lowest scoring path (Parameter: -PruneFactor X (inf))
  std::string InputFilesList;          // Filelist for input files (Parameter: -InputFilesList InputFileListName ())
  InputTypes InputType;                // type of input (Parameter: -InputType [text|fst] (text))
  std::string SymbolFile;              // symbolfile if reading from openfst lattices (Parameter: -SymbolFile SymbolfileName ())
  std::string InputArcInfosFile;       // arc infos file if reading from openfst lattices where input id points to label, start, end infos (Parameter: -InputArcInfosFile InputArcInfosFile ())
  std::string Prefix;
  std::string Separator;
  float ErrorRate;
  double AmScale;                      // acoustic model scaling factor (Parameter: -AmScale AcousticModelScalingDactor (1))
  float MaxErrorProb;
  unsigned int Debug;                  // configure debugging (Parameter: -Debug N (0))
  bool DrawFsts;
  unsigned int MaxLen;
  std::string PhoneAltFile;
  std::string DictFile;
  bool UseDictFile;
  unsigned int SampleInput;
  unsigned int SampleThreads;
  bool Deletions;
  bool AltInit;
  bool FullInit;
  LatticeFileTypes LatticeFileType;    // format of lattice files (Parameter: -LatticeFileType [cmu|htk|opefst] (text))
  unsigned int DictConstraint;
  bool ErrCorrection;
  unsigned int ErrStart;
  double RepProb;
  unsigned int InsCnt;
  bool CacheLattice;
  bool ExportLattices;                 // specify weather lattices should be exported to openfst format (Parameter: -ExportLattices ExportLatticesDirectoryName)
  bool AltLM;
  unsigned int SwitchIter;             // iteration before which the language model orders are switched (Parameter: -SwitchIter SwitchIterIdx NewKnownN NewUnkN NewLMNumIters (0 1 1 0))
  unsigned int NewKnownN;              // new word language model order
  unsigned int NewUnkN;                // new character language model order
  unsigned int NewLMNumIters;          // number of iteration for language model retraining after switching
  unsigned int NewAltLMN;
  double NewAmScale;
  unsigned int AltN;
  bool InitLM;                         // initialize language model from initialization fsts (Parameter: -InitLM InitTranscriptionFilename ())
  std::string InitTranscription;       // File containing the transcriptions used for initialization
  int InitLmNumIterations;             // Number of iterations for language model initialization (Parameter: -InitLmNumIterations NumIterations (0))
  std::string MLFFileName;
  bool UseMlf;
  bool Save;
  bool AddAll;
  bool CopyLex;
  bool Connect;
  std::vector<std::string> InputFiles; // actual list of files to read from (either read from command line argumants or InputFilesList ())
  unsigned int NumIter;                // maximum number of iterations (Parameter: -NumIter N (0))
  std::string ExportLatticesDirectoryName;     // name for lattice export directory (Parameter: -ExportLattices ExportLatticesDirectoryName)
  std::string OutputDirectoryBasename; // basename for result outpt directory (Parameter: -OutputDirectoryBasename OutputDirectoryBasename ())
  std::string OutputFilesBasename;     // basename for result outpt files (Parameter: -OutputFilesBasename OutputFilesBasename ())
  std::string ReferenceTranscription;  // File containing the reference transcriptions (Parameter: -ReferenceTranscription ReferenceTranscriptionFilename ())
  bool UseReferenceTranscription;      // specify if reference transcriptions are available
  bool CalculatePER;                   // Calculate phoneme error rate (Parameter: -CalculatePER (false))
  bool CalculateWER;                   // Calculate word error rate (Parameter: -CalculateWER (false))
  bool CalculateLPER;                  // Calcualte lattice phoneme error rate (Parameter: -CalculateLPER (false))
  double PruningStart;                 // start pruning value for lper calculation
  double PruningStep;                  // stepsize to increase pruning during lper calculation (Parameter: -PruningStep PruningStart PruningStep PruningEnd (0 1 1 0))
  double PruningEnd;                   // end pruning valur for lper calculation
  int BeamWidth;                       // Beam width when composing the FSTs. -1 disables all pruning (Parameter: -BeamWidth Beamwidth (-1))
  bool OutputEditOperations;           // Output edit operations after LPER, PER and WER calculation (Parameter: -OutputEditOperations (false))
  int EvalInterval;                    // Evaluation interval (Parameter: -EvalInterval EvalInterval (1))
  double WordLengthModulation;         // Set word length modulation. -1: off, 0: automatic, >0 set mean word length (Paremter: -WordLengthModulation WordLength)
  unsigned int UseViterby;                      // Set iteration to switch to viterby decoding. 0: off, >0 number of iteration (Patameter: -UseVitery NumIter)
  unsigned int DeactivateCharacterModel;        // Set iteration to deactivate character model. 0: off, >0 number of iteration (Patameter: -DeactivateCharacterModel NumIter)
  double HTKLMScale;                    // Language model scaling factor when reading HTK lattices (Parameter: -HTKLMScale K (0))
  double AMScoreShift;                  // Normalize AM scores with additive shift constant
  bool ReadNodeTimes;                   // Read node timing informations from HTK lattice

  ParameterStruct(); // constructor to set default values
};

/* Parser for input parameters */
class ParameterParser {
  ParameterStruct Parameters; // input parameters


  /* internal functions */
  // helper function to print help and terminate on error
  void DieOnHelp(
    const std::string& err=""
  ) const;
  
  // read name of files form a filelist
  void ReadFilesFromFileList(
    const std::string &InputFileList
  );
  
  // read name of files from input parameters
  void ReadFilesFromParameters(
    int argc,
    const char **argv
  );

public:
  /* constructor */
  // constructor called with input parameters
  ParameterParser(int argc, const char **argv);

  
  /* interface */
  // return parameter struct
  const ParameterStruct &GetParameters() const;
};

#endif
