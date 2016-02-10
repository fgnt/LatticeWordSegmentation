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
#include <sys/stat.h>
#include <boost/filesystem/path.hpp>
#include <fst/rmepsilon.h>
#include <fst/arcsort.h>
#include <fst/compose.h>
#include "FileReader.hpp"
#include "WeightedMapper.hpp"
#include "definitions.hpp"

FileReader::FileReader()
{
  GlobalStringToInt.Insert(EPS_SYMBOL);
  GlobalStringToInt.Insert(PHI_SYMBOL);
  GlobalStringToInt.Insert(UNKBEGIN_SYMBOL);
  GlobalStringToInt.Insert(UNKEND_SYMBOL);
  GlobalStringToInt.Insert(SENTSTART_SYMBOL);
  GlobalStringToInt.Insert(SENTEND_SYMBOL);
}


void FileReader::ReadSymbols(const std::string &SymbolFile)
{
  // open the symbolfile
  std::ifstream in(SymbolFile);
  if (!in) {
    std::ostringstream err;
    err << "Could not find symbol file " << SymbolFile << endl;
    throw std::runtime_error(err.str());
  }

  // We need to make sure that the symbol input is appropriate for latticelm
  vector<string> sanity(12);
  sanity[0] = EPS_SYMBOL;
  sanity[1] = "0";
  sanity[2] = PHI_SYMBOL;
  sanity[3] = "1";
  sanity[4] = UNKBEGIN_SYMBOL;
  sanity[5] = "2";
  sanity[6] = UNKEND_SYMBOL;
  sanity[7] = "3";
  sanity[8] = SENTSTART_SYMBOL;
  sanity[9] = "4";
  sanity[10] = SENTEND_SYMBOL;
  sanity[11] = "5";
  string buff;
  for (unsigned i = 0; i < 12; i++) {
    in >> buff;
    if (sanity[i] != buff) {
//    std::runtime_error("The first two symbols in the symbol file must be \"<eps> 0\" and \"<phi> 1\"");
      std::cerr << "The first sixs symbols in the symbol file must be \"<eps> 0\", \"<phi> 1\", \"<unk> 2\", \"</unk> 3\", \"<s> 4\" and \"</s> 5\"" << std::endl;
      exit(1);
    }
  }

  // read the symbols
  while (in >> buff) {
    GlobalStringToInt.Insert(buff);
//  cerr << "Adding symbol " << buff << " as " << GlobalStringToInt.GetInt(buff) << endl;
    in >> buff;
  }
}


void FileReader::ReadInitTranscription(const string &InitFile)
{
  ReadTextFiles(std::vector<std::string>(1, InitFile), &InitFsts, &InitFileNames);
  InitStringToInt = GlobalStringToInt;
}


void FileReader::ReadInputFilesFromList(const vector< string > &InputFiles, LatticeFileTypes LatticeFileType, double PruningFactor, double HTKLMScale)
{
  switch (LatticeFileType) {
  case CMU_FST:
    ReadCMULattices(InputFiles, PruningFactor);
    break;
  case HTK_FST:
    ReadHTKLattices(InputFiles, PruningFactor, HTKLMScale);
    break;
  case OPEN_FST:
    ReadOpenFSTLattices(InputFiles, PruningFactor);
    break;
  case TEXT:
    ReadTextFiles(InputFiles, &InputFsts, &InputFileNames);
    break;
  default:
    std::cout << "Invalid lattice file type!" << std::endl;
    exit(1);
  }
  InputStringToInt = GlobalStringToInt;
}


