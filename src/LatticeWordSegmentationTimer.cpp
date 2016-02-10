// ----------------------------------------------------------------------------
/**
   File: LatticeWordSegmentationTimer.cpp
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
#include <iostream>
#include <iomanip>
#include "LatticeWordSegmentationTimer.hpp"

LatticeWordSegmentationTimer::LatticeWordSegmentationTimer(int MaxNumThreads, int NumTimersPerThread) :
  tInSamples(MaxNumThreads, std::vector<SimpleTimer>(NumTimersPerThread))
{
}

LatticeWordSegmentationTimer::SimpleTimer::SimpleTimer() :
  Duration(0)
{
}

void LatticeWordSegmentationTimer::SimpleTimer::SetStart()
{
  Start = std::chrono::high_resolution_clock::now();
}

void LatticeWordSegmentationTimer::SimpleTimer::AddTimeSinceStartToDuration()
{
  Duration += std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now() - Start);
}

double LatticeWordSegmentationTimer::SimpleTimer::GetDuration() const
{
  return Duration.count();
}

void LatticeWordSegmentationTimer::PrintTimingStatistics() const
{
  // output some timing statistics
  std::cout << std::fixed << std::setprecision(4);
  std::cout << " Build LexFST:       " << std::right << std::setw(8) << tLexFst.GetDuration() << " s\n"
            << " Removing:           " << std::right << std::setw(8) << tRemove.GetDuration() << " s\n"
            << " Sampling:           " << std::right << std::setw(8) << tSample.GetDuration() << " s\n";
  int IdxThread = 0;
  for (const auto & tInSample : tInSamples) {
    std::cout << "  Thread[" << IdxThread++ << "]:         ";
    for (const auto & t : tInSample) {
      std::cout << std::right << std::setw(8) << t.GetDuration() << " s ";
    }
    std::cout << std::endl;
  }
  std::cout << std::right << std::setw(8)
            << " Parsing and adding: " << std::right << std::setw(8) << tParseAndAdd.GetDuration() << " s\n"
            << " Parameter sampling: " << std::right << std::setw(8) << tHypSample.GetDuration() << " s\n"
            << " Perplexity calc.:   " << std::right << std::setw(8) << tCalcPerplexity.GetDuration() << " s\n"
            << " WER calculation:    " << std::right << std::setw(8) << tCalcWER.GetDuration() << " s\n"
            << " PER calculation:    " << std::right << std::setw(8) << tCalcPER.GetDuration() << " s\n\n";
}
