// ----------------------------------------------------------------------------
/**
   File: FileData.cpp

   Status:         Version 1.0
   Language: C++

   License: UPB licence

   Copyright (c) <2016> <University of Paderborn>
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


   Author: Thomas Glarner

   E-Mail: glarner@nt.uni-paderborn.de

   Description: io class for lattice reading, processing and writing

   Limitations: -

   Change History:
   Date         Author       Description
   2016         Glarner      Initial
*/
// ----------------------------------------------------------------------------
#ifndef _FILEDATA_HPP_
#define _FILEDATA_HPP_

#include <fst/vector-fst.h>
#include "definitions.hpp"
#include "StringToIntMapper.hpp"

class FileData{
  // a global string to int mapper which is updated with each reading process
  StringToIntMapper GlobalStringToInt;

  // members for initialization fsts
  StringToIntMapper InitStringToInt;
  std::vector<LogVectorFst> InitFsts;
  // filename of read initialization files
  std::vector<std::string> InitFileNames;

  // members for input fsts
  StringToIntMapper InputStringToInt;
  std::vector<LogVectorFst> InputFsts;
  std::vector<std::string> InputFileNames;
  std::vector<ArcInfo> InputArcInfos; // ArcInfo members: {label, start, end}

  // members for reference fsts
  StringToIntMapper ReferenceStringToInt;
  std::vector<LogVectorFst> ReferenceFsts;
  std::vector<std::string> ReferenceFileNames;
  
  // members for special transducers
  LogVectorFst WordEndTransducer;

public:
  /* Constructor */
  FileData(
    StringToIntMapper GlobalStringToInt,
    StringToIntMapper InitStringToInt,
    std::vector<LogVectorFst> InitFsts,
    std::vector<std::string> InitFileNames,
    StringToIntMapper InputStringToInt,
    std::vector<LogVectorFst> InputFsts,
    std::vector<std::string> InputFileNames,
    std::vector<ArcInfo> InputArcInfos,
    StringToIntMapper ReferenceStringToInt,
    std::vector<LogVectorFst> ReferenceFsts,
    std::vector<std::string> ReferenceFileNames,
    LogVectorFst WordEndTransducer
  );


  /* Copy Constructor */
  FileData(const FileData& lhs);


  /* interface */
  // get vector mapping integer to string (characters)
  const std::vector<std::string> &GetInputIntToStringVector() const;

  // integer to string mapping for the reference transcription (characters)
  const std::vector<std::string> &GetReferenceIntToStringVector() const;

  // integer to string mapping for the init transcription (characters)
  const std::vector<std::string> &GetInitIntToStringVector() const;

  const std::vector<LogVectorFst> &GetInputFsts() const;

  const std::vector<LogVectorFst> &GetReferenceFsts() const;

  const std::vector<LogVectorFst> &GetInitFsts() const;

  const std::vector<std::string> &GetInputFileNames() const;

  const std::vector<std::string> &GetReferenceFileNames() const;

  const std::vector<std::string> &GetInitFileNames() const;

  const std::vector<ArcInfo> &GetInputArcInfos() const;
  
  const LogVectorFst &GetWordEndTransducer() const;
};

#endif // _FILEDATA_HPP_
