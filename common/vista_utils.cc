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

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#if defined(WIN32)

#include <assert.h>
#include <tchar.h>
#include "gears/base/common/vista_utils.h"
#ifdef OS_WINCE
// Force GetProcAddress to be defined to the ASCII version in order to
// make the call compile.
#undef GetProcAddress
#define GetProcAddress GetProcAddressA
#endif


// The LABEL_SECURITY_INFORMATION SDDL SACL to be set for low integrity
LPCWSTR LOW_INTEGRITY_SDDL_SACL_W = L"S:(ML;;NW;;;LW)";

//we use platform SDK 2k3 R2, which dose not have this value defined
#ifndef LABEL_SECURITY_INFORMATION
#define LABEL_SECURITY_INFORMATION 0x00000010L
#endif //LABEL_SECURITY_INFORMATION

#if (_MSC_VER < 1500)
typedef struct _TOKEN_MANDATORY_LABEL {
  SID_AND_ATTRIBUTES Label;
} TOKEN_MANDATORY_LABEL, *PTOKEN_MANDATORY_LABEL;
#endif


bool VistaUtils::IsRunningOnVista() {
  static DWORD os_major_version = 0;
  static DWORD platform_id = 0;

  if (0 == os_major_version) {
    OSVERSIONINFO os_version = {0};
    os_version.dwOSVersionInfoSize = sizeof(os_version);
    GetVersionEx(&os_version);
    os_major_version = os_version.dwMajorVersion;
    platform_id = os_version.dwPlatformId;
  }

  return (6 == os_major_version) && (VER_PLATFORM_WIN32_NT == platform_id);
}



bool VistaUtils::IEIsProtectedModeProcess() {
  if (!IsRunningOnVista()) {
    return false;
  }

  typedef HRESULT (WINAPI *IEIsProtectedModeProcessProc)(BOOL *result);

  // We use GetModuleHandle here because this call is really only
  // valid inside IE. IF we are already in IE then ieframe.dll is
  // already loaded. If we are not running in IE we want to return
  // a failure code anyway.
  IEIsProtectedModeProcessProc function =
      reinterpret_cast<IEIsProtectedModeProcessProc>(
                              GetProcAddress(
                                  GetModuleHandle(_T("ieframe.dll")),
                                  "IEIsProtectedModeProcess"));
  if (!function) {
    return false;
  }

  BOOL is_protected_mode = FALSE;
  HRESULT hr = function(&is_protected_mode);
  if (FAILED(hr)) {
    return false;
  }

  return is_protected_mode ? true : false;
}



bool VistaUtils::GetLocalAppDataLowPath(std::string16 *fullpath) {
  if (!IsRunningOnVista()) {
    return false;
  }

  HMODULE shell32module = GetModuleHandle(_T("shell32.dll"));
  if (!shell32module) {
    // We depend on shell32.dll having already been loaded
    assert(false);
    return false;
  }

  // Defined in the platform SDK for Vista
  const DWORD KF_FLAG_CREATE = 0x00008000;
  const GUID FOLDERID_LocalAppDataLow =
    {0xA520A1A4, 0x1780, 0x4FF6,
     {0xBD, 0x18, 0x16, 0x73, 0x43, 0xC5, 0xAF, 0x16}};

  typedef HRESULT (WINAPI *SHGetKnownFolderPathProc)(REFGUID rfid,
                                                     DWORD wdFlags,
                                                     HANDLE hToken,
                                                     PWSTR *ppszPath);
  SHGetKnownFolderPathProc function =
      reinterpret_cast<SHGetKnownFolderPathProc>(
                           GetProcAddress(shell32module,
                                          "SHGetKnownFolderPath"));
  if (!function) {
    return false;
  }

  WCHAR *path = NULL;
  HRESULT hr = function(FOLDERID_LocalAppDataLow, KF_FLAG_CREATE, NULL, &path);
  if (FAILED(hr)) {
    return false;
  }

  *fullpath = path;
  CoTaskMemFree(path);
  return true;
}


//make a kernel object to low integrity level, which is accessible from protected ie
bool VistaUtils::SetObjectToLowIntegrity( HANDLE hObject,
    SE_OBJECT_TYPE type /*= SE_KERNEL_OBJECT*/) {
  //once our code is not running in vista and later, this function is a noop
  //thus we can call for any kernel object we create
  if(!IsRunningOnVista()) {
    return true;
  }

  bool bRet = false;
  DWORD dwErr = ERROR_SUCCESS;
  PSECURITY_DESCRIPTOR pSD = NULL;
  PACL pSacl = NULL;
  BOOL fSaclPresent = FALSE;
  BOOL fSaclDefaulted = FALSE;

  if ( ConvertStringSecurityDescriptorToSecurityDescriptorW (
        LOW_INTEGRITY_SDDL_SACL_W, SDDL_REVISION_1, &pSD, NULL ) ) {
    if ( GetSecurityDescriptorSacl (
          pSD, &fSaclPresent, &pSacl, &fSaclDefaulted ) ) {
      dwErr = SetSecurityInfo (
          hObject, type, LABEL_SECURITY_INFORMATION,
          NULL, NULL, NULL, pSacl );

      bRet = (ERROR_SUCCESS == dwErr);
    }

    LocalFree ( pSD );
  }
  return bRet;
}

VistaUtils::ProcessIntegrity 
VistaUtils::GetCurrentProcessIntegrityLevel() {
  if (!IsRunningOnVista())
    return PROCESS_INTEGRITY_UNKNOWN;
    
  HANDLE hToken = NULL;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE, &hToken)) {
    // Get the Integrity level.
    unsigned long need_length = 0;
    if (!GetTokenInformation(hToken, static_cast<TOKEN_INFORMATION_CLASS>(25)/*TokenIntegrityLevel*/, 
        NULL, 0, &need_length)) {
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        PTOKEN_MANDATORY_LABEL tml = NULL;
        tml = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0, need_length);
        if (tml != NULL) {
          if (GetTokenInformation(hToken, static_cast<TOKEN_INFORMATION_CLASS>(25)/*TokenIntegrityLevel*/, 
              tml, need_length, &need_length)) {
            unsigned long subauthority_count = (uint32)(byte)(*GetSidSubAuthorityCount(tml->Label.Sid) - 1);
            unsigned long integrity_level = *GetSidSubAuthority(tml->Label.Sid, subauthority_count);
            if (integrity_level < SECURITY_MANDATORY_MEDIUM_RID) {
              // Low Integrity
              return PROCESS_INTEGRITY_LOW;
            }
            else if (integrity_level >= SECURITY_MANDATORY_MEDIUM_RID && 
              integrity_level < SECURITY_MANDATORY_HIGH_RID) {
              // Medium Integrity
              return PROCESS_INTEGRITY_MEDIUM;
            }
            else if (integrity_level >= SECURITY_MANDATORY_HIGH_RID) {
              // High Integrity
              return PROCESS_INTEGRITY_HIGH;
            }
          }
          LocalFree(tml);
        }
      }
    }

    CloseHandle(hToken);
  }
  return PROCESS_INTEGRITY_UNKNOWN;
}

#endif  // WIN32
