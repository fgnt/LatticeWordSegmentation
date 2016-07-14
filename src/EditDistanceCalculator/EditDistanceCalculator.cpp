// ----------------------------------------------------------------------------
/**
   File: EditDistanceCalculator.cpp
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
#include <fst/compose.h>
#include <fst/shortest-path.h>
#include <thread>
#include "EditDistanceCalculator.hpp"
#include "definitions.hpp"

using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::cerr;


EditDistanceCalculator::EditDistanceCalculator(unsigned int NumThreads_, const std::vector<std::string> &FileNames_, const std::string &Prefix_, bool OutputEditOperations_) :
  NumThreads(NumThreads_),
  InsDelSubCorrNFoundNRef(6, 0),
  FileNames(FileNames_),
  Prefix(Prefix_),
  OutputEditOperations(OutputEditOperations_)
{
}


void EditDistanceCalculator::SetInputFstsAndReferenceSentences(const std::vector<fst::VectorFst< fst::StdArc > > &InputFsts_, const std::vector<std::vector< int > > &ReferenceSentences_, const vector< int > &InputAndOutputIds_, const vector< string > &Id2CharacterSequenceVector_)
{
  // initialize some variables
  NumSentences = InputFsts_.size();
  InputFsts = InputFsts_;
  ReferenceSentences = ReferenceSentences_;
  InputAndOutputIds = InputAndOutputIds_;
  Id2CharacterSequenceVector = Id2CharacterSequenceVector_;

  // build input and reference fsts
  ReferenceFsts.resize(NumSentences);
  for (unsigned int IdxSentence = 0; IdxSentence < NumSentences; IdxSentence++) {
    BuildFstFromIdSequence(ReferenceSentences_.at(IdxSentence), &ReferenceFsts.at(IdxSentence));
  }

  // build left and right factors
  BuildLeftAndRightFactors();

  // calculate edit Calculate edit distance
  CalculateEditDistance();
}


void EditDistanceCalculator::SetInputAndReferenceSentences(const vector< vector< int > > &InputSentences_, const vector< vector< int > > &ReferenceSentences_, const vector< int > &InputAndOutputIds_, const vector< string > &Id2CharacterSequenceVector_)
{
  // initialize some variables
//   std::cout << "Setting NumSentences" << std::endl;
  NumSentences = InputSentences_.size();
//   std::cout << "Setting InputSentences" << std::endl;
  InputSentences = InputSentences_;
//   std::cout << "Setting ReferenceSentences" << std::endl;
  ReferenceSentences = ReferenceSentences_;
//   std::cout << "Setting InputAndOutputId" << std::endl;
  InputAndOutputIds = InputAndOutputIds_;
//   std::cout << "Setting Id2CharacterSequenceVector" << std::endl;
  Id2CharacterSequenceVector = Id2CharacterSequenceVector_;

  // build input and reference fsts
//   std::cout << "build input and reference fsts" << std::endl;
  InputFsts.resize(NumSentences);
  ReferenceFsts.resize(NumSentences);
  for (unsigned int IdxSentence = 0; IdxSentence < NumSentences; IdxSentence++) {
    BuildFstFromIdSequence(InputSentences.at(IdxSentence), &InputFsts.at(IdxSentence));
    BuildFstFromIdSequence(ReferenceSentences.at(IdxSentence), &ReferenceFsts.at(IdxSentence));
  }

  // build left and right factors
//   std::cout << "build left and right factors" << std::endl;
  BuildLeftAndRightFactors();

  // calculate edit distance
//   std::cout << "calculate edit distance" << std::endl;
  CalculateEditDistance();
}


void EditDistanceCalculator::BuildFstFromIdSequence(const vector< int > &IdSequence, fst::VectorFst< fst::StdArc > *Fst)
{
  Fst->DeleteStates();
  Fst->ReserveStates(IdSequence.size() + 1);
  int CurrentState = Fst->AddState();
  Fst->SetStart(CurrentState);
  for (int Id : IdSequence) {
    int NextState = Fst->AddState();
    Fst->AddArc(CurrentState, fst::StdArc(Id, Id, 0, NextState));
    CurrentState = NextState;
  }
  Fst->SetFinal(CurrentState, 0);
}


void EditDistanceCalculator::BuildLeftAndRightFactors()
{
  std::sort(InputAndOutputIds.begin(), InputAndOutputIds.end());
  Id2CharacterSequenceVector.push_back("<ins>");
  InsertionId = InputAndOutputIds.back() + 1;
  Id2CharacterSequenceVector.push_back("<sub>");
  SubstitutionId = InputAndOutputIds.back() + 2;
  Id2CharacterSequenceVector.push_back("<del>");
  DeletionId = InputAndOutputIds.back() + 3;

  LeftFactor.AddState();
  LeftFactor.SetStart(0);
  LeftFactor.SetFinal(0, 0);
  RightFactor.AddState();
  RightFactor.SetStart(0);
  RightFactor.SetFinal(0, 0);

  LeftFactor.AddArc(0, fst::StdArc(EPS_SYMBOLID, InsertionId, 0.5, 0));
  RightFactor.AddArc(0, fst::StdArc(DeletionId, EPS_SYMBOLID, 0.5, 0));
  for (int InputAndOutputId : InputAndOutputIds) {
    LeftFactor.AddArc(0, fst::StdArc(InputAndOutputId, DeletionId, 0.5, 0));
    LeftFactor.AddArc(0, fst::StdArc(InputAndOutputId, SubstitutionId, 0.500001, 0));
    LeftFactor.AddArc(0, fst::StdArc(InputAndOutputId, InputAndOutputId, 0, 0));

    RightFactor.AddArc(0, fst::StdArc(InsertionId, InputAndOutputId, 0.5, 0));
    RightFactor.AddArc(0, fst::StdArc(SubstitutionId, InputAndOutputId, 0.500001, 0));
    RightFactor.AddArc(0, fst::StdArc(InputAndOutputId, InputAndOutputId, 0, 0));
  }
}


void EditDistanceCalculator::CalculateEditDistance()
{
  ResultFsts.resize(NumSentences);

//   std::cout << "initialize threads and variables" << std::endl;
  std::vector<std::thread> Threads(NumThreads - 1);
  int IdxRangeStep = std::ceil(NumSentences / static_cast<double>(NumThreads));
  std::vector<std::vector<int> > InsDelSubCorrNFoundNRefPerIdxRange(NumThreads - 1, std::vector<int>(6, 0));

  std::vector<fst::VectorFst<fst::StdArc> > RightFactors(NumThreads - 1, RightFactor);
  std::vector<fst::VectorFst<fst::StdArc> > LeftFactors(NumThreads - 1, LeftFactor);
//   std::cout << "starting threads: ";
  for (unsigned int IdxRange = 0; IdxRange < (NumThreads - 1); ++IdxRange) {
//     std::cout << IdxRange << " ";
    unsigned int StartIdx = IdxRange * IdxRangeStep;
    unsigned int EndIdx = std::min((IdxRange + 1) * IdxRangeStep, NumSentences);
    Threads[IdxRange] = std::thread(CalculateEditDistanceIdxRange, &InputFsts, &ReferenceFsts, &ResultFsts, &LeftFactors.at(IdxRange), &RightFactors.at(IdxRange), &InsDelSubCorrNFoundNRefPerIdxRange.at(IdxRange), StartIdx, EndIdx, &Id2CharacterSequenceVector, &FileNames, &Prefix, OutputEditOperations);
  }
//   std::cout << std::endl << "starting final thread" << std::endl;
  CalculateEditDistanceIdxRange(&InputFsts, &ReferenceFsts, &ResultFsts, &LeftFactor, &RightFactor, &InsDelSubCorrNFoundNRef, (NumThreads - 1) * IdxRangeStep, std::min(NumThreads * IdxRangeStep, NumSentences), &Id2CharacterSequenceVector, &FileNames, &Prefix, OutputEditOperations);

//   std::cout << "wating for threads and collecting data: " << std::endl;
  for (unsigned int IdxRange = 0; IdxRange < (NumThreads - 1); ++IdxRange) {
//     std::cout << IdxRange << ":";
    Threads[IdxRange].join();
//     std::cout << IdxRange;
    InsDelSubCorrNFoundNRef[0] += InsDelSubCorrNFoundNRefPerIdxRange[IdxRange][0];
    InsDelSubCorrNFoundNRef[1] += InsDelSubCorrNFoundNRefPerIdxRange[IdxRange][1];
    InsDelSubCorrNFoundNRef[2] += InsDelSubCorrNFoundNRefPerIdxRange[IdxRange][2];
    InsDelSubCorrNFoundNRef[3] += InsDelSubCorrNFoundNRefPerIdxRange[IdxRange][3];
    InsDelSubCorrNFoundNRef[4] += InsDelSubCorrNFoundNRefPerIdxRange[IdxRange][4];
    InsDelSubCorrNFoundNRef[5] += InsDelSubCorrNFoundNRefPerIdxRange[IdxRange][5];
//     std::cout << "(ok) ";
  }
}

void EditDistanceCalculator::CalculateEditDistanceIdxRange(const std::vector<fst::VectorFst< fst::StdArc > > *InputFsts, const std::vector<fst::VectorFst< fst::StdArc > > *ReferenceFsts, std::vector<fst::VectorFst< fst::StdArc > > *ResultFsts, const fst::VectorFst< fst::StdArc > *LeftFactor, const fst::VectorFst< fst::StdArc > *RightFactor, vector< int > *InsDelSubCorrNFoundNRef, unsigned int StartIdx, unsigned int EndIdx, const std::vector<std::string> *Id2CharacterSequenceVector, const std::vector<std::string> *Filenames, const std::string *Prefix, bool OutputEditOperations)
{
  for (unsigned int IdxLattice = StartIdx; IdxLattice < EndIdx; ++IdxLattice) {
    CalculateEditDistanceSingleIdx(InputFsts->at(IdxLattice), ReferenceFsts->at(IdxLattice), &ResultFsts->at(IdxLattice), *LeftFactor, *RightFactor, InsDelSubCorrNFoundNRef, Id2CharacterSequenceVector, &Filenames->at(IdxLattice), Prefix, OutputEditOperations);
  }
}


void EditDistanceCalculator::CalculateEditDistanceSingleIdx(const fst::VectorFst<fst::StdArc> &InputFst, const fst::VectorFst<fst::StdArc> &ReferenceFst, fst::VectorFst<fst::StdArc> *ResultFst, const fst::VectorFst<fst::StdArc> &LeftFactor, const fst::VectorFst<fst::StdArc> &RightFactor, std::vector<int> *InsDelSubCorrNFoundNRef, const std::vector<std::string> *Id2CharacterSequenceVector, const std::string *FileName, const std::string *Prefix, bool OutputEditOperations)
{
  fst::ComposeFstOptions<fst::StdArc> copt;
  copt.gc_limit = 0;

  fst::ComposeFst<fst::StdArc> Left(ReferenceFst, LeftFactor);
  fst::ComposeFst<fst::StdArc> Right(RightFactor, InputFst);
  fst::ArcSortFst<fst::StdArc, fst::ILabelCompare<fst::StdArc> > SortedRight(Right, fst::ILabelCompare<fst::StdArc>(), copt);
  fst::ComposeFst<fst::StdArc> Final(Left, SortedRight, copt);

  std::vector<fst::StdArc::Weight> Distance;
  fst::NaturalShortestFirstQueue<fst::StdArc::StateId, fst::StdArc::Weight> StateQueue(Distance);
  fst::ShortestPathOptions<fst::StdArc, fst::NaturalShortestFirstQueue<fst::StdArc::StateId, fst::StdArc::Weight>, fst::AnyArcFilter<fst::StdArc> > spopts(&StateQueue, fst::AnyArcFilter<fst::StdArc>());
  spopts.first_path = true;

  fst::ShortestPath(Final, ResultFst, &Distance, spopts);

//   fst::ArcMapFst<fst::StdArc, fst::LogArc, fst::StdToLogMapper > StdToLog4(*ResultFst, fst::StdToLogMapper());
//   FileReader::PrintFST("lattice_debug/Result.0", *Id2CharacterSequenceVector, fst::VectorFst<fst::LogArc>(StdToLog4), true, NAMESANDIDS);

  std::ofstream myfile;
  if (OutputEditOperations) {
    myfile.open(*Prefix + *FileName);
  }

  // get insertions, deletions and substitutions
  for (fst::StateIterator<fst::Fst<fst::StdArc> > siter(*ResultFst); !siter.Done(); siter.Next()) {
    for (fst::ArcIterator<fst::Fst<fst::StdArc> > aiter(*ResultFst, siter.Value()); !aiter.Done(); aiter.Next()) {
      if (aiter.Value().ilabel == EPS_SYMBOLID) {
        (*InsDelSubCorrNFoundNRef)[0]++;
        (*InsDelSubCorrNFoundNRef)[4]++;
        if (OutputEditOperations) {
//           std::cout << "i:[" << aiter.Value().ilabel << "," << Id2CharacterSequenceVector->at(aiter.Value().ilabel) << " --> " << aiter.Value().olabel << "," << Id2CharacterSequenceVector->at(aiter.Value().olabel) << "]" << std::endl;
          myfile << "i:[" << aiter.Value().ilabel << "," << Id2CharacterSequenceVector->at(aiter.Value().ilabel) << " --> " << aiter.Value().olabel << "," << Id2CharacterSequenceVector->at(aiter.Value().olabel) << "]" << std::endl;
        }
      } else if (aiter.Value().olabel == EPS_SYMBOLID) {
        (*InsDelSubCorrNFoundNRef)[1]++;
        (*InsDelSubCorrNFoundNRef)[5]++;
        if (OutputEditOperations) {
//           std::cout << "d:[" << aiter.Value().ilabel << "," << Id2CharacterSequenceVector->at(aiter.Value().ilabel) << " --> " << aiter.Value().olabel << "," << Id2CharacterSequenceVector->at(aiter.Value().olabel) << "]" << std::endl;
          myfile << "d:[" << aiter.Value().ilabel << "," << Id2CharacterSequenceVector->at(aiter.Value().ilabel) << " --> " << aiter.Value().olabel << "," << Id2CharacterSequenceVector->at(aiter.Value().olabel) << "]" << std::endl;
        }
      } else if (aiter.Value().ilabel != aiter.Value().olabel) {
        (*InsDelSubCorrNFoundNRef)[2]++;
        (*InsDelSubCorrNFoundNRef)[4]++;
        (*InsDelSubCorrNFoundNRef)[5]++;
        if (OutputEditOperations) {
//           std::cout << "s:[" << aiter.Value().ilabel << "," << Id2CharacterSequenceVector->at(aiter.Value().ilabel) << " --> " << aiter.Value().olabel << "," << Id2CharacterSequenceVector->at(aiter.Value().olabel) << "]" << std::endl;
          myfile << "s:[" << aiter.Value().ilabel << "," << Id2CharacterSequenceVector->at(aiter.Value().ilabel) << " --> " << aiter.Value().olabel << "," << Id2CharacterSequenceVector->at(aiter.Value().olabel) << "]" << std::endl;
        }
      } else {
        (*InsDelSubCorrNFoundNRef)[3]++;
        (*InsDelSubCorrNFoundNRef)[4]++;
        (*InsDelSubCorrNFoundNRef)[5]++;
        if (OutputEditOperations) {
//           std::cout << "c:[" << aiter.Value().ilabel << "," << Id2CharacterSequenceVector->at(aiter.Value().ilabel) << " --> " << aiter.Value().olabel << "," << Id2CharacterSequenceVector->at(aiter.Value().olabel) << "]" << std::endl;
          myfile << "c:[" << aiter.Value().ilabel << "," << Id2CharacterSequenceVector->at(aiter.Value().ilabel) << " --> " << aiter.Value().olabel << "," << Id2CharacterSequenceVector->at(aiter.Value().olabel) << "]" << std::endl;
        }
      }
    }
  }
  if (OutputEditOperations) {
    myfile.close();
  }
}

const vector< int > &EditDistanceCalculator::GetInsDelSubCorrNFoundNRef() const
{
  return InsDelSubCorrNFoundNRef;
}
