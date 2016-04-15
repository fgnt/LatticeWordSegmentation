// ----------------------------------------------------------------------------
/**
   File: WordLengthProbCalculator.hpp

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

   Description: calculate word length probabilities and scalings

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _WORDLENGTHPROBCACLULATOR_HPP_
#define _WORDLENGTHPROBCACLULATOR_HPP_

#include <vector>
#include "NHPYLM/NHPYLM.hpp"

/* class to calculate some word length probability distribution */
class WordLengthProbCalculator {
  std::vector<double> GeneratedLengthDistribution;       // length distribution of generated sentences
  std::vector<double> ObervedLengthDistribution;         // length distribution of observed sentences
  double MeanObservedLength;                             // mean lengh of observed sentences
  std::vector<double> ScaledWordLengthProbabilityVector; // scaling vector for probabilites depending on word length
  bool UpdateScaledWordLengthProbabilityVector;          // update sacling vector before ouput

  /* some internal functions */
  // calculate factorial of n
  double factorial(
    double n
  ) const;

  // return word length probility
  double WordLengthProbability(
    int WordLength
  ) const;
  
  // return scaling factor for probabilites
  double GetScaledWordLengthProbability(
    int WordLength
  ) const;
  
  // set the generated length distribution 
  void SetGeneratedLengthDistribution(
    const std::vector<double> &GeneratedLengthDistribution_
  );
  
  // set the observed length distribution
  void SetObservedLengthDistribution(
    const std::vector<double> &ObervedLengthDistribution_
  );
  
  // set a fixed mean for the poison distribution
  void SetDesiredLengthMean(
    double DesiredLengthMean_
  );
  
  // return scaling vector for probabilites depending on word length
  const std::vector<double> &GetScaledWordLengthProbabilityVector();

public:
  /* interface */
  // update scaling vector for probabilites
  // depending on word lengths in language model
  static void UpdateWHPYLMBaseProbabilitiesScale(
    NHPYLM *LanguageModel,
    int WordLengthModulation
  );
};

#endif