void FileReader::ReadCMULattices(const std::vector<std::string> &InputFiles, double PruningFactor)
{
  for (unsigned i = 0; i < InputFiles.size(); i++) {
    std::cout << "Reading nBest file from CMU FST " << InputFiles.at(i) << std::endl;

    //prepare the fst -> it gets a unique start and end state and splits up every path
    fst::VectorFst<fst::LogArc> fst;
    fst.AddState();
    fst.SetStart(0);

    int state = 0;
    int debug_ = 0;

    unordered_map<int, int> stateMap;
    unordered_map<int, int>::const_iterator mapItStart;
    unordered_map<int, int>::const_iterator mapItEnd;
    stateMap.insert(make_pair(0, 0));

    vector<bool> isConnected;
    isConnected.push_back(true);

    //read the path
    string line;
    std::ifstream in(InputFiles[i].c_str());
    while (std::getline(in, line)) {
      std::istringstream iss(line);

      fst::LogArc arc;
      string cur;
      string phone;

      //start
      int start;
      if (!(iss >> cur)) {
        break;
      }
      start = atoi(cur.c_str());

      //end
      int end;
      if (!(iss >> cur)) {
        break;
      }
      end = start + atoi(cur.c_str());

      //acoustic model score
      int amScore;
      if (!(iss >> cur)) {
        break;
      }
      amScore = atoi(cur.c_str());

      // phone
      if (!(iss >> phone)) {
        break;
      }

      // ignore silence and the other lattice
      if (amScore < 0) {
        CharId lab;
        if (!strcmp(phone.c_str(), "SIL")) {
          lab = 0;
        } else {
          lab = GlobalStringToInt.Insert(phone);
        }

        //look if the start exists
        mapItStart = stateMap.find(start);
        if (debug_ > 2) {
          cout << "phone: " << phone << "[" << lab << "]" << " start: " << start << " end: " << end << " score: " << amScore;
        }
        if (mapItStart != stateMap.end()) {
          if (debug_ > 2) {
            cout << " - start state exists";
          }

          //Check if the end state exists
          mapItEnd = stateMap.find(end);
          if (mapItEnd != stateMap.end()) {
            //Check if arc exists
            fst::MutableArcIterator<fst::VectorFst<fst::LogArc> > aiter(&fst, state);
            while (!aiter.Done() && !(aiter.Value().olabel == lab && aiter.Value().nextstate == end)) {
              aiter.Next();
            }

            if (!aiter.Done()) {
              fst::LogArc arc = aiter.Value();
              arc.weight = fst::Plus(arc.weight, fst::LogWeight(-amScore));
              aiter.SetValue(arc);
            } else {
              if (debug_ > 2) {
                cout << " - new arc" << endl;
              }
              fst.AddArc(mapItStart->second, fst::LogArc(lab, lab, -amScore, mapItEnd->second));
              isConnected.at(mapItEnd->second) = true;
            }
          } else {
            if (debug_ > 2) {
              cout << " - new end state" << endl;
            }
            int newState = fst.AddState();
            state++;
            isConnected.push_back(true);
            fst.AddArc(mapItStart->second, fst::LogArc(lab, lab, -amScore, newState));
            stateMap.insert(make_pair(end, newState));
          }
        } else {
          if (debug_ > 2) {
            cout << " - new start state";
          }
          //Create new state
          int newStart = fst.AddState();
          isConnected.push_back(false);
          state++;
          stateMap.insert(make_pair(end, newStart));
          //Check if the end state exists
          mapItEnd = stateMap.find(end);
          if (mapItEnd != stateMap.end()) {
            if (debug_ > 2) {
              cout << " - new arc" << endl;
            }
            fst.AddArc(newStart, fst::LogArc(lab, lab, -amScore, mapItEnd->second));
            isConnected.at(mapItEnd->second) = true;
          } else {
            if (debug_ > 2) {
              cout << " - new end state" << endl;
            }
            int newState = fst.AddState();
            isConnected.push_back(true);
            fst.AddArc(newStart, fst::LogArc(lab, lab, -amScore, newState));
            stateMap.insert(make_pair(end, newState));
          }
        }
      }
    } //End read line

    unsigned s;
    for (s = 0; s < isConnected.size(); s++) {
      if (!isConnected.at(s)) {
        if (debug_) {
          cout << "state " << s << " is not connected";
        }
        fst.AddArc(s - 1, fst::LogArc(0, 0, 0, s));
      }
    }

    fst.SetFinal(s - 1, 0);
    fst::RmEpsilon(&fst);
    cout << fst.NumStates() << " States." << endl;

    //Pruning
    if (PruningFactor != std::numeric_limits<double>::infinity()) {
      fst::ArcMapFst<fst::LogArc, fst::StdArc, fst::LogToStdMapper> InStdArcFst(fst, fst::LogToStdMapper());
      fst::VectorFst<fst::StdArc> OutStdArcFst;
      fst::Prune(InStdArcFst, &OutStdArcFst, PruningFactor);
      fst::ArcMap(OutStdArcFst, &fst, fst::StdToLogMapper());
      fst::ArcSort(&fst, fst::OLabelCompare<fst::LogArc>());
      int arcCnt = 0;
      for (fst::StateIterator<fst::VectorFst<fst::LogArc> > siter(fst); !siter.Done(); siter.Next()) {
        arcCnt += fst.NumArcs(siter.Value());
      }
      cout << " (" << fst.NumStates() << " States | " << arcCnt << " Arcs after pruning)";
    }
    cout << endl;

    InputFsts.push_back(fst);
    InputFileNames.push_back(boost::filesystem::path(InputFiles.at(i)).filename().string());
  }
}


