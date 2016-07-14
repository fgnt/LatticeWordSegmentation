// ----------------------------------------------------------------------------
/**
   File: FileReader.cpp
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
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include <fst/rmepsilon.h>
#include <fst/arcsort.h>
#include <fst/compose.h>
#include "FileReader.hpp"
#include <CustomArcMappers.hpp>
#include <cstdlib>
#include "definitions.hpp"
#include "../DebugLib.hpp"
#include "../EditDistanceCalculator/LPERCalculator.hpp"

FileReader::FileReader(ParameterStruct Params) :
  Params(Params)
{
  GlobalStringToInt.Insert(EPS_SYMBOL);
  GlobalStringToInt.Insert(PHI_SYMBOL);
  GlobalStringToInt.Insert(UNKBEGIN_SYMBOL);
  GlobalStringToInt.Insert(UNKEND_SYMBOL);
  GlobalStringToInt.Insert(SENTSTART_SYMBOL);
  GlobalStringToInt.Insert(SENTEND_SYMBOL);

  // read the initialization, input and reference data,
  // export data if specified and do some prepocessing
  // of the input lattices (acoustic model scaling and
  // application of word end and sentence end transducers
  if (!Params.SymbolFile.empty()) {
    ReadSymbols();
  }

  if (!Params.InputArcInfosFile.empty()) {
    ReadInputArcInfos();
  }

  if (Params.UseDictFile) {
    ReadPronDict();
  }

  if (Params.InitLM) {
    ReadInitTranscription();
  }

  ReadInputFilesFromList();

  if (Params.ExportLattices) {
    WriteOpenFSTLattices();
  }

  if (Params.UseReferenceTranscription) {
    ReadReferenceTranscription();
    if (Params.CalculateLPER) {
      CalculateLatticePhonemeErrorRate();
    }
  }

  if (Params.AmScale != 1) {
    ApplyAcousticModelScalingFactor();
  }
  ApplyWordEndTransducer();
  ApplySentEndTransducer();

}

FileData FileReader::GetInputFileData()
{
  return FileData(GlobalStringToInt, InitStringToInt, InitFsts, InitFileNames,
                  InputStringToInt, InputFsts, InputFileNames, InputArcInfos,
                  ReferenceStringToInt, ReferenceFsts, ReferenceFileNames);
}


void FileReader::WriteInputArcInfos() const
{
  std::cout << "  Writing input arc information to "
            << Params.ExportLatticesDirectoryName
            << "input_arcs.txt" << std::endl;
  std::ofstream ArcFile(Params.ExportLatticesDirectoryName + "input_arcs.txt");
  for (std::size_t i = 0; i < InputArcInfos.size(); i++) {
    ArcFile << InputArcInfos.at(i).label << "\t" << InputArcInfos.at(i).start
            << "\t" << InputArcInfos.at(i).end << std::endl;
  }
  ArcFile.close();
}


void FileReader::ReadInputArcInfos()
{
  std::cout << "  Reading input arc information from "
            << Params.InputArcInfosFile << std::endl;
  std::ifstream ArcFile(Params.InputArcInfosFile);
  int label;
  float start;
  float end;

  // read the arc infos
  while (ArcFile >> label) {
    ArcFile >> start;
    ArcFile >> end;
    InputArcInfos.push_back(ArcInfo(label, start, end));
//     std::cout << "Input arc info [" << InputArcInfos.size() - 1
//               << "] - Label: " << label << " Start: " << start << " End: "
//               << end << std::endl;
  }
  ArcFile.close();
}


void FileReader::ReadSymbols()
{
  // open the symbolfile
  std::ifstream in(Params.SymbolFile);
  if (!in) {
    std::ostringstream err;
    err << "Could not find symbol file " << Params.SymbolFile << std::endl;
    throw std::runtime_error(err.str());
  }

  // We need to make sure that the symbol input is appropriate for latticelm
  std::vector<std::string> sanity = {
    EPS_SYMBOL, "0",
    PHI_SYMBOL, "1",
    UNKBEGIN_SYMBOL, "2",
    UNKEND_SYMBOL, "3",
    SENTSTART_SYMBOL, "4",
    SENTEND_SYMBOL, "5"
  };

  std::string buff;
  for (auto& sanityItem: sanity) {
    in >> buff;
    if (sanityItem != buff) {
      std::cerr << "The first six symbols in the symbol file must be "
                << "\"<eps> 0\", \"<phi> 1\", \"<unk> 2\", \"</unk> 3\","
                << " \"<s> 4\" and \"</s> 5\"" << std::endl;
      std::exit(1);
    }
  }

  // read the symbols
  while (in >> buff) {
    GlobalStringToInt.Insert(buff);
//  std::cerr << "Adding symbol " << buff << " as "
//            << GlobalStringToInt.GetInt(buff) << std::endl;
    in >> buff;
  }
}


void FileReader::ReadInitTranscription()
{
  ReadTextFiles(std::vector<std::string>(1, Params.InitTranscription),
                &InitFsts, &InitFileNames, PARSE_REFERENCES);
  InitStringToInt = GlobalStringToInt;
}


void FileReader::ReadInputFilesFromList()
{
  switch (Params.LatticeFileType) {
  case HTK_FST:
    ReadHTKLattices();
    break;
  case OPEN_FST:
    ReadOpenFSTLattices();
    break;
  case TEXT:
    ReadTextFiles(Params.InputFiles, &InputFsts, &InputFileNames);
    break;
  default:
    std::cout << "Invalid lattice file type!" << std::endl;
    std::exit(1);
  }
  InputStringToInt = GlobalStringToInt;
}


void FileReader::ReadHTKLattices()
{
  for (std::size_t InputFileId = 0;
       InputFileId < Params.InputFiles.size(); InputFileId++) {
    std::cout << "Reading nBest file [" << InputFileId << "/"
              << Params.InputFiles.size() << "] from HTK FST "
              << Params.InputFiles.at(InputFileId) << std::endl;

    // some variables
    float lmscale = Params.HTKLMScale;
    int debug_ = 0;
    std::string line;
    std::string utterance;

    // open file
    std::ifstream in(Params.InputFiles[InputFileId]);

    //get Version (first line)
    std::getline(in, line);

    //get Utterance
    std::getline(in, line);
    std::size_t pos = line.find("=");
    utterance = line.substr(pos + 1);
    if (debug_) {
      std::cout << "Reading utterance: " << utterance << std::endl;
    }

    //get LM factor
    while (std::getline(in, line) &&
        line.substr(0, 7) != "lmscale" && line.substr(0, 1) != "N") {
      std::getline(in, line);
    }

    if (line.substr(0, 7) == "lmscale") {
      std::istringstream iss(line);
      string lmScaleString;
      iss >> lmScaleString;
      pos = lmScaleString.find("=");
      lmscale = std::stof(lmScaleString.substr(pos + 1));
    }

    //get Nodes and Links
    while (in.good() && line.substr(0, 1) != "N") {
      std::getline(in, line);
    }

    std::istringstream iss(line);
    string nodeStr;
    iss >> nodeStr;
    pos = nodeStr.find("=");
    std::size_t nodes = std::stoull(nodeStr.substr(pos + 1));
    string linksStr;
    iss >> linksStr;
    pos = linksStr.find("=");
    int linksLeft = std::stoi(linksStr.substr(pos + 1));

    // prepare the fst -> it gets a unique start and end state
    LogVectorFst latticeFst;
    latticeFst.AddState();
    latticeFst.SetStart(0);

    // create the nodes
    latticeFst.ReserveStates(nodes);
    for (std::size_t s = 1; s < nodes; s++) {
      latticeFst.AddState();
    }
    // latticeFst.SetFinal(nodes - 1, 0);

    // read node times
    std::vector<float> NodeTimes;
    if (Params.ReadNodeTimes) {

      // read nodes
      NodeTimes.resize(nodes, -1);
      for (std::size_t NodeId = 0; NodeId < nodes; NodeId++) {
        // find next node entry
        std::getline(in, line);
        while (in.good() && line.substr(0, 1) != "I") {
          std::getline(in, line);
        }
        std::istringstream iss(line);
        string nodeStr;
        iss >> nodeStr; // discard node number
        iss >> nodeStr;
        pos = nodeStr.find("=");
        NodeTimes[NodeId] = std::stof(nodeStr.substr(pos + 1));
        if (debug_) {
          std::cout << "Reading Node " << NodeId << " at time "
                    << NodeTimes[NodeId] << std::endl;
        }
      }
      InputArcInfos.push_back(ArcInfo(EPS_SYMBOLID, -1, -1));
    }

    //  read segment list
    bool ReadSegList = false;
    if (ReadSegList) {
      ReadSegmentList(InputFileId, line, debug_);
    }

    //read the path
    std::vector<bool> IsFinal(nodes, true);

    while (std::getline(in, line)) {
      if (line.substr(0, 1) == "J") {       //only evaluate the links
        //Format example: J=5 S=0 E=5 W="zh"  v=0 a=-273.284  l=-3.80666
        // J: link no; S: start node; E: end node; W: phone;
        // v: ???; a: acoustic model score; l: lm score

        //variables
        CharId ilab;
        CharId olab;
        fst::LogArc arc;
        string cur;
        string phone;
        std::istringstream iss(line);

        //discard link number
        iss >> cur;

        //start
        int start;
        if (!(iss >> cur)) {
          break;
        }
        pos = cur.find("=");
        start = std::stoi(cur.substr(pos + 1));
        IsFinal.at(start) = false;

        //end
        int end;
        if (!(iss >> cur)) {
          break;
        }
        pos = cur.find("=");
        end = std::stoi(cur.substr(pos + 1));

        // phone
        if (!(iss >> cur)) {
          break;
        }
        pos = cur.find("=");
        int off = 0; //RASR quotes the phones. We only want the phone
        if (cur.substr(pos + 1, 1) == "\"") {
          off = 1;
        }
        phone = cur.substr(pos + 1 + off, cur.length() - pos - 1 - (2 * off));

        //discard v
        iss >> cur;
        if (cur.substr(0, 2) == "v=") {
          iss >> cur;
        }

        //acoustic model score
        float amScore;
        pos = cur.find("=");
        amScore = -(std::stof(cur.substr(pos + 1)) + Params.AMScoreShift);

        //lm score
        iss >> cur;
        float lmScore;
        pos = cur.find("=");
        lmScore = -std::stof(cur.substr(pos + 1));
        amScore = amScore / log(10);
        amScore += lmscale * lmScore / log(10);

        //replace silence with eps
        if (IsSilence(phone)) {
          olab = EPS_SYMBOLID;
          ilab = EPS_SYMBOLID;
        } else {
          olab = GlobalStringToInt.Insert(phone);
          if (Params.ReadNodeTimes) {
            InputArcInfos.push_back(
              ArcInfo(olab, NodeTimes[start], NodeTimes[end]));
            ilab = InputArcInfos.size() - 1;
          } else {
            ilab = olab;
          }
        }

        //Add arc
        fst::MutableArcIterator< LogVectorFst > ArcIter(&latticeFst, start);
        while (!ArcIter.Done() && !(ArcIter.Value().olabel == olab &&
                                    ArcIter.Value().nextstate == end)) {
          ArcIter.Next();
        }
        if (!ArcIter.Done()) {
          fst::LogArc arc = ArcIter.Value();
          if (debug_ > 2) {
            std::cout << "Found arc: Old: " << start << "->" << arc.nextstate
                      << "[" << arc.weight.Value() << "] new: " << start
                      << "->" << end << "[" << amScore << "]" << std::endl;
          }
          arc.weight = fst::Plus(arc.weight, fst::LogWeight(amScore));
          ArcIter.SetValue(arc);
          if (debug_ > 2) {
            std::cout << "Modified phone: " << phone << "[" << olab << "]"
                      << " start: " << start << " end: " << end
                      << " score: " << arc.weight.Value() << " | "
                      << linksLeft << " left" << std::endl;
          }
        } else {
          latticeFst.AddArc(start, fst::LogArc(ilab, olab, amScore, end));
          if (debug_ > 2) {
            std::cout << "Added: phone: " << phone << "[" << olab << "]"
                      << " start: " << start << " end: " << end
                      << " score: " << amScore << " | "
                      << linksLeft << " left" << std::endl;
          }
        }
        linksLeft--;
      } //end if
    } //end read line

    // set final nodes
    if (!IsFinal.at(nodes - 1)){
      std::cout << "Warning: The last node is not a final node!"
                << std::endl;
    }

    for(std::size_t NodeId = 0; NodeId < nodes; NodeId++) {
      if (IsFinal.at(NodeId)) {
        latticeFst.SetFinal(NodeId, 0);
        if (NodeId != nodes - 1) {
          std::cout << "Warning: Final node " << NodeId
                    << " is not the last node!" << std::endl;
        }
      }
    }

    // rmepsilon
    if (debug_ > 2) {
      std::cout << "RmEpsilon";
    }
    fst::RmEpsilon(&latticeFst);

    // topsort
    if (debug_ > 2) {
      std::cout << " | TopSort";
    }
    fst::TopSort(&latticeFst);

    // arcsort
    if (debug_ > 2) {
      std::cout << " | ArcSort";
    }
    fst::ArcSort(&latticeFst, fst::OLabelCompare<fst::LogArc>());

    // print number of states
    if (debug_ > 2) {
      std::cout << std::endl;
    }
    int arcCnt = 0;
    for (LogStateIterator siter(latticeFst); !siter.Done(); siter.Next()) {
      arcCnt += latticeFst.NumArcs(siter.Value());
    }
    std::cout << latticeFst.NumStates() << " States | " << arcCnt << " Arcs";

    //Pruning
    if (Params.PruneFactor != std::numeric_limits<double>::infinity()) {
      LogToStdMapFst InStdArcFst(latticeFst, fst::LogToStdMapper());
      StdVectorFst OutStdArcFst;
      fst::Prune(InStdArcFst, &OutStdArcFst, Params.PruneFactor);
      fst::ArcMap(OutStdArcFst, &latticeFst, fst::StdToLogMapper());
      fst::ArcSort(&latticeFst, fst::OLabelCompare<fst::LogArc>());
      arcCnt = 0;
      for (LogStateIterator StateIter(latticeFst);
           !StateIter.Done(); StateIter.Next()) {
        arcCnt += latticeFst.NumArcs(StateIter.Value());
      }
      std::cout << " (" << latticeFst.NumStates()
                << " States | " << arcCnt << " Arcs after pruning)";
    }
    std::cout << std::endl;

    if (latticeFst.NumStates() == 0) {
      std::cout << "Error: no states for utterance " << utterance << std::endl;
      std::runtime_error("Exiting");
    }
    InputFsts.push_back(latticeFst);
    InputFileNames.push_back(
      boost::filesystem::path(
        Params.InputFiles.at(InputFileId)).filename().string());
  }
}

void FileReader::ReadSegmentList(std::size_t InputFileId,
                                 std::string line, int debug_){
  std::size_t NumSegments;
  std::vector<float> SegmentStart;
  std::vector<float> SegmentEnd;
  // read segments
  std::size_t pos = Params.InputFiles.at(InputFileId).find_last_of(".");
  std::string SegmentFile(Params.InputFiles.at(InputFileId).substr(0, pos) +
                      "_Segments.txt");
  std::cout << "Reading segments from " << SegmentFile << std::endl;
  std::ifstream inSegFile(SegmentFile);
  std::getline(inSegFile, line); // read number of segments
  NumSegments = std::stoull(GetSubstrAfterSep(line, '='));
  SegmentStart.resize(NumSegments, -1);
  SegmentEnd.resize(NumSegments, -1);
  for (std::size_t SegmentId = 0; SegmentId < NumSegments; SegmentId++) {
    // find next segment enty
    std::getline(inSegFile, line);
    while (inSegFile.good() && line.substr(0, 1) != "J") {
      std::getline(inSegFile, line);
    }
    std::istringstream iss(line);
    std::string SegStr;
    iss >> SegStr; // discard Segment number
    iss >> SegStr; // read start time
    SegmentStart[SegmentId] = std::stof(GetSubstrAfterSep(SegStr, '='));
    iss >> SegStr; // read end time
    SegmentEnd[SegmentId] = std::stof(SegStr.substr(SegStr.find("=") + 1));
    iss >> SegStr; // read word
    if (SegStr.substr(SegStr.find("=") + 1) == "[silence]") {
      SegmentStart[SegmentId] = -1;
      SegmentEnd[SegmentId] = -1;
    }
    if (debug_) {
      std::cout << "Reading Segment " << SegmentId << " of "
                << NumSegments << " Start: " << SegmentStart[SegmentId]
                << " End: " << SegmentEnd[SegmentId] << std::endl;
    }
  }
}
void FileReader::ReadOpenFSTLattices()
{
  for (std::size_t i = 0; i < Params.InputFiles.size(); i++) {
    std::cout << "Reading lattice file [" << i << "/"
              << Params.InputFiles.size() << "] from OpenFst FST "
              << Params.InputFiles.at(i) << std::endl;
    LogVectorFst LogArcVectorFst(*LogVectorFst::Read(Params.InputFiles.at(i)));
    int arcCnt = 0;
    for (LogStateIterator StateIter(LogArcVectorFst);
         !StateIter.Done(); StateIter.Next()) {
      arcCnt += LogArcVectorFst.NumArcs(StateIter.Value());
    }
    std::cout << LogArcVectorFst.NumStates()
              << " States | " << arcCnt << " Arcs";

    //Pruning
    if (Params.PruneFactor != std::numeric_limits<double>::infinity()) {
      LogToStdMapFst InStdArcFst(LogArcVectorFst, fst::LogToStdMapper());
      StdVectorFst OutStdArcFst;
      fst::Prune(InStdArcFst, &OutStdArcFst, Params.PruneFactor);
      fst::ArcMap(OutStdArcFst, &LogArcVectorFst, fst::StdToLogMapper());
      fst::ArcSort(&LogArcVectorFst, fst::OLabelCompare<fst::LogArc>());
      arcCnt = 0;
      for (LogStateIterator StateIter(LogArcVectorFst);
           !StateIter.Done(); StateIter.Next()) {
        arcCnt += LogArcVectorFst.NumArcs(StateIter.Value());
      }
      std::cout << " (" << LogArcVectorFst.NumStates()
                << " States | " << arcCnt << " Arcs after pruning)";
    }
    std::cout << std::endl;

    if (LogArcVectorFst.NumStates() == 0) {
      std::cout << "Error: no states for utterance " << i << std::endl;
      std::runtime_error("Exiting");
    }

    InputFsts.push_back(LogArcVectorFst);

    InputFileNames.push_back(
      boost::filesystem::path(Params.InputFiles.at(i)).filename().string());
  }
}

void FileReader::ReadPronDict()
{
  // Read pronunciation dictionary and store into map
  std::vector<std::string> Pronunciation;

  std::string line;
  std::ifstream PDStream(Params.DictFile);
  RevPronDictType RevPronDict;
  std::size_t MaxAux = 0;
  // foreach line:
  while(std::getline(PDStream, line)) {
    std::istringstream Tokens(line);
    std::string Word, Phone;

    // first part is word label
    if(!(Tokens >> Word)){
      std::runtime_error("Invalid PronDict format!");
    }

    if (PronDict.count(Word) > 0) {
      std::cerr << "Warning: Multiple entries of the same word found."
                << "Skipped: " << Word << std::endl
                <<  "# Suggestion: use word+pron as a word if necessary."
                << Word << std::endl;
    }

    // second part is phoneme/char label sequence
    Pronunciation.resize(0);
    std::ostringstream Phones;
    while(Tokens >> Phone) {
      // append phone to current sequence
      Pronunciation.push_back(Phone);
      Phones << Phone << " ";

      // Add new phone to symbol table
      std::string PhoneSymbol(PHONE_PREFIX+Phone);
      GlobalStringToInt.Insert(PhoneSymbol);
    }

    if(Pronunciation.size() == 0){
      std::runtime_error("Word without pronunciation: " + Word);
    }

    // if phone sequence is already known, append a pseudo phone to disambiguate
    auto PhoneSequence = Phones.str();
    if(RevPronDict.count(PhoneSequence) > 0) {
      // add word to already known sequence.
      auto& SequenceEntry = RevPronDict.at(PhoneSequence);
      std::size_t NumAux = SequenceEntry.size();
      SequenceEntry.insert(std::make_pair(Word, NumAux));

      // disambiguate sequence by adding a pseudo phone using the count
      Pronunciation.push_back("#"+ NumAux);
      MaxAux = std::max(MaxAux, NumAux);

    } else {
      // add sequence and word with count 0 to reverse dictionary
      RevPronDict[PhoneSequence] =
          StringToIntMap{std::make_pair(Word, 0)};
    }
    PronDict.insert(std::make_pair(Word, Pronunciation));
  }

  // Add pseudo phones to symbol table.
  // Abort with error if a symbol already exists.
  for(std::size_t NumAux = 1; NumAux < MaxAux; NumAux++){
    std::string AuxPhoneSymbol(PHONE_PREFIX + "#" + std::to_string(NumAux));
    if(GlobalStringToInt.GetInt(AuxPhoneSymbol) >= 0) {
      std::runtime_error("Error: Pseudo phone " + PHONE_PREFIX +
                         "#" + std::to_string(NumAux) +
                         " should not exist in symbol table!");
    }
    GlobalStringToInt.Insert(AuxPhoneSymbol);
  }
}

void FileReader::ReadTextFiles(
  const std::vector<std::string> &InputFiles,
  std::vector<LogVectorFst>* InputFsts,
  std::vector<std::string> *FileNames,
  bool ParseReferences)
{
  for(auto& InputFile: InputFiles) {
    std::ifstream in(InputFile);

    int SentenceIndex = 0;
    for (std::string line; std::getline(in, line); ) {
      std::istringstream iss(line);
      LogVectorFst latticeFst;
      latticeFst.AddState();
      latticeFst.SetStart(0);
      StateId State = 0;

      // Read sentence and create Transducer
      if (ParseReferences && Params.UseDictFile) {
        // Dict file present: read words from file and look up pronunciation
        for (std::string Word; iss >> Word; ) {
          std::vector<std::string>& Pronunciation = PronDict.at(Word);
          for(std::string phone: Pronunciation) {
            State = AddPhoneToFst(latticeFst, State, phone);
          }
          // explicitly add word end marker
          State = AddPhoneToFst(latticeFst, State, UNKEND_SYMBOL);
        }
      } else {
        // Sentence as sequence of phones separated by word end markers
        for (std::string phone; iss >> phone; ) {
          GlobalStringToInt.Insert(phone);
          State = AddPhoneToFst(latticeFst, State, phone);
        }
      }
      latticeFst.SetFinal(State, 0);

      if (State == 0) {
        std::cout << "Empty line found in " << InputFile << std::endl;
        std::cout << "Please ensure that each line in the training file "
                  << "contains at least one symbol." << std::endl;
        std::exit(1);
      }
      InputFsts->push_back(latticeFst);
      FileNames->push_back(
        boost::filesystem::path(InputFile).filename().string() +
        "_Line_" + std::to_string(++SentenceIndex));
    }
  }
}

StateId FileReader::AddPhoneToFst(LogVectorFst& LatticeFst, StateId State,
                                  const std::string& phone)
{
  CharId OutLab = GlobalStringToInt.GetInt(PHONE_PREFIX+phone);
  CharId InLab = OutLab;
  StateId nextState = LatticeFst.AddState();
  LatticeFst.AddArc(State, fst::LogArc(InLab, OutLab, 0, nextState));
  return nextState;
}

void FileReader::PruneLattices(double PruningFactor)
{
  if (PruningFactor != std::numeric_limits<double>::infinity()) {
    for (auto& currentInputFst: InputFsts) {
      LogToStdMapFst InStdArcFst(currentInputFst, fst::LogToStdMapper());
      StdVectorFst OutStdArcFst;
      fst::Prune(InStdArcFst, &OutStdArcFst, PruningFactor);
      fst::ArcMap(OutStdArcFst, &currentInputFst, fst::StdToLogMapper());
      fst::ArcSort(&currentInputFst, fst::OLabelCompare<fst::LogArc>());

//       int arcCnt = 0;
//       for (fst::StateIterator<LogVectorFst> siter(InputFsts.at(InputFstIdx));
//            !siter.Done(); siter.Next()) {
//         arcCnt += InputFsts.at(InputFstIdx).NumArcs(siter.Value());
//       }
//       std::cout << " (" << InputFsts.at(InputFstIdx).NumStates()
//                 << " States | " << arcCnt << " Arcs after pruning)"
//                 << std::endl;
    }
  }
}


void FileReader::ReadReferenceTranscription()
{
  ReadTextFiles(std::vector<std::string>(1, Params.ReferenceTranscription),
                &ReferenceFsts, &ReferenceFileNames);
  ReferenceStringToInt = GlobalStringToInt;
  if (ReferenceStringToInt.GetSize() > InputStringToInt.GetSize()) {
    std::cout << "WARNING: Reference transcription contains"
              << " more characters than input transcription!" << std::endl;
    InputStringToInt = ReferenceStringToInt;
  }
}


void FileReader::WriteOpenFSTLattices() const
{
  for (std::size_t FileIdx = 0; FileIdx < InputFileNames.size(); FileIdx++) {
    DebugLib::WriteOpenFSTLattice(InputFsts.at(FileIdx),
                                  Params.ExportLatticesDirectoryName +
                                  InputFileNames.at(FileIdx) + "_" +
                                  std::to_string(Params.PruneFactor));
  }
  DebugLib::WriteSymbols(Params.ExportLatticesDirectoryName + "symbols.txt",
                         InputStringToInt.GetIntToStringVector(), NAMES);
  if (!InputArcInfos.empty()) {
    WriteInputArcInfos();
  }
}


void FileReader::ApplyAcousticModelScalingFactor()
{
  fst::WeightedMapper AcousticModelScalingFactorMapper(Params.AmScale);
  for (auto& currentInputFst: InputFsts) {
    fst::Map(&currentInputFst, AcousticModelScalingFactorMapper);
//     std::cout << "Scaling FST: " << InputFstIdx << " with factor: "
//               << Params.AmScale << std::endl;
  }
}


void FileReader::ApplyWordEndTransducer()
{
  LogVectorFst WordEndTransducer;
  StateId UNKState = WordEndTransducer.AddState();
  StateId CharacterState = WordEndTransducer.AddState();

  WordEndTransducer.SetStart(UNKState);
  WordEndTransducer.SetFinal(UNKState, 0);
  WordEndTransducer.AddArc(CharacterState,
    fst::LogArc(EPS_SYMBOLID, UNKEND_SYMBOLID, 0, UNKState));
  WordEndTransducer.AddArc(CharacterState,
    fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, 0, UNKState));
  for (std::size_t k = CHARACTERSBEGIN;
       k < InputStringToInt.GetIntToStringVector().size(); k++) {

    WordEndTransducer.AddArc(UNKState,
      fst::LogArc(k, k, 0, CharacterState));

    WordEndTransducer.AddArc(CharacterState,
      fst::LogArc(k, k, 0, CharacterState));
  }

  for (auto& currentInputFst: InputFsts) {
    currentInputFst = LogComposeFst(currentInputFst, WordEndTransducer);
  }
}

void FileReader::ApplySentEndTransducer()
{
  LogVectorFst SentEndTransducer;
  StateId CharacterState = SentEndTransducer.AddState();
  StateId SentEndState = SentEndTransducer.AddState();
  StateId FinalUNKState = SentEndTransducer.AddState();

  SentEndTransducer.SetStart(CharacterState);
  SentEndTransducer.SetFinal(FinalUNKState, 0);
  SentEndTransducer.AddArc(CharacterState,
    fst::LogArc(EPS_SYMBOLID, SENTEND_SYMBOLID, 0, SentEndState));
  SentEndTransducer.AddArc(CharacterState,
    fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, 0, CharacterState));

  for (std::size_t k = CHARACTERSBEGIN;
       k < InputStringToInt.GetIntToStringVector().size(); k++) {

    SentEndTransducer.AddArc(CharacterState,
      fst::LogArc(k, k, 0, CharacterState));
  }
  SentEndTransducer.AddArc(SentEndState,
    fst::LogArc(EPS_SYMBOLID, UNKEND_SYMBOLID, 0, FinalUNKState));

  for (auto& currentInputFst: InputFsts) {
    currentInputFst = LogComposeFst(currentInputFst, SentEndTransducer);
    fst::Connect(&currentInputFst);
  }
}

void FileReader::CalculateLatticePhonemeErrorRate()
{
  std::cout << " Calculating LPER with pruning from "
            << Params.PruningStart << " with step size "
            << Params.PruningStep << " to "
            << Params.PruningEnd << "!"
            << std::endl << std::endl;

  for (double PruningFactor = Params.PruningStart;
       PruningFactor >= Params.PruningEnd;
       PruningFactor -= Params.PruningStep) {
    PruneLattices(PruningFactor);
    std::cout << "  Pruning factor: " << PruningFactor << std::endl;

    std:: string prefix =
      Params.OutputDirectoryBasename + "KnownN_" +
      std::to_string(Params.KnownN) + "_UnkN_" +
      std::to_string(Params.UnkN) + "/" +
      Params.OutputFilesBasename + "LPER_";

    LPERCalculator InputLatticeStatistics(
      InputFsts,
      ReferenceFsts,
      ReferenceStringToInt.GetIntToStringVector(),
      Params.NoThreads,
      InputFileNames,
      prefix,
      Params.OutputEditOperations,
      InputArcInfos
    );

    DebugLib::PrintEditDistanceStatistics(
      InputLatticeStatistics.GetInsDelSubCorrNFoundNRef(),
      "Lattice phoneme error rate",
      "PER"
    );

    if (Params.PruningStart == std::numeric_limits<double>::infinity()) {
      break;
    }
  }
}

bool FileReader::IsSilence(std::string phone)
{
  return phone == "!SENT_START" || phone == "!SENT_END" || phone == "!NULL" ||
         phone == "sil" || phone == "!ENTER" || phone == "!EXIT" ||
         phone == "NSN" || phone == "<s>" || phone == "</s>";
}

std::string FileReader::GetSubstrAfterSep(std::string inStr, char sep)
{
  return inStr.substr(inStr.find(sep)+1);
}
