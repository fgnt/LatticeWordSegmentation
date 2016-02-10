// ----------------------------------------------------------------------------
/**
   File: LexFst.cpp
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
#include "LexFst.hpp"

LexFst::LexFst(bool Debug_, const vector< string > &Symbols_, int CharactersBegin_, const std::vector<double> &CharacterSequenceProbabilityScale_) :
  Debug(Debug_),
  Symbols(Symbols_),
  CharactersBegin(CharactersBegin_),
  CharactersEnd(Symbols.size()),
  CharacterSequenceProbabilityScale(CharacterSequenceProbabilityScale_)
{
  initializeArcs();
}

void LexFst::initializeArcs()
{
  // initialize the states
  HomeState = AddState();
  SetStart(HomeState);
  SetFinal(HomeState, 0);
  if (CharacterSequenceProbabilityScale.empty()) {
    UnkState = AddState();
    AddArc(UnkState, fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, 0, HomeState));   // end of unknown word
    for (int i = CharactersBegin; i < CharactersEnd; i++) {
      AddArc(UnkState, fst::LogArc(i, i, 0, UnkState));
    }
    AddArc(HomeState, fst::LogArc(PHI_SYMBOLID, EPS_SYMBOLID, 0, UnkState));
  } else {
    AddState();
    UnkState = AddState();
    AddArc(UnkState, fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, -log(CharacterSequenceProbabilityScale[2]), HomeState));   // end of unknown word
    for (int i = CharactersBegin; i < CharactersEnd; i++) {
      AddArc(UnkState - 1, fst::LogArc(i, i, 0, UnkState));
    }
    AddArc(HomeState, fst::LogArc(PHI_SYMBOLID, EPS_SYMBOLID, 0, UnkState - 1));
    for (unsigned int WordLength = 3; WordLength < CharacterSequenceProbabilityScale.size(); WordLength++) {
      AddState();
      AddArc(WordLength, fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, -log(CharacterSequenceProbabilityScale[WordLength]), HomeState));   // end of unknown word
      for (int i = CharactersBegin; i < CharactersEnd; i++) {
        AddArc(WordLength - 1, fst::LogArc(i, i, 0, WordLength));
      }
    }
  }
}


void LexFst::BuildLexiconTansducer(const Word2IdHashmap &Word2Id)
{
  for (Word2IdHashmap::const_iterator it = Word2Id.begin(); it != Word2Id.end(); ++it) {
    addWord(it->first.begin(), it->first.size(), it->second);
  }
}


void LexFst::addWord(std::vector< int >::const_iterator WordBegin, int WordLength, int WordId)
{
//     cout << "Lex: adding Word ";
//     for ( unsigned c=0; c < WordLength; c++ )
//         cout << *(WordBegin + c) << " ";
//     cout << endl;

  if (WordLength == 0) {
    return;
  }

  // Add the word to the transducer
  // cout << "Add to transducer" << endl;
  int curState = Start();
  if (curState == fst::kNoStateId) {
    cout << "Error: LexFst has no start state!" << endl;
    return;
  }

  bool newEndState = false;
  bool histFixed = false;
  for (std::vector<CharId>::const_iterator c_it = WordBegin; c_it != WordBegin + WordLength; ++c_it) {
    CharId id = *c_it;
    if (curState == fst::kNoStateId) {
      cout << "Error: LexFst has an arc pointing to a null state (id: " << id << ")!" << endl;
      exit(4);
    }

    //cout << "Pointer to lex: " << this << endl;
    fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, curState);
    for (; !aiter.Done() && (aiter.Value().ilabel != id); aiter.Next()) {
      ;
    }

    if (!aiter.Done()) {  //The current state has a transition with the character
      curState = aiter.Value().nextstate;
      if (curState == fst::kNoStateId) {
        cout << "Error: LexFst has an arc pointing to a null state (id: " << id << ")!" << endl;
        exit(4);
      }
    } else { // The current state has no transition with the charcter -> we add it
      int newState = AddState();
      //Add arcs and sort them (saves the expensive SortArc)
      if (Debug) {
        cout << "Adding new State " << newState << " (<-" << curState << " [" << id << "])" << endl;
      }
      addArcSorted(curState, fst::LogArc(id, EPS_SYMBOLID, 0, newState));
      //correct the history of current State by removing the transition to the unkState with the id  if a new state has been added
      if (!histFixed && curState != Start() && !newEndState) {
        int lastHistState = getLastHistState(curState);
        rmArcWithId(lastHistState, id);
        histFixed = true;
      }
      //Set the newState as current state and built a history for the state
      newEndState = true;
      curState = newState;
      bool last = (c_it + 1 == (WordBegin + WordLength)) ? true : false; //The history of the last character of the word must not have the word end tag
      //Build history
      int skipId = EPS_SYMBOLID;
      if (!last) {
        skipId = *(c_it + 1);
        if (Debug) {
          cout << "SkipID: " << skipId << endl;
        }
      }
      vector<CharId>::const_iterator h_c_it = WordBegin;
      if (h_c_it != c_it) {
        //History
        int newHistState = AddState();
        AddArc(curState, fst::LogArc(EPS_SYMBOLID, *h_c_it, 0, newHistState));
        ++h_c_it;
        for (; h_c_it != c_it; ++h_c_it) {
          int next = AddState();
          AddArc(newHistState, fst::LogArc(EPS_SYMBOLID, *h_c_it, 0, next));
          newHistState = next;
        }
        //Current Character
        int next = AddState();
        AddArc(newHistState, fst::LogArc(EPS_SYMBOLID, id, 0, next));
        //Transitions to unkState and to the homestate if it is not the last character of a word
        if (!last) {
          AddArc(next, fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, 0, HomeState));
        }
        for (CharId c = CharactersBegin; c <  CharactersEnd; c++)
          if (c != skipId) {
            AddArc(next, fst::LogArc(c, c, 0, UnkState));
          }
      } else {
        //Current Character
        int next = AddState();
        AddArc(curState, fst::LogArc(EPS_SYMBOLID, id, 0, next));
        //Transitions to unkState and to the homestate if it is not the last character of a word
        if (!last) {
          AddArc(next, fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, 0, HomeState));
        }
        for (CharId c = CharactersBegin; c < CharactersEnd; c++)
          if (c != skipId) {
            AddArc(next, fst::LogArc(c, c, 0, UnkState));
          }
      }
    }
  }

  //Add the wid
  if (Debug) {
    cout << "Adding wid: " << WordId << " (state: " << curState << "| newEndState: " << newEndState << ")" << endl;
  }
  addArcSorted(curState, fst::LogArc(UNKEND_SYMBOLID, WordId, 0, HomeState));

  if (!newEndState) { //If it is not a new end state we need to delete the word end tag at the end of the history
//  cout << "No new end state" << endl;
    int histState = getLastHistState(curState);
    //Delete the </w> tag
    rmArcWithId(histState, UNKEND_SYMBOLID);
  }
}


void LexFst::addArcSorted(StateId s, const fst::LogArc &arc)
{
  //Add arcs and sort them (saves the epxensiv SortArc)
  std::vector<fst::LogArc> arcs;
  for (fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, s); !aiter.Done(); aiter.Next()) {
    arcs.push_back(aiter.Value());
    if (aiter.Value().ilabel == arc.ilabel) {
      cerr << "Warning: [addArcSorted] arc with ilabel " << arc.ilabel << " already exists!" << endl;
    }
  }
  arcs.push_back(arc);
  std::sort(arcs.begin(), arcs.end(), LexFst::iLabelSort);
  DeleteArcs(s);
  for (unsigned a = 0; a < arcs.size(); a++) {
    AddArc(s, arcs.at(a));
  }
}


inline bool LexFst::iLabelSort(const fst::LogArc &i, const fst::LogArc &j)
{
  return i.ilabel < j.ilabel;
}


int LexFst::getLastHistState(LexFst::StateId s) const
{
  int histState = s;
  while (true) {
    fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, histState);
    for (; !aiter.Done() && (aiter.Value().ilabel != EPS_SYMBOLID) ; aiter.Next()) {
      ;
    }
    if (aiter.Done()) { //there is no <eps> transition so we are at the end of the history
      break;
    }
    histState = aiter.Value().nextstate;
  }
  if (histState < 2) {
    cerr << "Warning: [getLastHistState] The last history state for state " << s << " is " << histState << ". Something went wrong!" << endl;
  }
  return histState;
}


void LexFst::rmArcWithId(StateId s, CharId id)
{
  vector<fst::LogArc> arcs;
  bool deleted = false;
  for (fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, s); !aiter.Done(); aiter.Next()) {
    if (aiter.Value().ilabel != id) {
      arcs.push_back(aiter.Value());
    } else {
      if (deleted) {
        cerr << "Warning: [rmArcWithId]: The state " << s << " seems to have two transitions with id " << id << endl;
      }
      deleted = true;
    }
  }

  if (!deleted) {
    cerr << "Warning: [rmArcWithId]: The state " << s << " seems to have no transitions with id " << id << endl;
    cerr << "Available ids are: ";
    for (fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, s); !aiter.Done(); aiter.Next()) {
      cerr << aiter.Value().olabel << " ";
    }
    cerr << endl;
  }
  DeleteArcs(s);
  for (unsigned k = 0; k < arcs.size(); k++) {
    AddArc(s, arcs.at(k));
  }
}


void LexFst::rmWord(std::vector< int >::const_iterator WordBegin, int WordLength)
{
  if (Debug) {
    cout << "Removing word ";
    for (vector<CharId>::const_iterator cit = WordBegin; cit != (WordBegin + WordLength); ++cit) {
      cout << Symbols[*cit] << " ";
    }
    cout << endl;
  }

  //Remove from Transducer
  int curState = Start();
  // Determine the last state with a branch
  int lastDecideState = Start();
  CharId lastDecideId = *WordBegin;
  if (Debug) {
    cout << "Num arcs - ";
  }
  for (vector<CharId>::const_iterator c_it = WordBegin; c_it != (WordBegin + WordLength); ++c_it) {
    CharId id = *c_it;
    if (Debug) {
      cout << " [" << id << "]: " << NumArcs(curState);
    }
    if (NumArcs(curState) > 2) {
      lastDecideState = curState;
      lastDecideId = id;
    }

    fst::ArcIterator<Fst<fst::LogArc> > aiter(*this, curState);
    for (; !aiter.Done() && (aiter.Value().ilabel != id) ; aiter.Next()) {
      ;
    }
    if (!aiter.Done()) {
      curState = aiter.Value().nextstate;
    } else {
      cerr << "Warning: Should remove word ";
      for (vector<CharId>::const_iterator cit = WordBegin; cit != (WordBegin + WordLength); ++cit) {
        cerr << Symbols[*cit] << " ";
      }
      cerr << " but could not find it";
      exit(5);
    }
  }

  if (Debug) {
    cout << " [/]: " << NumArcs(curState);
  }

  if (NumArcs(curState) > 2) {
    lastDecideState = curState;
    lastDecideId = 0;
  }

  if (Debug) {
    cout << " => lastDecideState: " << lastDecideState << " ID: " << lastDecideId << endl;
    cout << "Arc labels at current state: ";
    for (fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, curState); !aiter.Done(); aiter.Next()) {
      cout << aiter.Value().ilabel << "/" << aiter.Value().olabel << " ";
    }
    cout << endl;
  }

  //Check if there is the end of a word at the current state
  bool isEndTag = false;
  for (fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, curState); !aiter.Done(); aiter.Next()) {
    if (aiter.Value().ilabel == UNKEND_SYMBOLID) {
      isEndTag = true;
    }
  }
  if (!isEndTag) {
    cerr << "Warning: Should remove a word but there is no arc with the wid leading to the start state!";
    exit(77);
  }

  //If we are in the middle of a path, we just remove the wid and add </w>
  if (lastDecideState == curState) {
    if (Debug) {
      cout << "Remove wid" << endl;
    }

    //Save all arcs without the wid and add them later
    vector<fst::LogArc> arcs;
    if (Debug) {
      cout << "Arcs total: " << NumArcs(curState) << endl;
    }

    for (fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, curState); !aiter.Done(); aiter.Next()) {
      if (aiter.Value().olabel < CharactersEnd) {
        arcs.push_back(aiter.Value());
      } else if (Debug) {
        cout << "Deleting arc with label: " << aiter.Value().olabel << endl;
      }
    }
    if (Debug) {
      cout << "Keeping " << arcs.size() << " arcs" << endl;
    }
    DeleteArcs(curState);
    for (unsigned k = 0; k < arcs.size(); k++) {
      AddArc(curState, arcs.at(k));
    }

    //find last history state and add the word end tag
    int histState = getLastHistState(curState);
    if (Debug) {
      cout << "CurState: " << curState << " -> histState: " << histState << endl;
    }
    addArcSorted(histState, fst::LogArc(UNKEND_SYMBOLID, UNKEND_SYMBOLID, 0, HomeState));
  } else {
    //cout << "Cut tree" << endl;
    //Cut the tree at lastDecideState
    vector<fst::LogArc> arcs;
    //cout << "Arcs total: " << NumArcs ( lastDecideState ) << endl;
    for (fst::ArcIterator<VectorFst<fst::LogArc> > aiter(*this, lastDecideState); !aiter.Done(); aiter.Next()) {
      if (aiter.Value().ilabel != lastDecideId) {
        arcs.push_back(aiter.Value());
      }
      // else
      // cout << "Deleting arc with label: " << aiter.Value().ilabel << endl;
    }
    //cout << "Keeping " << arcs.size() << " arcs" << endl;
    DeleteArcs(lastDecideState);
    for (unsigned k = 0; k < arcs.size(); k++) {
      AddArc(lastDecideState, arcs.at(k));
    }
    //Add the id at the end of the history (only if the decide state is not the start state which already has a phi transition
    if (lastDecideState > 1) {
      int histState = getLastHistState(lastDecideState);
      addArcSorted(histState, fst::LogArc(lastDecideId, lastDecideId, 0, UnkState));
    }
  }
}

uint64 LexFst::Properties(uint64 mask, bool) const
{
//   std::cout << std::oct << "Mask: " << mask << " Test: " << test << std::endl;
  return VectorFst::Properties(mask, false);
}