void FileReader::ReadHTKLattices(const std::vector< std::string > &InputFiles, double PruningFactor, double HTKLMScale)
{
  for (unsigned i = 0; i < InputFiles.size(); i++) {
    cout << "Reading nBest file [" << i << "/" << InputFiles.size() << "] from HTK FST " << InputFiles.at(i) << endl;

    //prepare the fst -> it gets a unique start and end state and splits up every path
    fst::VectorFst<fst::LogArc> fst; // = new fst::VectorFst<fst::LogArc>;
    fst.AddState();
    fst.SetStart(0);

    float lmscale = HTKLMScale;
    int debug_ = 0;

    string line;
    string utterance;

    // open file
    std::ifstream in(InputFiles[i].c_str());

    //get Version (first line)
    std::getline(in, line);

    //get Utterance
    std::getline(in, line);
    unsigned pos = line.find("=");
    utterance = line.substr(pos + 1);
    if (debug_) {
      cout << "Reading utterance: " << utterance << endl;
    }

    //get LM factor
    while (std::getline(in, line) && !(strcmp(line.substr(0, 7).c_str(), "lmscale") == 0) && !(strcmp(line.substr(0, 1).c_str(), "N") == 0)) {
      std::getline(in, line);
    }

    if (strcmp(line.substr(0, 7).c_str(), "lmscale") == 0) {
      std::istringstream iss(line);
      string lmScaleString;
      iss >> lmScaleString;
      pos = lmScaleString.find("=");
      lmscale = atof(lmScaleString.substr(pos + 1).c_str());
    }

    while (!in.eof() && !(strcmp(line.substr(0, 1).c_str(), "N") == 0)) {
      std::getline(in, line);
    }

    //get Nodes and Links
    std::istringstream iss(line);
    string nodeStr;
    iss >> nodeStr;
    pos = nodeStr.find("=");
    int nodes = atoi(nodeStr.substr(pos + 1).c_str());
    string linksStr;
    iss >> linksStr;
    pos = linksStr.find("=");
    int linksLeft = atoi(linksStr.substr(pos + 1).c_str());

    //create the nodes
    fst.ReserveStates(nodes);
    for (int s = 1; s < nodes; s++) {
      fst.AddState();
    }
    fst.SetFinal(nodes - 1, 0);

    //read the path
    while (std::getline(in, line)) {
      if (!strcmp(line.substr(0, 1).c_str(), "J")) {       //only evaluate the links
        //Format example: J=5 S=0 E=5 W="zh"  v=0 a=-273.284  l=-3.80666
        // J: link no; S: start node; E: end node; W: phone; v: ???; a: acoustic model score; l: lm score

        //variables
        CharId lab;
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
        start = atoi(cur.substr(pos + 1).c_str());

        //end
        int end;
        if (!(iss >> cur)) {
          break;
        }
        pos = cur.find("=");
        end = atoi(cur.substr(pos + 1).c_str());

        // phone
        if (!(iss >> cur)) {
          break;
        }
        pos = cur.find("=");
        int off = 0; //RASR quotes the phones. We only want the phone
        if (!strcmp(cur.substr(pos + 1, 1).c_str(), "\"")) {
          off = 1;
        }
        phone = cur.substr(pos + 1 + off, cur.length() - pos - 1 - (2 * off));

        //discard v
        iss >> cur;
        if ((strcmp(cur.substr(0, 2).c_str(), "v=") == 0)) {
          iss >> cur;
        }

        //acoustic model score
        float amScore;
        pos = cur.find("=");
        amScore = -atof(cur.substr(pos + 1).c_str());

        //lm score
        iss >> cur;
        float lmScore;
        pos = cur.find("=");
        lmScore = -atof(cur.substr(pos + 1).c_str());
        amScore = amScore / log(10);
        amScore += lmscale * lmScore / log(10);

        //replace silence with eps
        if (!strcmp(phone.c_str(), "!SENT_START") || !strcmp(phone.c_str(), "!SENT_END") || !strcmp(phone.c_str(), "!NULL") || !strcmp(phone.c_str(), "sil") || !strcmp(phone.c_str(), "!ENTER") || !strcmp(phone.c_str(), "!EXIT")) {
          lab = 0;
        } else {
          lab = GlobalStringToInt.Insert(phone);
        }

        //Add arc
        fst::MutableArcIterator< fst::VectorFst<fst::LogArc> > aiter(&fst, start);
        while (!aiter.Done() && !(aiter.Value().olabel == lab && aiter.Value().nextstate == end)) {
          aiter.Next();
        }
        if (!aiter.Done()) {
          fst::LogArc arc = aiter.Value();
          if (debug_ > 2) {
            cout << "Found arc: Old: " << start << "->" << arc.nextstate << "[" << arc.weight.Value() << "] new: " << start << "->" << end << "[" << amScore << "]" << endl;
          }
          arc.weight = fst::Plus(arc.weight, fst::LogWeight(amScore));
          aiter.SetValue(arc);
          if (debug_ > 2) {
            cout << "Modified phone: " << phone << "[" << lab << "]" << " start: " << start << " end: " << end << " score: " << arc.weight.Value() << " | " << linksLeft << " left" << endl;
          }
        } else {
          fst.AddArc(start, fst::LogArc(lab, lab, amScore, end));
          if (debug_ > 2) {
            cout << "Added: phone: " << phone << "[" << lab << "]" << " start: " << start << " end: " << end << " score: " << amScore << " | " << linksLeft << " left" << endl;
          }
        }
        linksLeft--;
      } //end if
    } //end read line

    if (debug_ > 2) {
      cout << "RmEpsilon";
    }
    fst::RmEpsilon(&fst);

    if (debug_ > 2) {
      cout << " | TopSort";
    }
    fst::TopSort(&fst);

    if (debug_ > 2) {
      cout << " | ArcSort";
    }
    fst::ArcSort(&fst, fst::OLabelCompare<fst::LogArc>());

    if (debug_ > 2) {
      cout << endl;
    }
    int arcCnt = 0;
    for (fst::StateIterator<fst::VectorFst<fst::LogArc> > siter(fst); !siter.Done(); siter.Next()) {
      arcCnt += fst.NumArcs(siter.Value());
    }
    cout << fst.NumStates() << " States | " << arcCnt << " Arcs";

    //Pruning
    if (PruningFactor != std::numeric_limits<double>::infinity()) {
      fst::ArcMapFst<fst::LogArc, fst::StdArc, fst::LogToStdMapper> InStdArcFst(fst, fst::LogToStdMapper());
      fst::VectorFst<fst::StdArc> OutStdArcFst;
      fst::Prune(InStdArcFst, &OutStdArcFst, PruningFactor);
      fst::ArcMap(OutStdArcFst, &fst, fst::StdToLogMapper());
      fst::ArcSort(&fst, fst::OLabelCompare<fst::LogArc>());
      arcCnt = 0;
      for (fst::StateIterator<fst::VectorFst<fst::LogArc> > siter(fst); !siter.Done(); siter.Next()) {
        arcCnt += fst.NumArcs(siter.Value());
      }
      cout << " (" << fst.NumStates() << " States | " << arcCnt << " Arcs after pruning)";
    }
    cout << endl;

    if (fst.NumStates() == 0) {
      cout << "Error: no states for utterance " << utterance << endl;
      std::runtime_error("Exiting");
    }
    InputFsts.push_back(fst);
    InputFileNames.push_back(boost::filesystem::path(InputFiles.at(i)).filename().string());
  }
}


