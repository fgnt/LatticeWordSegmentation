// ----------------------------------------------------------------------------
/**
   File: HPYLM.hpp

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

   Description: hierarchical pitman yor language model

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _HPYLM_HPP_
#define _HPYLM_HPP_

#include "Restaurant.hpp"

/*
 * class for the hierarchicl pitman yor (HPYLM) language model containing a tree of restaurants for the different contexts and words
 */

/* Hierarchical pitman yor language model */
class HPYLM {
  struct ContextRestaurant;
  typedef google::dense_hash_map <int, ContextRestaurant *> ContextsHashmap; // int to pointer of context restaurants

  /* structure holding restaurant for current context and references to previous and next context */
  struct ContextRestaurant {
    const int ContextId;                      // Unique id of context
    const std::vector<int> ContextSequence;   // Context sequence of current restaurant
    ContextsHashmap NextContext;              // hashmap containing next restraurant in restaurant tree
    ContextRestaurant *const PreviousContext; // reference to the previous restaurant
    Restaurant ThisRestaurant;                // restaurant for this context
    ContextRestaurant(const double &Discount_, const double &Concentration_, ContextRestaurant *PreviousContext_, int ContextId_, const std::vector<int> &ContextSequence_); // constructor for ContextRestaurant structure
  };

  /* structure holding the posterior parameters for the hyper parameter sampling */
  struct PosteriorParameters {
    std::vector<double> a;               // d_m = Beta(a, b);
    std::vector<double> b;               // d_m = Beta(a, b);
    std::vector<double> alpha;           // Theta_m = Gamma(alpha, beta)
    std::vector<double> beta;            // Theta_m = Gamma(alpha, beta)
    PosteriorParameters(int Order);      // Initialize vectors with prior parameters
  };

  /* struct holding the hyper parameters */
  struct HPYLMParameters {
    std::vector<double> Discount;          // vector of discount paramters for different levels
    std::vector<double> Concentration;     // vector of concentation parameters for different levels
    HPYLMParameters(unsigned int Order_, double Discount_, double Concentration_);  // Initialize parameters with given parameters (each level will get the same parameter)
  };

  static std::default_random_engine RandomGenerator;                    // Uniform random generator
  static std::gamma_distribution<double> GammaDistribution;             // Gamma distribution for sampling of Concentration and Discount
  static std::discrete_distribution<unsigned int> DiscreteDistribution; // discrete distribution for word sampling

  HPYLMParameters Parameters;                    // Parameters of the hpylm (discount and concentration for the different levels)
  ContextRestaurant RestaurantTree;              // root of the restaurant tree
  const unsigned int Order;                      // order of the language model (1: unigram, 2: bigram, 3: trigram, ...)
  int NextUnusedContextId;                       // id for assignment to the next created restaurant (context)
  std::list<int> FreedIds;                       // freed restaurant ids
  bool SortFreedIds;                             // set to true if freedids should be sorted before word adding
  ContextsHashmap ContextIdToContext;            // context id to context map
  std::vector<double> BaseProbabilitiesScale;    // scaling factor for base probabilities for words


