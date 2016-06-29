// ----------------------------------------------------------------------------
/**
   File: definitions.hpp

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

   Description: some definitions used in the language model

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _DEFINITIONS_HPP_
#define _DEFINITIONS_HPP_

#include <sparsehash/dense_hash_map>
#include <boost/functional/hash.hpp>

#define PrintDebugHeader std::cout << __FILE__ << " line:" << __LINE__ << " funtion:" << __FUNCTION__

enum SpecialSyms{
  EOS = 5,
  SOS = 4,
  EOW = 3,
  SOW = 2,
  PHI = 1,
  UNKNOWN = -32766,
  DELETED = -32767,
  EMPTY = -32768
};

// What has been removed:
enum WordRemoveStatus {
  NONEREMOVED,          // Nothing,
  TABLE,                // a table for the word,
  TABLE_WORD,           // the word has been removed from the restaurant
  TABLE_WORD_RESTAURANT // the restaurant has been removed
};

typedef std::vector<int>::iterator citerator; // vector of characters iterator
typedef std::vector<int>::iterator witerator; // vector of words iterator
typedef std::vector<int>::iterator iiterator; // vector of ints iterator

typedef std::vector<int>::const_iterator const_citerator; // const vector of characters iterator
typedef std::vector<int>::const_iterator const_witerator; // const vector of words iterator
typedef std::vector<int>::const_iterator const_iiterator; // const vector of ints iterator

typedef std::pair<const_iiterator, int> WordBeginLengthPair; // pair containing word begin and werd length
typedef std::pair<int, bool> WordIdAddedPair;                // pair containing word id and boolean indicating if word was added to dictionary

typedef google::dense_hash_map<int, std::vector<int> > Id2WordHashmap;                                  // int to vector of ints map
typedef google::dense_hash_map<int, std::string> Id2CharacterSequenceHashmap;                           // int to vector of strings map
typedef google::dense_hash_map<std::vector<int>, int, boost::hash< std::vector<int> > > Word2IdHashmap; // vector to int hashmap

struct NHPYLMParameters {
    const std::vector<double> &CHPYLMDiscount;      // discount parameters of hierarchical character pitman yor language model
    const std::vector<double> &CHPYLMConcentration; // concentration parameter of hierarchical character pitman yor language model
    const std::vector<double> &WHPYLMDiscount;      // discount parameters of hierarchical word pitman yor language model
    const std::vector<double> &WHPYLMConcentration; // concentration parameters of hierarchical word pitman yor language model
    NHPYLMParameters(const std::vector< double > &CHPYLMDiscount_, const std::vector< double > &CHPYLMConcentration_, const std::vector< double > &WHPYLMDiscount_, const std::vector< double > &WHPYLMConcentration_); // initialize parameters
};

/* transitions from one to the next context */
struct ContextToContextTransitions {
    std::vector<int> Words;            // word ids for transitions
    std::vector<int> NextContextIds;   // next context id after transistion
    std::vector<double> Probabilities; // probabilities of transitions
    bool HasTransitionToSentEnd;       // indicator of transition to sentence end is present
    ContextToContextTransitions();     // initialize transitions object
};

#endif