void FileReader::ReadOpenFSTLattices(const vector< string > &InputFiles, double PruningFactor)
{
  for (unsigned i = 0; i < InputFiles.size(); i++) {
    cout << "Reading lattice file [" << i << "/" << InputFiles.size() << "] from OpenFst FST " << InputFiles.at(i) << endl;
    fst::VectorFst<fst::LogArc> fst(*fst::VectorFst<fst::LogArc>::Read(InputFiles.at(i)));
    int arcCnt = 0;
    for (fst::StateIterator<fst::VectorFst<fst::LogArc> > siter(fst); !siter.Done(); siter.Next()) {
      arcCnt += fst.NumArcs(siter.Value());
    }
    cout << fst.NumStates() << " States | " << arcCnt << " Arcs";

    //Pruning
    if (PruningFactor != std::numeric_limits<double>::infinity()) {
      fst::ArcMapFst<fst::LogArc, fst::StdArc, fst::LogToStdMapper> InStdArcFst(fst, fst::LogToStdMapper());
      fst::VectorFst<fst::StdArc> OutStdArcFst;
      fst::Prune(InStdArcFst, &OutStdArcFst, PruningFactor);
      fst::ArcMap(OutStdArcFst, &fst, fst::StdToLogMapper());
      fst::ArcSort(&fst, fst::OLabelCompare<fst::LogArc>());
      arcCnt = 0;
      for (fst::StateIterator<fst::VectorFst<fst::LogArc> > siter(fst); !siter.Done(); siter.Next()) {
        arcCnt += fst.NumArcs(siter.Value());
      }
      cout << " (" << fst.NumStates() << " States | " << arcCnt << " Arcs after pruning)";
    }
    cout << endl;

    if (fst.NumStates() == 0) {
      cout << "Error: no states for utterance " << i << endl;
      std::runtime_error("Exiting");
    }

    InputFsts.push_back(fst);
    InputFileNames.push_back(boost::filesystem::path(InputFiles.at(i)).filename().string());
  }
}


