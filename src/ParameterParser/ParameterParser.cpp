// ----------------------------------------------------------------------------
/**
   File: ParameterParser.cpp
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
#include <thread>
#include "ParameterParser.hpp"

ParameterParser::ParameterParser(int argc, const char **argv)
{
  // version output
  std::cout << "-----------------------------------------------------------------------------------------------------------" << std::endl;
  std::cout << "LatticeWordSegmentation: build date: " << __DATE__ << " - time: " << __TIME__ << std::endl;
  std::cout << "-----------------------------------------------------------------------------------------------------------" << std::endl;

  if(argc < 2) {
    DieOnHelp();
  }
  // read the arguments
  int argPos = 1;
  for (; argPos < argc && argv[argPos][0] == '-'; argPos++) {
    if (!strcmp(argv[argPos], "-burnin")) {
      Parameters.NumBurnIn = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-trimRate")) {
      Parameters.TrimRate = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-samps")) {
      Parameters.NumSamples = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-samprate")) {
      Parameters.SampleRate = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-sampInputRate")) {
      Parameters.SampleInputRate = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-decodeMethod")) {
      Parameters.DecodeMethod = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-KnownN")) {
      Parameters.KnownN = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-UnkN")) {
      Parameters.UnkN = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-NoThreads")) {
      Parameters.NoThreads = atoi(argv[++argPos]);
      if (Parameters.NoThreads == 0) {
        Parameters.NoThreads = std::thread::hardware_concurrency();
        std::cout << " Running with " << Parameters.NoThreads << " Threads" << std::endl << std::endl;
      }
    } else if (!strcmp(argv[argPos], "-PruneFactor")) {
      Parameters.PruneFactor = atof(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-InputFilesList")) {
      Parameters.InputFilesList = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-InputType")) {
      ++argPos;
      if (!strcmp("fst", argv[argPos])) {
        Parameters.InputType = INPUT_FST;
      } else if (!strcmp("text", argv[argPos])) {
        Parameters.InputType = INPUT_TEXT;
      } else {
        std::ostringstream err;
        err << "Bad input type '" << argv[argPos] << "'";
        DieOnHelp(err.str());
      }
    } else if (!strcmp(argv[argPos], "-SymbolFile")) {
      Parameters.SymbolFile = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-InputArcInfosFile")) {
      Parameters.InputArcInfosFile = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-prefix")) {
      Parameters.Prefix = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-separator")) {
      Parameters.Separator = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-errorRate")) {
      Parameters.ErrorRate = atoi(argv[++argPos]) / 100.0;
    } else if (!strcmp(argv[argPos], "-AmScale")) {
      Parameters.AmScale = atof(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-maxErrorProb")) {
      Parameters.MaxErrorProb = atoi(argv[++argPos]) / 100.0;
    } else if (!strcmp(argv[argPos], "-Debug")) {
      Parameters.Debug = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-drawFsts")) {
      Parameters.DrawFsts = true;
    } else if (!strcmp(argv[argPos], "-WordLengthModulation")) {
      Parameters.WordLengthModulation = atof(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-phoneAlt")) {
      Parameters.PhoneAltFile = std::string(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-dict")) {
      Parameters.DictFile = std::string(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-sampleInputs")) {
      Parameters.SampleInput = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-sampleThreads")) {
      Parameters.SampleThreads = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-del")) {
      Parameters.Deletions = true;
    } else if (!strcmp(argv[argPos], "-altInit")) {
      Parameters.AltInit = true;
    } else if (!strcmp(argv[argPos], "-fullInit")) {
      Parameters.FullInit = true;
    } else if (!strcmp(argv[argPos], "-LatticeFileType")) {
      ++argPos;
      if (!strcmp("cmu", argv[argPos])) {
        Parameters.LatticeFileType = CMU_FST;
      } else if (!strcmp("htk", argv[argPos])) {
        Parameters.LatticeFileType = HTK_FST;
      } else if (!strcmp("openfst", argv[argPos])) {
        Parameters.LatticeFileType = OPEN_FST;
      } else {
        std::ostringstream err;
        err << "Bad lattice file type '" << argv[argPos] << "'";
        DieOnHelp(err.str());
      }
    } else if (!strcmp(argv[argPos], "-EvalInterval")) {
      Parameters.EvalInterval =  atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-errCorrection")) {
      Parameters.ErrCorrection = true;
      Parameters.ErrStart = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-repProb")) {
      Parameters.RepProb = atoi(argv[++argPos]) / 100.0;
    } else if (!strcmp(argv[argPos], "-insCnt")) {
      Parameters.InsCnt = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-UseViterby")) {
      Parameters.UseViterby = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-ExportLattices")) {
      Parameters.ExportLattices = true;
      Parameters.ExportLatticesDirectoryName = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-altLM")) {
      Parameters.AltLM = true;
    } else if (!strcmp(argv[argPos], "-SwitchIter")) {
      Parameters.SwitchIter = atoi(argv[++argPos]);
      Parameters.NewKnownN = atoi(argv[++argPos]);
      Parameters.NewUnkN = atoi(argv[++argPos]);
      Parameters.NewLMNumIters = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-DeactivateCharacterModel")) {
      Parameters.DeactivateCharacterModel = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-InitLM")) {
      Parameters.InitLM = true;
      Parameters.InitTranscription = std::string(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-mlf")) {
      Parameters.MLFFileName = std::string(argv[++argPos]);
      Parameters.UseMlf = true;
    } else if (!strcmp(argv[argPos], "-save")) {
      Parameters.Save = true;
    } else if (!strcmp(argv[argPos], "-addAll")) {
      Parameters.AddAll = true;
    } else if (!strcmp(argv[argPos], "-copyLex")) {
      Parameters.CopyLex = true;
    } else if (!strcmp(argv[argPos], "-OutputEditOperations")) {
      Parameters.OutputEditOperations = true;
    } else if (!strcmp(argv[argPos], "-NumIter")) {
      Parameters.NumIter = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-BeamWidth")) {
      Parameters.BeamWidth = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-OutputDirectoryBasename")) {
      Parameters.OutputDirectoryBasename = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-OutputFilesBasename")) {
      Parameters.OutputFilesBasename = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-ReferenceTranscription")) {
      Parameters.UseReferenceTranscription = true;
      Parameters.ReferenceTranscription = argv[++argPos];
    } else if (!strcmp(argv[argPos], "-CalculateLPER")) {
      Parameters.CalculateLPER = true;
    } else if (!strcmp(argv[argPos], "-CalculatePER")) {
      Parameters.CalculatePER = true;
    } else if (!strcmp(argv[argPos], "-CalculateWER")) {
      Parameters.CalculateWER = true;
    } else if (!strcmp(argv[argPos], "-InitLmNumIterations")) {
      Parameters.InitLmNumIterations = atoi(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-HTKLMScale")) {
      Parameters.HTKLMScale = atof(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-PruningStep")) {
      Parameters.PruningStart = atof(argv[++argPos]);
      Parameters.PruningStep = atof(argv[++argPos]);
      Parameters.PruningEnd = atof(argv[++argPos]);
    } else if (!strcmp(argv[argPos], "-ReadNodeTimes")) {
      Parameters.ReadNodeTimes = true;
    } else {
      std::ostringstream err;
      err << "Illegal option: " << argv[argPos];
      DieOnHelp(err.str());
    }
  }

  // Terminate if a PER, LPER or WER shall be calculated
  // without providing a reference transcription
  if ((Parameters.CalculateLPER || Parameters.CalculatePER ||
       Parameters.CalculateWER) && !Parameters.UseReferenceTranscription) {
    std::ostringstream err;
    err << "Illegal option: Without a reference transcription"
        << " LPER, PER or WER cannot be calculated!";
    DieOnHelp(err.str());
  }

  // load the input files, either from the list or from the parameters
  if (!Parameters.InputFilesList.empty()) {
    ReadFilesFromFileList(Parameters.InputFilesList);
  } else {
    ReadFilesFromParameters(argc - argPos, argv + argPos);
  }
}

void ParameterParser::DieOnHelp(const std::string& err) const
{
  std::cout << "---LatticeWordSegmentation---" << std::endl
            << " A tool for learning a language model and a word dictionary" << std::endl
            << " from lattices (or text) using Pitman-Yor language models and" << std::endl
            << " Weighted Finite State Transducers" << std::endl
            << "  By Oliver Walter. Based on code from Graham Neubig and Jahn Heymann" << std::endl
            << "  Contact: walter@nt.uni-paderborn.de, Web: http://nt.uni-paderborn.de/mitarbeiter/oliver-walter/" << std::endl << std::endl
            << " Options:" << std::endl
            << "  -KnownN:               The n-gram length of the word language model (-KnownN N (1))" << std::endl
            << "  -UnkN:                 The n-gram length of the character language model (-UnkN N (1))" << std::endl
            << "  -NoThreads:            The number of threads used for sampling (-NoThreads N (1))" << std::endl
            << "  -PruneFactor:          Prune paths in the input that have a PruneFactor times higher score" << std::endl
            << "                         than the lowest scoring path (-PruneFactor X (inf))" << std::endl
            << "  -InputFilesList:       A list of input files, one file per line.  (-InputFilesList InputFileListName (NULL))" << std::endl
            << "                         For fst input, files must be in OpenFST binary format, Log semiring" << std::endl
            << "                         Text files consist of one sentence per line, each symbol seperated by a whitespace." << std::endl
            << "  -InputType:            The type of the input (-InputType [text|fst] (text))" << std::endl
            << "  -SymbolFile:           The symbolfile if reading from openfst lattices (-SymbolFile SymbolfileName (NULL))" << std::endl
            << "  -InputArcInfosFile:    arc infos file if reading from openfst lattices where input id points to" << std::endl
            << "                         label, start, end infos (Parameter: -InputArcInfosFile InputArcInfosFile ())" << std::endl
            << "  -Debug:                Set debug level (-Debug N (0))" << std::endl
            << "  -LatticeFileType:      Format of lattice files (-LatticeFileType [cmu|htk|openfst] (text))" << std::endl
            << "  -ExportLattices:       Export the input lattices to openfst format (-ExportLattices ExportLatticesDirectoryName)" << std::endl
            << "  -NumIter:              Maximum number of iterations (-NumIter N (0))" << std::endl
            << "  -OutputDirectoryBasename: The basename for result outpt Directory" << std::endl
            << "                            (-OutputDirectoryBasename OutputDirectoryBasename ())" << std::endl
            << "  -OutputFilesBasename:     The basename for result outpt files" << std::endl
            << "                            (-OutputFilesBasename OutputFilesBasename ())" << std::endl
            << "  -ReferenceTranscription:  File containing the reference transcriptions" << std::endl
            << "                            (-ReferenceTranscription ReferenceTranscriptionFilename ())" << std::endl
            << "                            Reference transcriptions have to be in the same order as the input files"  << std::endl
            << "  -CalculateLPER:        Calcualte lattice phoneme error rate (-CalculateLPER (false))"  << std::endl
            << "  -CalculatePER:         Calculate phoneme error rate (-CalculatePER (false))"  << std::endl
            << "  -CalculateWER:         Calculate word error rate (-CalculateWER (false))"  << std::endl
            << "  -SwitchIter:           iteration before which the language model orders are switched"  << std::endl
            << "                         (-SwitchIter SwitchIterIdx NewKnownN NewUnkN NewLMNumIters (0 1 1 0))"  << std::endl
            << "  -AmScale:              acoustic model scaling factor (-AmScale AcousticModelScalingFactor (1))"  << std::endl
            << "  -InitLM:               initialize language model from initialization fsts"  << std::endl
            << "                         (-InitLM InitTranscriptionFilename ())"  << std::endl
            << "  -InitLmNumIterations:  Number of iterations for language model initialization"  << std::endl
            << "                         (-InitLmNumIterations NumIterations (0))"  << std::endl
            << "  -PruningStep:          stepsize through pruning values during lper calculation"  << std::endl
            << "                         (-PruningStep PruningStart PruningStep PruningEnd (inf 0 inf)"  << std::endl
            << "  -BeamWidth:            Beam width through the composed FST I*L*G. To disable pruning, set it to -1" << std::endl
            << "                         (-BeamWidth BeamWidth (-1))" << std::endl
            << "  -OutputEditOperations: Output edit operations after LPER, PER and WER calculation (false)" << std::endl
            << "                         (-OutputEditOperations (false))" << std::endl
            << "  -EvalInterval:         Evaluation interval (-EvalInterval EvalInterval (1))" << std::endl
            << "  -WordLengthModulation: Set word length modulation. -1: off, 0: automatic, >0 set mean word length" << std::endl
            << "                         (-WordLengthModulation WordLength (-1))" << std::endl
            << "  -UseViterby:           Set iteration to switch to viterby decoding." << std::endl
            << "                         0: off, >0 number of iteration (Parameter: -UseVitery NumIter)" << std::endl
            << "  -DeactivateCharacterModel: Set iteration to deactivate character model." << std::endl
            << "                             0: off, >0 number of iteration (Parameter: -DeactivateCharacterModel NumIter)" << std::endl
            << "  -HTKLMScale:           Language model scaling factor when reading HTK lattices (Parameter: -HTKLMScale K (0))" << std::endl
            << "  -ReadNodeTimes;        Read node timing informations from HTK lattice" << std::endl;

  if(!err.empty()){
    std::cout << std::endl << " Error: " << err << std::endl;
  }
  exit(1);
}

void ParameterParser::ReadFilesFromFileList(const std::string &InputFileList)
{
  std::ifstream files(InputFileList);
  if (!files) {
    std::ostringstream err;
    err << "Couldn't find the file list: '" << InputFileList << "'" << std::endl;
    DieOnHelp(err.str());
  }
  std::string buff;
  while (std::getline(files, buff)) {
    Parameters.InputFiles.push_back(buff);
    std::ifstream checkFile(buff.c_str());
    if (!checkFile) {
      std::ostringstream err;
      err << "Couldn't find input file: '" << buff << "'" << std::endl;
      DieOnHelp(err.str());
    }
    checkFile.close();
  }
  files.close();
}

void ParameterParser::ReadFilesFromParameters(int argc, const char **argv)
{
  for (int argPos = 0; argPos < argc; argPos++) {
    Parameters.InputFiles.push_back(std::string(argv[argPos]));
    std::ifstream checkFile(argv[argPos]);
    if (!checkFile) {
      std::ostringstream err;
      err << "Couldn't find input file: '" << argv[argPos] << "'" << std::endl;
      DieOnHelp(err.str());
    }
    checkFile.close();
  }
}

const ParameterStruct &ParameterParser::GetParameters() const
{
  return Parameters;
}


ParameterStruct::ParameterStruct() :
  KnownN(1),
  UnkN(1),
  NoThreads(1),
  PruneFactor(std::numeric_limits<double>::infinity()),
  InputFilesList(),
  InputType(INPUT_TEXT),
  SymbolFile(),
  InputArcInfosFile(),
  AmScale(1),
  Debug(0),
  LatticeFileType(TEXT),
  ExportLattices(false),
  SwitchIter(0),
  NewKnownN(1),
  NewUnkN(1),
  NewLMNumIters(0),
  InitLM(false),
  InitTranscription(),
  InitLmNumIterations(0),
  InputFiles(),
  NumIter(0),
  ExportLatticesDirectoryName(),
  OutputDirectoryBasename(),
  OutputFilesBasename(),
  ReferenceTranscription(),
  UseReferenceTranscription(false),
  CalculatePER(false),
  CalculateWER(false),
  CalculateLPER(false),
  PruningStart(std::numeric_limits<double>::infinity()),
  PruningStep(0),
  PruningEnd(std::numeric_limits<double>::infinity()),
  BeamWidth(-1),
  OutputEditOperations(false),
  EvalInterval(1),
  WordLengthModulation(-1),
  UseViterby(0),
  DeactivateCharacterModel(0),
  HTKLMScale(0),
  ReadNodeTimes(false)
{
}
