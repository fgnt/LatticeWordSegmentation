// ----------------------------------------------------------------------------
/**
   File: Dictionary.cpp
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
#include "Dictionary.hpp"

/** construct dictionary **/
Dictionary::Dictionary(unsigned int CHPYLMContextLength_,
                       const std::vector< std::string > &Symbols_) :
  Word2Id(),
  MaxId(Symbols_.size()),
  WordsBegin(Symbols_.size()),
  FreedIds(),
  CHPYLMContextLength(CHPYLMContextLength_),
  Id2Word(),
  Id2CharacterSequence(),
  SortFreedIds(false)
{
  Word2Id.set_deleted_key(std::vector<int>(1, DELETED));
  Word2Id.set_empty_key(std::vector<int>(1, EMPTY));
  Id2Word.set_deleted_key(DELETED);
  Id2Word.set_empty_key(EMPTY);
  Id2CharacterSequence.set_deleted_key(DELETED);
  Id2CharacterSequence.set_empty_key(EMPTY);
  for (std::size_t SymbolIdx = 0; SymbolIdx < Symbols_.size(); ++SymbolIdx) {
    const auto& Symbol = Symbols_.at(SymbolIdx);
    Id2CharacterSequence.insert(std::make_pair(SymbolIdx, Symbol));
  }
}


/** return word id given iterator to vector of ints and word length **/
int Dictionary::GetWordId(const const_citerator &c, unsigned int length) const
{
  std::vector<int> WordVector(c, c + length);
  Word2IdHashmap::const_iterator it = Word2Id.find(WordVector);
  int WordId;
  if (it == Word2Id.end()) {
    WordId = UNKNOWN;
  } else {
    WordId = it->second;
  }
  return WordId;
}


/** add word to dictionary given iterator to vector of ints and word length **/
WordIdAddedPair Dictionary::AddCharacterIdSequenceToDictionary(
    const const_citerator &c, unsigned int length)
{
  std::vector<int> WordVector(c, c + length);
  const auto it = Word2Id.find(WordVector);
  if (it == Word2Id.end()) {
    int WordId;

    /* get next availabe word id */
    if (FreedIds.empty()) {
      WordId = MaxId++;
    } else {
      if (SortFreedIds) {
        FreedIds.sort();
        SortFreedIds = false;
      }
      WordId = FreedIds.front();
      FreedIds.pop_front();
//       std::cout << "Reusing: " << WordId << std::endl;
    }

    /* add word */
    Word2Id[WordVector] = WordId;
    Id2Word[WordId] = std::vector<int>(CHPYLMContextLength, EOW);
    Id2Word[WordId].insert(Id2Word[WordId].end(), c, c + length);
    Id2Word[WordId].push_back(EOW);
    AddWordToId2CharacterSequence(c, length, WordId);
    return std::make_pair(WordId, true);
  } else {
    return std::make_pair(it->second, false);
  }
}


/** remove word from dictionary given word id **/
void Dictionary::RemoveWordFromDictionary(int OldWordId)
{
  std::vector<int> WordVector(Id2Word[OldWordId].begin() + CHPYLMContextLength,
                              Id2Word[OldWordId].end() - 1);
  Word2Id.erase(WordVector);
  Id2Word.erase(OldWordId);
  Id2CharacterSequence.erase(OldWordId);
  FreedIds.push_back(OldWordId);
  SortFreedIds = true;
//     std::cout << "Pushing back: " << WordId << std::endl;
}

/** return word vector corresponding to word id **/
const std::vector<int> &Dictionary::GetWordVector(int WordId) const
{
  return Id2Word.find(WordId)->second;
}

/** return complete id to word hashmap **/
const Id2WordHashmap &Dictionary::GetId2Word() const
{
  return Id2Word;
}

const Word2IdHashmap &Dictionary::GetWord2Id() const
{
  return Word2Id;
}


/** return begin and length for character id sequence of given word id **/
WordBeginLengthPair Dictionary::GetWordBeginLength(int WordId) const
{
  Id2WordHashmap::const_iterator it = Id2Word.find(WordId);
  return std::make_pair(it->second.begin() + CHPYLMContextLength,
                        it->second.size() - CHPYLMContextLength - 1);
}

/** return legth for character id sequence of given word id **/
int Dictionary::GetWordLength(int WordId) const
{
  return Id2Word.find(WordId)->second.size() - CHPYLMContextLength - 1;
}


/** insert written form a a word into symbol map **/
void Dictionary::AddWordToId2CharacterSequence(const_citerator c,
                                               unsigned int length, int WordId)
{
  std::ostringstream oss;
  for (unsigned int i = 0; i < length; i++) {
    oss << Id2CharacterSequence[*(c + i)];
  }
  Id2CharacterSequence.insert(make_pair(WordId, oss.str()));
}

/** return vector of strings containing characters and words **/
std::vector<std::string> Dictionary::GetId2CharacterSequenceVector() const
{
  std::vector<std::string> Id2CharacterSequenceVector(MaxId);
  for (auto CharSequence = Id2CharacterSequence.begin();
       CharSequence != Id2CharacterSequence.end(); ++CharSequence)
  {
    Id2CharacterSequenceVector[CharSequence->first] = CharSequence->second;
  }
  return Id2CharacterSequenceVector;
}

/** return vector of ints containing word lengths **/
std::vector<int> Dictionary::GetWordLengthVector() const
{
  std::vector<int> WordLengthVector(MaxId);
  for (const auto& Word: Id2Word) {
    WordLengthVector[Word.first] = Word.second.size() - CHPYLMContextLength - 1;
  }
  return WordLengthVector;
}


/** return vector of vector of strings containing the characters sequence for each word **/
std::vector<std::vector<std::string>>
    Dictionary::GetId2SeparatedCharacterSequenceVector() const
{
  std::vector<std::vector<std::string> > Id2SeparatedCharacterSequenceVector(MaxId);
  for (const auto& CharacterIdSequence: Id2Word) {
    for (const_citerator CharacterId =
              CharacterIdSequence.second.begin() + CHPYLMContextLength;
         CharacterId != CharacterIdSequence.second.end(); ++CharacterId)
    {
      auto currentCharacter(Id2CharacterSequence.find(*CharacterId)->second);
      Id2SeparatedCharacterSequenceVector[CharacterIdSequence.first].push_back(
            std::move(currentCharacter)
      );
    }
  }
  return Id2SeparatedCharacterSequenceVector;
}


/** return maximum numer of words **/
int Dictionary::GetMaxNumWords() const
{
  return MaxId;
}

int Dictionary::GetWordsBegin() const
{
  return WordsBegin;
}
