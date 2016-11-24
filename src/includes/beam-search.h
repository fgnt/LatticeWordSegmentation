//   Copyright 2009, Kyfd Project Team
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

//
// beam-trim.h
//
//  An implementation of beam-search for WFSTs
//  21.01.2015: Changed to work with LogArcs (and only LogArcs!) by Jahn Heymann

#ifndef BEAM_TRIM_H__
#define BEAM_TRIM_H__

#include <fst/fst.h>
#include <fst/mutable-fst.h>
#include <fst/arc.h>
#include <fst/weight.h>
#include <set>
#include <list>

// #define BEAM_DEBUG

namespace fst {

  class NaturalLessLog {
  public:

    bool operator()(const LogWeight &w1, const LogWeight &w2) const {
      return (min(w1.Value(), w2.Value()) == w1.Value()) && w1 != w2;
    }
  };

  template <class Arc> class HypothesisLess;

// a class that holds a single hypothesis
  template <class Arc>
  class Hypothesis {

  public:

    friend class HypothesisLess<Arc>;

    typedef typename Arc::StateId StateId;
    typedef typename Arc::Weight Weight;

    // ctor
    Hypothesis() : number_(globalId_++), state_(kNoStateId) { };

    // ctor
    Hypothesis(Arc arc, Weight weight, StateId state, bool best = true)
      : arc_(arc), weight_(weight), state_(state), number_(globalId_++), best_(best) { }

    // dtor
    ~Hypothesis() { }

    // getters
    unsigned getNumber() const {
      return number_;
    }
    StateId getState() const {
      return state_;
    }
    StateId getNextState() const {
      return arc_.nextstate;
    }
    const Weight &getWeight() const {
      return weight_;
    }
    const Arc &getArc() const {
      return arc_;
    }
    bool isBest() const {
      return best_;
    }

    // setters
    void setState(StateId stateId) {
      arc_.nextstate = stateId;
    }
    static void setGlobalId(unsigned globalId) {
      globalId_ = globalId;
    }
    void setBest(bool best) {
      best_ = best;
    }

  private:
    Arc arc_;
    Weight weight_;
    StateId state_;
    unsigned number_;
    bool best_;

    static unsigned globalId_;

  };

  template <class Arc> unsigned Hypothesis<Arc>::globalId_;

  template <class Arc>
  std::ostream &operator << (std::ostream &os, const Hypothesis<Arc> &hyp)
  {
    os << hyp.getState() << "--"
       << hyp.getArc().ilabel << "/"
       << hyp.getArc().olabel << ":"
       << hyp.getArc().weight << "-->"
       << hyp.getNextState() << "=="
       << hyp.getWeight() << " ["
       << hyp.getNumber() << "]";
    return os;
  }

// a less-than function for hypotheses
//  compares weights, and if weights are equal, breaks ties with the number ids
  template <class Arc>
  class HypothesisLess {

  public:

    typedef typename Arc::Weight Weight;

    HypothesisLess() { };

    bool operator()(const Hypothesis<Arc> &o1, const Hypothesis<Arc> &o2) {
      if (weightLess(o1.weight_, o2.weight_)) {
        return true;
      } else if (weightLess(o2.weight_, o1.weight_)) {
        return false;
      } else {
        return o1.number_ < o2.number_;
      }
    }

