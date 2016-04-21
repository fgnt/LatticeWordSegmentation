// ----------------------------------------------------------------------------
/**
   File: Dictionary.hpp

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

   Description: dictionary for character id sequence to word id mapping

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial
*/
// ----------------------------------------------------------------------------
#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include "definitions.hpp"

/* dicitionary class */
class Dictionary {
  Word2IdHashmap Word2Id;                           // charactersequence to word id mapper
  int MaxId;                                        // current maximum word id
  const int WordsBegin;                             // first word id
  std::list<int> FreedIds;                          // list with freed ids because of removed words
  const unsigned int CHPYLMContextLength;           // Order of character level hierarchical pitman yor model
  Id2WordHashmap Id2Word;                           // index to word mapping
  Id2CharacterSequenceHashmap Id2CharacterSequence; // index to character sequence mapping
  bool SortFreedIds;                                // set to true if freedids should be sorted before word adding

  /* some internal functions */
  void AddWordToId2CharacterSequence(const_citerator c, unsigned int length, int WordId); // add the written form for the added word to the symbols

public:
  /* constructor */
  Dictionary(unsigned int CHPYLMContextLength_, const std::vector<std::string> &Symbols_); // construct dictionary

  /* interface */
  WordIdAddedPair AddCharacterIdSequenceToDictionary(const const_citerator &c, unsigned int length);  // add word to dictionary given iterator to vector of ints and word length
  int GetWordId(const const_citerator &c, unsigned int length) const;                                 // return word id given iterator to vector of ints and word length
  void RemoveWordFromDictionary(int OldWordId);                                                       // remove word from dictionary given word id
  const Id2WordHashmap &GetId2Word() const;                                                           // return Id2Word hashmap
  const Word2IdHashmap &GetWord2Id() const;                                                           // return word to id hashmap
  WordBeginLengthPair GetWordBeginLength(int WordId) const;                                           // return a pair containing Word.begin() iterators and length
  int GetWordLength(int WordId) const;                                                                // return Word length
  std::vector<std::string> GetId2CharacterSequenceVector() const;                                     // construct and return a Id2CharacterSequence vector
  std::vector<int> GetWordLengthVector() const;                                                       // return vector with word lengths
  std::vector<std::vector<std::string>> GetId2SeparatedCharacterSequenceVector() const;               // construct and return a Id2SeparatedCharacterSequenceVector vector
  int GetMaxNumWords() const;                                                                         // return maximum number of words
  int GetWordsBegin() const;                                                                          // get first word id
  const std::vector<int> &GetWordVector(int WordId) const;                                            // return stored word vector from lexicon
};

#endif