void FileReader::ReadTextFiles(const vector< string > &InputFiles, vector< fst::VectorFst< fst::LogArc > > *InputFsts, vector< string > *FileNames)
{
  for (unsigned i = 0; i < InputFiles.size(); i++) {
    std::ifstream in(InputFiles[i].c_str());
    string line;
    string str;
    int SentenceIndex = 0;
    while (std::getline(in, line)) {
      std::istringstream iss(line);
      fst::VectorFst<fst::LogArc> fst;
      fst.AddState();
      fst.SetStart(0);
      int state = 0;

//       std::cout << "Reading line: ";
      while (iss >> str) {
//  std::cout << str << " ";
        CharId OutLab = GlobalStringToInt.Insert(str);
        CharId InLab = OutLab;
//         if(OutLab == UNKEND_SYMBOLID) {
//           InLab = EPS_SYMBOLID;
//         }
        int nextState = fst.AddState();
        fst.AddArc(state, fst::LogArc(InLab, OutLab, 0, nextState));
        state = nextState;
      }
//       std::cout << endl;
      fst.SetFinal(state, 0);

      if (state == 0) {
        cout << "Empty line found in " << InputFiles[i] << endl;
        cout << "Please ensure that each line in the training file contains at least one symbol." << endl;
        exit(1);
      }
      InputFsts->push_back(fst);
      FileNames->push_back(boost::filesystem::path(InputFiles.at(i)).filename().string() + "_Line_" + std::to_string(++SentenceIndex));
//       PrintFST("lattice_debug/" + FileNames->back(), GlobalStringToInt.GetIntToStringVector(), InputFsts->back(), true, NAMESANDIDS);
    }
  }
}