  /* some internal functions */
  void DestructRestaurantTreeRecursively(ContextRestaurant *CurrentRestaurant);                                                                                                       // internal function to recursively clean the restaurant tree
  bool AddWordRecursively(const const_witerator &Word, unsigned int level, HPYLM::ContextRestaurant *CurrentRestaurant, double BaseProbability);                                      // internal function to recursively add a word to the resaurant tree, considdering its context
  int GetNextAvailableContextId();                                                                                                                                                    // internal function to get the next availabe context id
  WordRemoveStatus RemoveWordRecursively(const const_witerator &Word, unsigned int level, HPYLM::ContextRestaurant *CurrentRestaurant);                                               // internal function to recursively remove a word from the resaurant tree, considdering its context
  double WordProbabilityRecursively(const const_witerator &Word, unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, double BaseProbability) const;                // internal function to recursively calculate the word probability in the resaurant tree, considdering its context
  void WordVectorProbabilityRecursively(const const_witerator &Word, const std::vector< int > &Words, unsigned int level, unsigned int ContextLenght, const HPYLM::ContextRestaurant &CurrentRestaurant, std::vector< double > *BaseProbabilities) const; // internal function to recursively claculate the word probabilities of a word vector in the restaurant tree, considdering its context
  int GetContextIdRecursively(const const_witerator &Word, unsigned int level, unsigned int ContextLength, const HPYLM::ContextRestaurant &CurrentRestaurant) const;                  // internal function to recursively search for the context id of a given context sequence                                               // internal function to recursively calculate the word probability in the resaurant tree, considdering its context
  void GetUpdatedPosteriorParametersRecursively(unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, HPYLM::PosteriorParameters *UpdatedPosteriorParameters) const; // internal function to resample the hyper parameters
  void GetTotalTablecountPerLevelRecursively(unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, std::vector< int > *TotalTablecountPerLevel) const;               // internal function to recursively get the total numeber of tables per level
  void GetTotalWordcountPerLevelRecursively(unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, std::vector< int > *TotalWordcountPerLevel) const;                 // internal function to recursively get the total numeber of words per level
  void GetTotalContextcountPerLevelRecursively(unsigned int level, const HPYLM::ContextRestaurant &CurrentRestaurant, std::vector< int > *TotalContextcountPerLevel) const;           // internal function to recursively get the total numeber of contexts per level
  int GenerateWordRecursively(const const_witerator &Word, const std::vector< int > &Words, unsigned int level, unsigned int ContextLenght, const HPYLM::ContextRestaurant &CurrentRestaurant, const std::vector< double > &BaseProbabilities) const; // internal function to recursively claculate the word probabilities of a word vector in the restaurant tree, considdering its context and draw a word from those probabilities

public:
  /* transitions from one to the next context */
  struct ContextToContextTransitions {
    std::vector<int> Words;            // word ids for transitions
    std::vector<int> NextContextIds;   // next context id after transistion
    std::vector<double> Probabilities; // probabilities of transitions
    bool HasTransitionToSentEnd;       // indicator of transition to sentence end is present
    ContextToContextTransitions();     // initialize transitions object
  };

  /* constructors/destructors */
  HPYLM(int Order_); // construct hpylm of given order
  ~HPYLM();          // destruct hpylm

  /* interface */
  bool AddWord(const const_witerator &Word, double BaseProbability);                     // add a word to the hpylm
  WordRemoveStatus RemoveWord(const const_witerator &Word);                              // remove a word from the hpylm
  double WordProbability(const const_witerator &Word, double BaseProbability) const;     // calculate the probability of a word in the hpylm
  void WordVectorProbability(const std::vector< int > &ContextSequence, const std::vector< int > &Words, std::vector< double > *BaseProbabilities) const; // calculate probabilities for all words in vector given context
  double WordSequenceLoglikelihood(const std::vector< int > &WordSequence, const google::dense_hash_map< int, double > &BaseProbabilities) const;   // calculate the log likelihood of a word sequence
  int GetContextId(const std::vector<int> &ContextSequence) const;                       // calculate the probability of a word in the hpylm
  void ResampleHyperParameters();                                                        // resample the hyper parameters strengh and discount for each level
  const HPYLMParameters &GetHPYLMParameters() const;                                     // Get HPYLM discount and concentation
  ContextToContextTransitions GetTransitions(int ContextId, int SentEndSymbolId, const std::vector< bool > &ActiveWords) const; // Get possible transitions from one context to another
  int GetNextUnusedContextId() const;                                                    // Returns next free context id
  const std::vector< int > &GetContextSequence(int ContextId) const;                     // return context sequence
  std::vector< int > GetTotalWordcountPerLevel() const;                                  // return total word count per level
  std::vector< int > GetTotalTablecountPerLevel() const;                                 // return total table count per level
  std::vector< int > GetTotalContextCountPerLevel() const;                               // get total number of contexts
  int GenerateWord(const std::vector< int > &ContextSequence, const std::vector< int > &Words, const std::vector< double > &BaseProbabilities, bool SampleFromBase) const; // draw one of the secified words according to their probabilites
  int GetBaseTablesPerWord(int WordId) const;
  void SetBaseProbabilitiesScale(const std::vector<double> &BaseProbabilitiesScale_);
  const std::vector<double> &GetBaseProbabilitiesScale() const;
  void SetDiscount(int Level, double Value);
  void SetConcentration(int Level, double Value);
};

#endif