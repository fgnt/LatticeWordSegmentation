// ----------------------------------------------------------------------------
/**
   File: WeightedMapper.hpp

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

   Description: functions used for sampling a segmentation from an input lattice

   Limitations: -

   Change History:
   Date         Author       Description
   2014         Walter       Initial


   Note: License for class WeightedMapper:
   Copyright 2010, Graham Neubig

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   Description: mapper used for acoustic model scaling

   Limitations: -

   Change History:
   Date         Author       Description
   2010         Neubig       Initial
   2013         Heymann      Changed to work with LogArcs (and only LogArcs!)
   2016         Walter       Add more mappers
*/
// ----------------------------------------------------------------------------
#ifndef _WEIGHTED_MAPPER_H__
#define _WEIGHTED_MAPPER_H__

#include <fst/map.h>
#include "definitions.hpp"

namespace fst {
  // Copyright 2010, Graham Neubig, modified by Jahn Heymann (2013) and Oliver Walter (2014) //
  // a map putting a log-linear weight on the arc value
  struct WeightedMapper {

    float weight_;

    WeightedMapper(float weight) : weight_(weight) { }

    LogArc operator()(const LogArc &arc) const {
      LogWeight ret(arc.weight.Value() * (arc.weight.Value() == FloatLimits<float>::PosInfinity() ? 1 : weight_));
      return LogArc(arc.ilabel, arc.olabel, ret, arc.nextstate);
    }

    MapFinalAction FinalAction() const {
      return MAP_NO_SUPERFINAL;
    }

    MapSymbolsAction InputSymbolsAction() const {
      return MAP_COPY_SYMBOLS;
    }

    MapSymbolsAction OutputSymbolsAction() const {
      return MAP_COPY_SYMBOLS;
    }

    uint64 Properties(uint64 props) const {
      return props;
    }

  };

  // a map copying the output label to the input label
  struct CopyOlabelMapper {

    int eps_label_;

    CopyOlabelMapper(int unk_label) : eps_label_(unk_label) { }

    LogArc operator()(const LogArc &arc) const {
      int ret(arc.olabel <= eps_label_ ? 0 : arc.olabel);
      return LogArc(ret, arc.olabel, arc.weight, arc.nextstate);
    }

    MapFinalAction FinalAction() const {
      return MAP_NO_SUPERFINAL;
    }

    MapSymbolsAction InputSymbolsAction() const {
      return MAP_COPY_SYMBOLS;
    }

    MapSymbolsAction OutputSymbolsAction() const {
      return MAP_COPY_SYMBOLS;
    }

    uint64 Properties(uint64 props) const {
      return props;
    }

  };

  // a map reconvering the input label from the ilabel id
  struct RestoreIlabelMapper {

    const std::vector<ArcInfo> &InputArcInfos_;

    RestoreIlabelMapper(const std::vector<ArcInfo> &InputArcInfos) : InputArcInfos_(InputArcInfos) { }

    LogArc operator()(const LogArc &arc) const {
      int ret(InputArcInfos_.empty() ? arc.ilabel : InputArcInfos_[arc.ilabel].label);
      return LogArc(ret, arc.olabel, arc.weight, arc.nextstate);
    }

    MapFinalAction FinalAction() const {
      return MAP_NO_SUPERFINAL;
    }

    MapSymbolsAction InputSymbolsAction() const {
      return MAP_COPY_SYMBOLS;
    }

    MapSymbolsAction OutputSymbolsAction() const {
      return MAP_COPY_SYMBOLS;
    }

    uint64 Properties(uint64 props) const {
      return props;
    }

  };
}

#endif
