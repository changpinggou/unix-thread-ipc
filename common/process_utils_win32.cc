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

#ifdef WIN32

#include <windows.h>
#include <atlbase.h>
#include "gears/base/common/string16.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

HMODULE GetGearsModuleHandle() {
  MEMORY_BASIC_INFORMATION mbi;
  void *address = GetGearsModuleHandle;
  SIZE_T result = VirtualQuery(address, &mbi, sizeof(mbi));
  return static_cast<HMODULE>(mbi.AllocationBase);
}

void RegSvr(const std::string16 &path, const std::string &method) {
  LRESULT (CALLBACK *lpDllEntryPoint)();
  HINSTANCE hLib = LoadLibrary(path.c_str());
  if (hLib < (HINSTANCE)HINSTANCE_ERROR)
    return;
  (FARPROC&)lpDllEntryPoint = GetProcAddress(hLib, method.c_str());
  if(lpDllEntryPoint)
    (*lpDllEntryPoint)();
  else
    DWORD err = ::GetLastError();
}

static bool CreateStringRegKey(const char16 *path, const char16 *name, const char16 *value) {
  ATL::CRegKey rk;
  long lResult = rk.Create(HKEY_LOCAL_MACHINE, path);
  if( lResult == ERROR_SUCCESS ) {
    rk.SetStringValue(name, value);
    return true;
  } else {
    return false;
  }

}

static bool RemoveRegValueAt(const char16 *path, const char16 *name) {
  ATL::CRegKey rk;
  long lResult = rk.Open( HKEY_LOCAL_MACHINE, path);
  if ( lResult == ERROR_SUCCESS) {
    rk.DeleteValue( name );
    return true;
  } else {
    return false;
  }
}

static bool GetStringRegKey(const char16 *path, const char16 *name, std::string16 *retval) {
  CRegKey rk;
  long lResult = rk.Open(HKEY_LOCAL_MACHINE, path, KEY_READ);
  if ( lResult != ERROR_SUCCESS) {
    return false;
  }
  ULONG nchar = 0;
  char16* buffer = NULL;
  scoped_array<char16> buffer_store;
  if(retval != NULL) {
    nchar = MAX_PATH * 2;
    buffer_store.reset(new char16[nchar + 1]);
    buffer = buffer_store.get();
  }

  DWORD rs = rk.QueryStringValue(name, buffer, &nchar);
  while (retval != NULL && ERROR_MORE_DATA == rs) {
    nchar += MAX_PATH;
    buffer_store.reset(new char16[nchar + 1]);
    buffer = buffer_store.get();
    rs = rk.QueryStringValue(name, buffer, &nchar);
  }

  if(rs == ERROR_SUCCESS) {
    if(retval) {
      *retval = buffer;
    }
    return true;
  }
  if(retval) {
    *retval = L"";
  }
  return false;
}

bool ScheduleRunOnce(const char16 *name, const char16 *value) {
  return CreateStringRegKey(L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                         name,
                         value);
}

//remove an scheduled run once
bool RemoveScheduledRunOnce(const char16 *name) {
  return RemoveRegValueAt(L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", name);
}


bool ScheduleRun(const char16 *name, const char16 *value) {
  return CreateStringRegKey(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                         name,
                         value);
}

//remove an scheduled run
bool RemoveScheduledRun(const char16 *name) {
  return RemoveRegValueAt(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", name);
}

bool GetScheduledRunState(const char16 *name, std::string16 *retval) {
  return GetStringRegKey(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", name, retval);
}

bool GetScheduledRunOnceState(const char16 *name, std::string16 *retval) {
  return GetStringRegKey(L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", name, retval);
}



//start a process and wait for it's return value
bool ShellExecuteSync(const char16* elevator, const char16* module_path, const char16* parameter, const char16* working_path, DWORD *retval) {
  SHELLEXECUTEINFO sei = {
    sizeof(SHELLEXECUTEINFO), SEE_MASK_NOCLOSEPROCESS, NULL,
    elevator, module_path, parameter, working_path,
    SW_SHOWNORMAL, NULL, NULL,
    NULL, 0, 0, NULL
  };
  DWORD result;
  if(!ShellExecuteEx(&sei) || NULL == sei.hProcess) {
    return false;
  } else {
    result = WaitForSingleObject(sei.hProcess, INFINITE);
    if(WAIT_OBJECT_0 != result) return false;
    if(!GetExitCodeProcess(sei.hProcess, &result)) {
      return false;
    }
  }
  if(retval) {
    *retval = result;
  }
  return true;
}

// start a process with high integrity
bool ShellExcuteRunas(const char16* module_path, const char16* parameter, const char16* working_path) {
  SHELLEXECUTEINFO sei = {
    sizeof(SHELLEXECUTEINFO), 0, NULL,
    L"runas", module_path, parameter, working_path,
    SW_SHOWNORMAL, NULL, NULL,
    NULL, 0, 0, NULL
  };
  
  if (!ShellExecuteEx(&sei))
    return false;
    
  return true;
}


#endif  // WIN32
