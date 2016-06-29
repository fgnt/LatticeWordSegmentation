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
 * class for the hierarchicl pitman yor (HPYLM) language model containing
 * a tree of restaurants for the different contexts and words
 */

/* Hierarchical pitman yor language model */
class HPYLM {
  struct ContextRestaurant;
  // int to pointer of context restaurants
  typedef google::dense_hash_map <int, ContextRestaurant *> ContextsHashmap;

  /* structure holding restaurant for current context
   * and references to previous and next context */
  struct ContextRestaurant {
    // Unique id of context
    const int ContextId;
    // Context sequence of current restaurant
    const std::vector<int> ContextSequence;
    // hashmap containing next restraurant in restaurant tree
    ContextsHashmap NextContext;
    // reference to the previous restaurant
    ContextRestaurant *const PreviousContext;
    // restaurant for this context
    Restaurant ThisRestaurant;
    
    // constructor for ContextRestaurant structure
    ContextRestaurant(
      const double &Discount_,
      const double &Concentration_,
      ContextRestaurant *PreviousContext_,
      int ContextId_,
      const std::vector<int> &ContextSequence_
    );
  };

  /* structure holding the posterior parameters
   * for the hyper parameter sampling */
  struct PosteriorParameters {
    // d_m = Beta(a, b);
    std::vector<double> a;
    // d_m = Beta(a, b);
    std::vector<double> b;
    // Theta_m = Gamma(alpha, beta)
    std::vector<double> alpha;
    // Theta_m = Gamma(alpha, beta)
    std::vector<double> beta;

    // Initialize vectors with prior parameters
    PosteriorParameters(int Order);
  };

  /* struct holding the hyper parameters */
  struct HPYLMParameters {
    // vector of discount paramters for different levels
    std::vector<double> Discount;
    // vector of concentation parameters for different levels
    std::vector<double> Concentration;

    // Initialize parameters with given parameters
    // (each level will get the same parameter)
    HPYLMParameters(
      unsigned int Order_,
      double Discount_,
      double Concentration_
    );
  };

  // Uniform random generator
  static std::default_random_engine RandomGenerator;
  // Gamma distribution for sampling of Concentration and Discount
  static std::gamma_distribution<double> GammaDistribution;
  // discrete distribution for word sampling
  static std::discrete_distribution<unsigned int> DiscreteDistribution;

  // Parameters of the hpylm
  // (discount and concentration for the different levels)
  HPYLMParameters Parameters;
  // root of the restaurant tree
  ContextRestaurant RestaurantTree;
  // order of the language model (1: unigram, 2: bigram, 3: trigram, ...)
  const unsigned int Order;
  // id for assignment to the next created restaurant (context)
  int NextUnusedContextId;
  // freed restaurant ids
  std::list<int> FreedIds;
  // set to true if freedids should be sorted before word adding
  bool SortFreedIds;
  // context id to context map
  ContextsHashmap ContextIdToContext;
  // scaling factor for base probabilities for words
  std::vector<double> BaseProbabilitiesScale;


  /* some internal functions */
  // internal function to recursively clean the restaurant tree
  void DestructRestaurantTreeRecursively(
    ContextRestaurant *CurrentRestaurant
  );

  // internal function to recursively add a word to the resaurant tree,
  // considdering its context  
  bool AddWordRecursively(
    const const_witerator &Word,
    unsigned int level,
    HPYLM::ContextRestaurant *CurrentRestaurant,
    double BaseProbability
  );

  // internal function to get the next availabe context id
  int GetNextAvailableContextId();

  // internal function to recursively remove a word from the resaurant tree,
  // considdering its context
  WordRemoveStatus RemoveWordRecursively(
    const const_witerator &Word,
    unsigned int level,
    HPYLM::ContextRestaurant *CurrentRestaurant
  );

  // internal function to recursively calculate the word probability
  // in the resaurant tree, considdering its context
  double WordProbabilityRecursively(
    const const_witerator &Word,
    unsigned int level,
    const HPYLM::ContextRestaurant &CurrentRestaurant,
    double BaseProbability
  ) const;

  // internal function to recursively claculate the word probabilities of
  // a word vector in the restaurant tree, considdering its context
  void WordVectorProbabilityRecursively(
    const const_witerator &Word,
    const std::vector< int > &Words,
    unsigned int level,
    unsigned int ContextLenght,
    const HPYLM::ContextRestaurant &CurrentRestaurant,
    std::vector< double > *BaseProbabilities
  ) const;