void FileReader::PruneLattices(double PruningFactor)
{
  if (PruningFactor != std::numeric_limits<double>::infinity()) {
    for (unsigned int InputFstIdx = 0; InputFstIdx < InputFsts.size(); InputFstIdx++) {
      fst::ArcMapFst<fst::LogArc, fst::StdArc, fst::LogToStdMapper> InStdArcFst(InputFsts.at(InputFstIdx), fst::LogToStdMapper());
      fst::VectorFst<fst::StdArc> OutStdArcFst;
      fst::Prune(InStdArcFst, &OutStdArcFst, PruningFactor);
      fst::ArcMap(OutStdArcFst, &InputFsts.at(InputFstIdx), fst::StdToLogMapper());
      fst::ArcSort(&InputFsts.at(InputFstIdx), fst::OLabelCompare<fst::LogArc>());

//       int arcCnt = 0;
//       for (fst::StateIterator<fst::VectorFst<fst::LogArc> > siter(InputFsts.at(InputFstIdx)); !siter.Done(); siter.Next()) {
//         arcCnt += InputFsts.at(InputFstIdx).NumArcs(siter.Value());
//       }
//       cout << " (" << InputFsts.at(InputFstIdx).NumStates() << " States | " << arcCnt << " Arcs after pruning)" << endl;
    }
  }
}


void FileReader::ReadReferenceTranscription(const string &ReferenceFile)
{
  ReadTextFiles(std::vector<std::string>(1, ReferenceFile), &ReferenceFsts, &ReferenceFileNames);
  ReferenceStringToInt = GlobalStringToInt;
  if (ReferenceStringToInt.GetSize() > InputStringToInt.GetSize()) {
    std::cout << "WARNING: Reference transcription contains more characters than input transcription!" << std::endl;
    InputStringToInt = ReferenceStringToInt;
  }
}


void FileReader::WriteOpenFSTLattices(const string &Prefix, const string &Postfix) const
{
  for (unsigned int FileIdx = 0; FileIdx < InputFileNames.size(); FileIdx++) {
    WriteOpenFSTLattice(InputFsts.at(FileIdx), Prefix + InputFileNames.at(FileIdx) + Postfix);
//     PrintFST("./output_debug/" + FileNames.at(FileIdx), StringToInt.GetIntToStringVector(), InputFsts.at(FileIdx), true, true);
  }
  WriteSymbols(Prefix + "symbols.txt", InputStringToInt.GetIntToStringVector(), NAMES);
}


void FileReader::WriteOpenFSTLattice(const fst::VectorFst< fst::LogArc > &fst, const string &FileName)
{
  CreateDirectoryRecursively(FileName);
  fst.Write(FileName.c_str());
}


void FileReader::ApplyAcousticModelScalingFactor(double AcousticModelScalingFactor)
{
  fst::WeightedMapper AcousticModelScalingFactorMapper(AcousticModelScalingFactor);
  for (unsigned int InputFstIdx = 0; InputFstIdx < InputFsts.size(); InputFstIdx++) {
    fst::Map(&InputFsts[InputFstIdx], AcousticModelScalingFactorMapper);
//     std::cout << "Scaling FST: " << InputFstIdx << " with factor: " << AcousticModelScalingFactor << std::endl;
  }
}


void FileReader::ApplyWordEndTransducer()
{
  fst::VectorFst<fst::LogArc> WordEndTransducer;
  unsigned UNKState = WordEndTransducer.AddState();
  unsigned CharacterState = WordEndTransducer.AddState();
  WordEndTransducer.SetStart(UNKState);
  WordEndTransducer.SetFinal(UNKState, 0);
  WordEndTransducer.AddArc(CharacterState, fst::LogArc(EPS_SYMBOLID, UNKEND_SYMBOLID, 0, UNKState));
  WordEndTransducer.AddArc(CharacterState, fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, 0, UNKState));
  for (unsigned k = CHARACTERSBEGIN; k < InputStringToInt.GetIntToStringVector().size(); k++) {
    WordEndTransducer.AddArc(UNKState, fst::LogArc(k, k, 0, CharacterState));
    WordEndTransducer.AddArc(CharacterState, fst::LogArc(k, k, 0, CharacterState));
  }

