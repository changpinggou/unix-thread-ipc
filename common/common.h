// Copyright 2006, Google Inc.
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
//
// Common definitions for shared code between the Firefox and IE implementations
// of Gears.

#ifndef GEARS_BASE_COMMON_COMMON_H__
#define GEARS_BASE_COMMON_COMMON_H__

#include <float.h>

#include "basictypes.h"
#include "string16.h"

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define STRINGIZE_HELPER(a) #a
#define STRINGIZE(a) STRINGIZE_HELPER(a)
#define CAT_CON__(a,b) a ## L":" ##b
#define CAT_CON(a,b) CAT_CON__(a,b)
#define __WFILE__ WIDEN(__FILE__)
#define __CLINE__ STRINGIZE(__LINE__)
#define __WLINE__ WIDEN(__CLINE__)
#define __WLOCATION__ CAT_CON(__WFILE__, __WLINE__)

#define GET_INTERNAL_ERROR_MESSAGE() \
    (std::string16(STRING16(L"Internal Error: " __WFILE__ L" Line ")) + \
     IntegerToString16(__LINE__))

// JS represents all numbers as doubles (IEEE 754). This
// effectively gives the [-2^53, 2^53] range of integers that
// can be represented by IEEE 754 without loss of precision.
// For this reason, we cap the range of supported integers
// to [-2^53, 2^53] by throwing an exception if the values
// are outside this interval. This restriction is enforced,
// for example, in GearsDatabase::GetLastInsertRowId() and
// GearsResultSet::Field().
#define JS_INT_MAX (GG_LONGLONG(1) << DBL_MANT_DIG)  // 2^53
#define JS_INT_MIN (-JS_INT_MAX)  // -2^53


#ifdef WIN32
#include "gears/base/common/common_win32.h"
#elif defined(OS_MACOSX)
#include "gears/base/common/common_osx.h"
#endif

#if BROWSER_IE || BROWSER_IEMOBILE
#include "gears/base/common/common_ie.h"
#elif BROWSER_FF
#include "gears/base/common/common_ff.h"
#elif BROWSER_WEBKIT
#include "gears/base/common/common_sf.h"
#elif BROWSER_NPAPI
#include "gears/base/common/common_np.h"
#elif BROWSER_NONE

#ifdef WIN32
#include "gears/base/common/common_ie.h"
#endif

// TODO(jianli): implement a simple logging for other platforms.
// In long term, we will use the logging code from brettw.
#if !defined(LOG)
#define LOG(args) do { } while (false)
#endif

#endif

//type-erasure object handle, which can contain anything in it safely
//it act as an auto_ptr, but provide only operator void*()
class ObjectHolder {
 public:
  ~ObjectHolder() {
    deallocator_(object_);
  }

  template<typename Ty_>
  static void Deallocate(void *target) {
    Ty_ *tmp = reinterpret_cast<Ty_*>(target);
    delete tmp;
  }

  template<typename Ty_>
  static ObjectHolder*  Hold(Ty_ *target) {
    return new ObjectHolder(reinterpret_cast<void*>(target), &Deallocate<Ty_>);
  }

  operator void*() {
    return object_;
  }

 private:
  ObjectHolder(void *target, void(*deallocator)(void*))
  : object_(target), deallocator_(deallocator) {
  }
  void *object_;
  void (*deallocator_)(void*);
  DISALLOW_EVIL_CONSTRUCTORS(ObjectHolder);
};



#endif // GEARS_BASE_COMMON_COMMON_H__
