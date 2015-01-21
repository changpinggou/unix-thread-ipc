// Copyright 2008, Google Inc.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GEARS_BASE_COMMON_BASE64_H__
#define GEARS_BASE_COMMON_BASE64_H__

#include <vector>

#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"

// Both of these methods use the standard form of base64. 

// Encodes the input string in base64.  Returns true if successful and false
// otherwise.  The output string is only modified if successful.
bool Base64Encode(const std::vector<uint8>& input, std::string* output);

//string version for giant uploader
bool Base64Encode(const std::string& input, std::string* output);


// Decodes the base64 input string.  Returns true if successful and false
// otherwise.  The output string is only modified if successful.
bool Base64Decode(const std::string& input, std::vector<uint8>* output);

//string16 version for uploader::resume
bool Base64Decode(const std::string16& input, std::string* output);


#endif  // GEARS_BASE_COMMON_BASE64_H__