  private:
    NaturalLessLog weightLess;

  };

// do a beam-search type trim, aligning the number of non-epsilon input symbols
  template <class Arc>
  void BeamTrim(const Fst<Arc> &ifst, MutableFst<Arc> *ofst, unsigned beamWidth)
  {

    typedef typename std::set< Hypothesis<Arc>, HypothesisLess<Arc> > HypothesisSet;
    typedef typename Arc::StateId StateId;
    typedef typename Arc::Weight Weight;
    typedef typename std::pair< StateId, StateId > StatePair;
    typedef typename std::map< StatePair, unsigned > ArcMap;

    // set up the state mapping and other constants
    NaturalLessLog weightLess;
    std::map< StateId, StateId > stateMap;
    Hypothesis<Arc>::setGlobalId(0);
    ArcMap hasArcs;

    // get the current set
    HypothesisSet *currSet = new HypothesisSet;
    StateId startState = stateMap[ifst.Start()] = ofst->AddState();
    ofst->SetStart(startState);
    Arc startArc(kNoLabel, kNoLabel, Weight::One(), startState);
    Hypothesis<Arc> startHyp(startArc, Weight::One(), kNoStateId);
    currSet->insert(startHyp);

    // set the final set of hypotheses to be a single bad one
    HypothesisSet *finalSet = new HypothesisSet;
    Arc finalArc(kNoLabel, kNoLabel, Weight::Zero(), kNoStateId);
    Hypothesis<Arc> badHyp(finalArc, Weight::Zero(), kNoStateId);
    finalSet->insert(badHyp);

    // while there is still a current hypothesis worth expanding
    typename HypothesisSet::iterator index;
    unsigned step = 1;
    while (currSet->size() > 0 && weightLess(currSet->begin()->getWeight(), finalSet->rbegin()->getWeight())) {

#ifdef BEAM_DEBUG
      cerr << "Starting iteration " << step << ", currSet->size() == " << currSet->size() << endl;
      for (typename HypothesisSet::iterator hypIt = currSet->begin(); hypIt != currSet->end(); hypIt++) {
        cerr << " Hypothesis: " << *hypIt << std::endl;
      }
#endif

      // create the set to insert symbol transitions into
      std::set<StateId> expanded;
      HypothesisSet *nextSet = new HypothesisSet;
      nextSet->insert(badHyp);

      // loop through all hypotheses in the current set
      while (currSet->size() > 0) {

        // pop the current hypothesis and make sure it is fit to be examined
        Hypothesis<Arc> currHyp = * currSet->begin();
#ifdef BEAM_DEBUG
        cerr << " examining " << currHyp << endl;
#endif
        currSet->erase(currHyp);
#ifdef BEAM_DEBUG
        cerr << " erased " << currHyp << endl;
#endif


        const Weight &worstWeight = nextSet->rbegin()->getWeight();
        if (!weightLess(currHyp.getWeight(), worstWeight)) {
          break;
        }

        // get the info about the current hypothesis
        StateId currState = currHyp.getState();
        StateId nextState = currHyp.getNextState();
        const Weight &currWeight = currHyp.getWeight();

        // skip ones that have already been added this iteration
        StatePair arcPair(currState, nextState);
        typename ArcMap::iterator arcIt = hasArcs.find(arcPair);
        bool arcExists = (arcIt != hasArcs.end());
        if (arcExists && arcIt->second == step) {
          continue;
        }
        hasArcs[arcPair] = step;

        // if it's ok, print it, adjusting the state symbols
        if (!arcExists && currState != kNoStateId) {
          Arc ofstArc = currHyp.getArc();
          typename std::map<StateId, StateId>::iterator stateIdx = stateMap.find(currState);
          typename std::map<StateId, StateId>::iterator arcStateIdx = stateMap.find(ofstArc.nextstate);
          StateId ofstState = (stateIdx == stateMap.end() ?
                               stateMap[currState] = ofst->AddState() :
                                                     stateIdx->second);
          ofstArc.nextstate = (arcStateIdx == stateMap.end() ?
                               stateMap[nextState] = ofst->AddState() :
                                                     arcStateIdx->second);
          assert(ofstState < ofst->NumStates());
          assert(ofstArc.nextstate < ofst->NumStates());
          ofst->AddArc(ofstState, ofstArc);

#ifdef BEAM_DEBUG
          cerr << "  OFST (" << ofst->NumStates() << ") adding: " << ofstState
               <<  " -> " << ofstArc.nextstate
               <<  "(" << ofstArc.ilabel
               <<  "/" << ofstArc.olabel
               <<  ":" << ofstArc.weight
               << ")" << endl;
#endif

          // add to the final set if necessary
          const Weight &finalWeight = ifst.Final(nextState);
          if (finalWeight != Weight::Zero()) {
            ofst->SetFinal(ofstArc.nextstate, finalWeight);
            finalSet->insert(currHyp);
          }
        }

        // add new hypotheses for all the arcs in the state
        for (ArcIterator< Fst<Arc> > ait(ifst, nextState); !ait.Done(); ait.Next()) {

          const Arc &arc = ait.Value();
          Hypothesis<Arc> nextHyp(arc, Times(arc.weight, currWeight), nextState, true);//nextBest);
          // add to the appropriate stack based on whether an input symbol exists
          if (weightLess(nextHyp.getWeight(), worstWeight)) {
#ifdef BEAM_DEBUG
            cerr << "  Adding hypothesis " << nextHyp << endl;
#endif
            (arc.ilabel ? nextSet : currSet)->insert(nextHyp);

          }
        }

        // trim the next set
        while (nextSet->size() > beamWidth) {
#ifdef BEAM_DEBUG
          cerr << "  Erasing " << *nextSet->rbegin() << endl;
#endif
          nextSet->erase(*nextSet->rbegin());
        }

      }

      // delete the old hypotheses and ones over the limit
      currSet->clear();
      // set< pair<StateId,StateId> > hasStates;
      for (index = nextSet->begin(); index != nextSet->end() && currSet->size() < beamWidth; index++) {
        // pair<StateId,StateId> myPair(index->getState(), index->getNextState());
        // if(!hasStates.count(myPair)) {
        currSet->insert(*index);
        //    hasStates.insert(myPair);
        // }
      }

#ifdef BEAM_DEBUG
      cerr << " Hypothesis Count: before=" << nextSet->size() << ", after=" << currSet->size() << endl;
      for (index = finalSet->begin(); index != finalSet->end(); index++) {
        cerr << " FinalSet: " << *index << endl;
      }
#endif

      step++;

      delete nextSet;

    }

    delete finalSet;
    delete currSet;

#ifdef BEAM_DEBUG
    cerr << "Done Trimming" << endl;
#endif

  }

}

#endif // BEAM_TRIM_H__