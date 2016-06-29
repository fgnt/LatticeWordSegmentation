// ----------------------------------------------------------------------------
/**
   File: Restaurant.hpp

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

   Description: chinese restaurant process language model

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _RESTAURANT_HPP_
#define _RESTAURANT_HPP_

#include "definitions.hpp"

#include <random>

/*
 * class for one restaurant containing the different words
 */

/* Restaurant class holding: c_u.. and t_u.*/
class Restaurant {
  /* Tablegroup holding: c_uw., c_uwk and t_uw */
  struct WordTableGroup {
    unsigned int Wordcount;                   // Number of times the Word exists in the WordTableGroup
    std::vector<unsigned int> TableWordcount; // Wordcount for the Word in each table in the WordTableGroup
    unsigned int GroupTableCount;             // Number of ocupied tables in WordTableGroup
    WordTableGroup();                         // Constructor: initialite wordtablegroup to default values
  };
  typedef google::dense_hash_map <int, WordTableGroup> WordsHashmap; // hashmap mapping from int to WordTableGroup

  WordsHashmap Words;           // Hashmap to hold the WordTableGroups for each word
  unsigned int TotalWordCount;  // total number of words in restaurant
  unsigned int TotalTableCount; // number of tables in restaurant

  const double &Discount;       // Discount parameter for restaurant
  const double &Concentration;  // Concentration parameter for restaurant

  static std::default_random_engine RandomGenerator;                  // Uniform sandom generator
  static std::discrete_distribution<unsigned int> TableDistribution;  // discrete distribution for table sampling
  static std::vector<double> TableProbabilities;                      // vector used to hold probabilities for tables sampling
  static std::bernoulli_distribution BernoulliDistribution;           // bernoulli distribution for auxiliary variable Yui and Zwkj
  static std::gamma_distribution<double> GammaDistribution;           // Gamma distribution for sampling of Xu
public:
  /* constructor */
  Restaurant(const double &Discount_, const double &Concentration_);  // construct restaurant

  /* interface */
  bool IncrementWordCount(int Word, double BaseProbability);             // increment word count for given word in restaurant
  WordRemoveStatus DecrementWordCount(int Word);                         // decrement word count for given word in restaurant
  double WordProbability(int Word, double BaseProbability) const;        // get predictive probability of word in restaurant
  void WordVectorProbability(const std::vector<int> &WordVector, std::vector<double> *BaseProbabilities) const; // get predictive probability for all words in word vector
  unsigned int GetOneMinusYuiSum() const;                                // Sum over auxiliary variables (1 - Yui)
  unsigned int GetOneMinusZuwkjSum() const;                              // Sum over auxiliary varaibles Zuwk
  double GetYuiSum() const;                                              // Sum over auxiliary variables Yui
  double GetLogXu() const;                                               // Sum over auxiliary variables log(Xu)
  std::vector<int> GetWords(const std::vector<bool> &ActiveWords) const; // Return all words in this restaurant
  double GetTotalWordCount() const;                                      // return total number of words in restaurant
  double GetTotalTableCount() const;                                     // return total number of tables in restaurant
  int GetTablesPerWord(int WordId) const;                                // return totoal number of tables per word
};

#endif
