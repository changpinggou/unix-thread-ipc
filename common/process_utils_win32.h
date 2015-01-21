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

#ifndef GEARS_BASE_COMMON_PROCESS_UTILS_WIN32_H__
#define GEARS_BASE_COMMON_PROCESS_UTILS_WIN32_H__

#ifdef WIN32

#include <windows.h>
#include "gears/base/common/string16.h"

// Gets a reference to the Windows module containing the Gears code.
HMODULE GetGearsModuleHandle();

void RegSvr(const std::string16 &path, const std::string &method);

//schedule an one time excuetion after reboot
bool ScheduleRunOnce(const char16 *name, const char16 *value);

//remove an scheduled run once
bool RemoveScheduledRunOnce(const char16 *name);

//schedule a Run on localmachine level
bool ScheduleRun(const char16 *name, const char16 *value);

//remove an scheduled run
bool RemoveScheduledRun(const char16 *name);

//start a process and wait for it's return value
bool ShellExecuteSync(const char16* elevator, const char16* module_path, const char16* parameter, const char16* working_path, DWORD *retval);

// start a process with high integrity
bool ShellExcuteRunas(const char16* module_path, const char16* parameter, const char16* working_path);

bool GetScheduledRunState(const char16 *name, std::string16 *retval);

bool GetScheduledRunOnceState(const char16 *name, std::string16 *retval);

template<typename Proto_>
Proto_ GetWin32Proc(LPCSTR name, LPCTSTR module) {
  return reinterpret_cast<Proto_>(::GetProcAddress(::GetModuleHandle(module), name));
}

#endif  // WIN32
#endif  // GEARS_BASE_COMMON_PROCESS_UTILS_WIN32_H__