  // internal function to recursively search for the context id
  // of a given context sequence
  int GetContextIdRecursively(
    const const_witerator &Word,
    unsigned int level,
    unsigned int ContextLength,
    const HPYLM::ContextRestaurant &CurrentRestaurant
  ) const;

  // internal function to resample the hyper parameters
  void GetUpdatedPosteriorParametersRecursively(
    unsigned int level,
    const HPYLM::ContextRestaurant &CurrentRestaurant,
    HPYLM::PosteriorParameters *UpdatedPosteriorParameters
  ) const; 

  // internal function to recursively get the total number of tables per level
  void GetTotalTablecountPerLevelRecursively(
    unsigned int level,
    const HPYLM::ContextRestaurant &CurrentRestaurant,
    std::vector< int > *TotalTablecountPerLevel
  ) const;

  // internal function to recursively get the total number of words per level
  void GetTotalWordcountPerLevelRecursively(
    unsigned int level,
    const HPYLM::ContextRestaurant &CurrentRestaurant,
    std::vector< int > *TotalWordcountPerLevel
  ) const;

  // internal function to recursively get the total number of contexts per level
  void GetTotalContextcountPerLevelRecursively(
    unsigned int level,
    const HPYLM::ContextRestaurant &CurrentRestaurant,
    std::vector< int > *TotalContextcountPerLevel
  ) const;

  // internal function to recursively claculate the word probabilities of
  // a word vector in the restaurant tree, considdering its context
  // and draw a word from those probabilities
  int GenerateWordRecursively(
    const const_witerator &Word,
    const std::vector< int > &Words,
    unsigned int level,
    unsigned int ContextLenght,
    const HPYLM::ContextRestaurant &CurrentRestaurant,
    const std::vector< double > &BaseProbabilities
  ) const;

public:
  /* constructors/destructors */
  // construct hpylm of given order
  HPYLM(int Order_);
  // destruct hpylm
  ~HPYLM();

  /* interface */
  // add a word to the hpylm
  bool AddWord(
    const const_witerator &Word,
    double BaseProbability
  );

  // remove a word from the hpylm
  WordRemoveStatus RemoveWord(
    const const_witerator &Word
  );

  // calculate the probability of a word in the hpylm
  double WordProbability(
    const const_witerator &Word,
    double BaseProbability
  ) const;

  // calculate probabilities for all words in vector given context
  void WordVectorProbability(
    const std::vector< int > &ContextSequence,
    const std::vector< int > &Words,
    std::vector< double > *BaseProbabilities
  ) const;

  // calculate the log likelihood of a word sequence
  double WordSequenceLoglikelihood(
    const std::vector< int > &WordSequence,
    const google::dense_hash_map< int, double > &BaseProbabilities
  ) const;

  // calculate the probability of a word in the hpylm
  int GetContextId(
    const std::vector<int> &ContextSequence
  ) const;

  // resample the hyper parameters strengh and discount for each level
  void ResampleHyperParameters();

  // Get HPYLM discount and concentation
  const HPYLMParameters &GetHPYLMParameters() const;

  // Get possible transitions from one context to another
  ContextToContextTransitions GetTransitions(
    int ContextId,
    int SentEndSymbolId,
    const std::vector< bool > &ActiveWords
  ) const;

  // Returns next free context id
  int GetNextUnusedContextId() const;

  // return context sequence
  const std::vector< int > &GetContextSequence(
    int ContextId
  ) const;

  // return total word count per level
  std::vector< int > GetTotalWordcountPerLevel() const;
  // return total table count per level
  std::vector< int > GetTotalTablecountPerLevel() const;
  // get total number of contexts
  std::vector< int > GetTotalContextCountPerLevel() const;

  // draw one of the secified words according to their probabilites
  int GenerateWord(
    const std::vector< int > &ContextSequence,
    const std::vector< int > &Words,
    const std::vector< double > &BaseProbabilities,
    bool SampleFromBase
  ) const;

  int GetBaseTablesPerWord(
    int WordId
  ) const;

  void SetBaseProbabilitiesScale(
    const std::vector<double> &BaseProbabilitiesScale_
  );

  const std::vector<double> &GetBaseProbabilitiesScale() const;

  void SetDiscount(
    int Level,
    double Value
  );

  void SetConcentration(
    int Level,
    double Value
  );
};

#endif