//   PrintFST("lattice_debug/WordEndTransducer", GlobalStringToInt.GetIntToStringVector(), WordEndTransducer, true, NAMESANDIDS);

  for (unsigned int InputFstIdx = 0; InputFstIdx < InputFsts.size(); InputFstIdx++) {
    InputFsts[InputFstIdx] = fst::ComposeFst<fst::LogArc>(InputFsts[InputFstIdx], WordEndTransducer);
//     PrintFST("lattice_debug/" + InputFileNames[InputFstIdx] + "WordEnd", GlobalStringToInt.GetIntToStringVector(), InputFsts[InputFstIdx], true, NAMESANDIDS);
  }
}

void FileReader::ApplySentEndTransducer()
{
  fst::VectorFst<fst::LogArc> SentEndTransducer;
  unsigned CharacterState = SentEndTransducer.AddState();
  unsigned SentEndState = SentEndTransducer.AddState();
  unsigned FinalUNKState = SentEndTransducer.AddState();
  SentEndTransducer.SetStart(CharacterState);
  SentEndTransducer.SetFinal(FinalUNKState, 0);
  SentEndTransducer.AddArc(CharacterState, fst::LogArc(EPS_SYMBOLID, SENTEND_SYMBOLID, 0, SentEndState));
  SentEndTransducer.AddArc(CharacterState, fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, 0, CharacterState));
  for (unsigned k = CHARACTERSBEGIN; k < InputStringToInt.GetIntToStringVector().size(); k++) {
    SentEndTransducer.AddArc(CharacterState, fst::LogArc(k, k, 0, CharacterState));
  }
  SentEndTransducer.AddArc(SentEndState, fst::LogArc(EPS_SYMBOLID, UNKEND_SYMBOLID, 0, FinalUNKState));

//   PrintFST("lattice_debug/SentEndTransducer", GlobalStringToInt.GetIntToStringVector(), SentEndTransducer, true, NAMESANDIDS);

  for (unsigned int InputFstIdx = 0; InputFstIdx < InputFsts.size(); InputFstIdx++) {
    InputFsts[InputFstIdx] = fst::ComposeFst<fst::LogArc>(InputFsts[InputFstIdx], SentEndTransducer);
    fst::Connect(&InputFsts[InputFstIdx]);
//     PrintFST("lattice_debug/" + InputFileNames[InputFstIdx] + "SentenceEnd", GlobalStringToInt.GetIntToStringVector(), InputFsts[InputFstIdx], true, NAMESANDIDS);
  }
}

const vector< string > &FileReader::GetInputIntToStringVector() const
{
  return InputStringToInt.GetIntToStringVector();
}

const vector< string > &FileReader::GetReferenceIntToStringVector() const
{
  return ReferenceStringToInt.GetIntToStringVector();
}


const vector< string > &FileReader::GetInitIntToStringVector() const
{
  return InitStringToInt.GetIntToStringVector();
}


const vector< fst::VectorFst<fst::LogArc> > &FileReader::GetInputFsts() const
{
  return InputFsts;
}


const vector< fst::VectorFst< fst::LogArc > > &FileReader::GetReferenceFsts() const
{
  return ReferenceFsts;
}


const vector< fst::VectorFst< fst::LogArc > > &FileReader::GetInitFsts() const
{
  return InitFsts;
}


const vector< string > &FileReader::GetInputFileNames() const
{
  return InputFileNames;
}


const vector< string > &FileReader::GetReferenceFileNames() const
{
  return ReferenceFileNames;
}


const vector< string > &FileReader::GetInitFileNames() const
{
  return InitFileNames;
}


