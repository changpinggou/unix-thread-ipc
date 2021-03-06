// Copyright 2007, Google Inc.
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

#include "gears/base/common/detect_version_collision.h"

#include <assert.h>
#include <atlbase.h>
#include <atlsync.h>
#include <windows.h>
#include "gears/base/common/string_utils.h"
#include "gears/base/common/process_utils_win32.h"
#include "gears/base/common/vista_utils.h"
#include "gears/ui/ie/string_table.h"
#include "genfiles/product_constants.h"

// Gets the string for the title of the dialog. Returns NULL if an error
// occured getting the string.
static const char16 *GetVersionCollisionErrorTitle();

static const int kMaxStringLength = 256;

// We run our detection code once at startup
static bool OneTimeDetectVersionCollision();
static bool detected_collision = OneTimeDetectVersionCollision();

bool DetectedVersionCollision() {
  return detected_collision;
}

// We use two named mutex objects to determine if another version of the same
// Gears distribution is running.  The first indicates an instance of ANY
// version is running.  The second indicates an instance of OUR version is
// running.  The base mutex names should not change across versions!
//
// TODO(michaeln): Ideally, these should be per user kernel objects rather
// than per local session to detect when the same user is running Gears in
// another windows session. See //depot/googleclient/ci/common/synchronized.cpp
// for some clues.
static CMutex running_mutex;
static CMutex our_version_running_mutex;

static const wchar_t *kMutexName =
    L"IsRunning" PRODUCT_COLLISION_GUID;
static const wchar_t *kOurVersionMutexName =
    L"IsRunning" PRODUCT_COLLISION_GUID L"-" PRODUCT_VERSION_STRING;

// Returns true if we detect that a different version of Gears is running.
// If no collision is detected, leaves mutex handles open to indicate that
// our version is running.  If a collision is detected, this instance of Gears
// will be crippled, so we close all mutex handles so others don't see this
// instance as 'running'. Should only be called once.
static bool OneTimeDetectVersionCollision() {
  std::wstring running_name(kMutexName);
#if BROWSER_NPAPI
  // Append the full path of the running executable.  Developers may run
  // multiple NPAPI browsers, and/or multiple versions of a single browser.
  // When Gears lives in a subdirectory of the browser, multiple versions do not
  // collide unless they are in the same browser directory.
  // Note: GetModuleFileName() doesn't normalize path (see MSDN).
#ifdef OS_WINCE
  // _(w)pgmptr is not available on WinCE. For now, we simply append use the
  // name of the browser, as it's very unlikely that any app other than the
  // intended browser will load Gears on WinCE.
  // TODO(steveblock): See if there's some way to get the exe path on WinCE.
#ifdef BROWSER_OPERA
  std::wstring browser_path(L"opera");
#endif
#else  // OS_WINCE
  assert(_pgmptr && strlen(_pgmptr) > 0);  // may need _wpgmptr if flags change
  std::wstring browser_path;
  UTF8ToString16(_pgmptr, &browser_path);
#endif  // OS_WINCE
  // Replace bad mutex name chars.
  EnsureStringValidPathComponent(browser_path, true);

  running_name += L"-";
  running_name += browser_path;
#endif  // BROWSER_NPAPI
  // Detect if an instance of any version is running
  if (!running_mutex.Create(NULL, FALSE, running_name.c_str())) {
    DWORD hresult = ::GetLastError();
    assert(false);
    return true;
  }
  bool already_running = (GetLastError() == ERROR_ALREADY_EXISTS);

  // Detect if another instance of our version is running
  std::wstring our_version_running_name(kOurVersionMutexName);
#if BROWSER_NPAPI
  our_version_running_name += L"-";
  our_version_running_name += browser_path;
#endif
  if (!our_version_running_mutex.Create(NULL, FALSE,
                                        our_version_running_name.c_str())) {
    assert(false);
    running_mutex.Close();
    return true;
  }
  bool our_version_already_running = (GetLastError() == ERROR_ALREADY_EXISTS);

  if (!already_running) {
    // No collision, we are the first instance to run
    assert(!our_version_already_running);
    VistaUtils::SetObjectToLowIntegrity(running_mutex);
    VistaUtils::SetObjectToLowIntegrity(our_version_running_mutex);
    return false;
  } else if (our_version_already_running) {
    // No collision, other instances of our version are running
    return false;
  } else {
    // A collision with a different version!
    our_version_running_mutex.Close();
    running_mutex.Close();
    return true;
  }
}

// Low tech UI to notify the user
static bool alerted_user = false;

void MaybeNotifyUserOfVersionCollision() {
  assert(detected_collision);
  if (!alerted_user) {
    NotifyUserOfVersionCollision();
  }
}

void NotifyUserOfVersionCollision() {
  assert(detected_collision);
  alerted_user = true;
  const char16 *title = GetVersionCollisionErrorTitle();
  const char16 *text = GetVersionCollisionErrorString();
  if (title && text) {
    MessageBox(NULL, text, title, MB_OK);
  }
}

const char16 *GetVersionCollisionErrorString() {
#ifdef OS_WINCE
  // On WinCE, LoadString does not support multiple languages.
  // TODO(steveblock): Internationalize string.
  static const char16 *error_text = STRING16(L"A " PRODUCT_FRIENDLY_NAME
                                    L" update has been downloaded.\n"
                                    L"\n"
                                    L"Please restart your browser"
                                    L" to complete the upgrade process.\n");
  return error_text;
#else
  // TODO(playmobil): This code isn't threadsafe, refactor.
  static char16 error_text[kMaxStringLength];

  if (!LoadString(GetGearsModuleHandle(), IDS_VERSION_COLLISION_TEXT,
                  error_text, kMaxStringLength)) {
    return NULL;
  }
  return error_text;
#endif
}

static const char16 *GetVersionCollisionErrorTitle() {
#ifdef OS_WINCE
  // On WinCE, LoadString does not support multiple languages.
  // TODO(steveblock): Internationalize string.
  static const char16 *title = STRING16(L"Please restart your browser");
  return title;
#else
  static char16 title[kMaxStringLength];
  if (!LoadString(GetGearsModuleHandle(), IDS_VERSION_COLLISION_TITLE,
                  title, kMaxStringLength)) {
    return NULL;
  }
  return title;
#endif
}