void FileReader::PrintFST(const string &pFileName, const vector< string > &words, const fst::VectorFst< fst::LogArc > &fst, bool printPDF, SymbolWriteModes SymbolWriteMode)
{
  WriteOpenFSTLattice(fst, pFileName);

  string fileNameSymI = pFileName + "_sym_i.txt";
  string fileNameSymO = pFileName + "_sym_o.txt";
  if (SymbolWriteMode != NONE) {
    WriteSymbols(pFileName + "_sym_i.txt", words, SymbolWriteMode);
    WriteSymbols(pFileName + "_sym_o.txt", words, SymbolWriteMode);
  }

  string fileNameBat = pFileName + "_draw.sh";
  FILE *batOut = std::fopen(fileNameBat.c_str(), "w");
  if (!batOut) {
    exit(10);
  }

  std::fprintf(batOut, "#!/bin/bash\n");
  if (SymbolWriteMode != NONE) {
    std::fprintf(batOut, "fstdraw --portrait=true --title=\"%s\" --isymbols=%s --osymbols=%s %s %s.dot\n", pFileName.c_str(), fileNameSymI.c_str(), fileNameSymO.c_str(), pFileName.c_str(), pFileName.c_str());
  } else {
    std::fprintf(batOut, "fstdraw --portrait=true --title=\"%s\" %s %s.dot\n", pFileName.c_str(), pFileName.c_str(), pFileName.c_str());
  }
  std::fprintf(batOut, "dot -Gcharset=latin1 -Gdpi=1200 -Tpdf %s.dot >%s.pdf", pFileName.c_str(), pFileName.c_str());
  std::fclose(batOut);
  chmod(fileNameBat.c_str(), 0744);

  std::printf("Printing %s: ", pFileName.c_str());
  if (printPDF) {
    int ret1 = system(("./" + fileNameBat).c_str());
    int ret2 = system(("rm " + fileNameBat).c_str());
    int ret3 = system(("rm " + fileNameSymI).c_str());
    int ret4 = system(("rm " + fileNameSymO).c_str());
    int ret5 = system(("rm " + pFileName).c_str());
    int ret6 = system(("rm " + pFileName + ".dot").c_str());
    std::printf("%i %i %i %i %i %i\n", ret1, ret2, ret3, ret4, ret5, ret6);
  }
}


void FileReader::WriteSymbols(const string &fileName, const vector< string > &words, SymbolWriteModes SymbolWriteMode)
{
  std::cout << "  Writing symbols to " << fileName << std::endl;
  std::ofstream symOut(fileName.c_str());
  for (unsigned i = 0; i < words.size(); i++) {
    if (SymbolWriteMode == NAMESANDIDS) {
      symOut << words.at(i) << "[" << i << "]\t" << i << endl;
    } else {
      symOut << words.at(i) << "\t" << i << endl;
    }
  }
  symOut.close();
}


void FileReader::PrintSentencesToFile(const std::string &FileName, const std::vector<std::vector<int> > &Sentences, const std::vector<std::string> &Id2CharacterSequence)
{
  CreateDirectoryRecursively(FileName);

  std::ofstream myfile;
  myfile.open(FileName);
  for (std::vector<std::vector<int> >::const_iterator sit = Sentences.begin(); sit != Sentences.end(); ++sit) {
//     DebugLib::PrintSentence(sit->begin(), sit->size(), Id2CharacterSequence);
    for (const_witerator wit = sit->begin(); wit != sit->end(); ++wit) {
      myfile << Id2CharacterSequence.at(*wit) << " ";
    }
    myfile << std::endl;
  }
  myfile.close();
}

void FileReader::CreateDirectoryRecursively(const std::string &DirectoryName, size_t PathSeperatorPosition)
{
  PathSeperatorPosition = DirectoryName.find_last_of('/', PathSeperatorPosition);
  if ((PathSeperatorPosition != string::npos) && (access(DirectoryName.substr(0, PathSeperatorPosition).c_str(), 0) != 0)) {
    CreateDirectoryRecursively(DirectoryName, PathSeperatorPosition - 1);
    mkdir(DirectoryName.substr(0, PathSeperatorPosition).c_str(), 0755);
  }
}