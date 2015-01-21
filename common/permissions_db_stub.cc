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

#include "gears/base/common/permissions_db.h"
#include "gears/base/common/message_service.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/thread_locals.h"
#include "gears/database2/database2_metadata.h"
#include "gears/localserver/common/localserver_db.h"

static const char16 *kDatabaseName = STRING16(L"permissions.db");
// TODO(aa): Rename this table "LocalDataAccess"
static const char16 *kLocalDataAccessTableName = STRING16(L"Access");
static const char16 *kLocationDataAccessTableName = STRING16(L"LocationAccess");
static const char16 *kSupressDialogsKeyName = STRING16(L"SupressDialogs");
// TODO(aa): Rename this table "Settings"
static const char16 *kVersionTableName = STRING16(L"VersionInfo");
static const char16 *kVersionKeyName = STRING16(L"Version");
static const int kCurrentVersion = 9;
static const int kOldestUpgradeableVersion = 1;

const char16 *PermissionsDB::kShortcutsChangedTopic = 
                  STRING16(L"base:permissions:shortcuts-changed");

const ThreadLocals::Slot kThreadLocalKey = 0;

PermissionsDB *PermissionsDB::GetDB() {
  return NULL;
}


void PermissionsDB::DestroyDB(void *context) {
}


PermissionsDB::PermissionsDB()
    : settings_table_(&db_, kVersionTableName),
      local_data_access_table_(&db_, kLocalDataAccessTableName),
      location_access_table_(&db_, kLocationDataAccessTableName),
      shortcut_table_(&db_),
      database_name_table_(&db_){
}


bool PermissionsDB::Init() {


  return false;
}


bool PermissionsDB::ShouldSupressDialogs() {
  return false;
}


PermissionsDB::PermissionValue PermissionsDB::GetPermission(
    const SecurityOrigin &origin,
    PermissionType type) {
  return PERMISSION_ALLOWED;
}


void PermissionsDB::SetPermission(const SecurityOrigin &origin,
                                  PermissionType type,
                                  PermissionsDB::PermissionValue value) {
}


bool PermissionsDB::GetOriginsByValue(PermissionsDB::PermissionValue value,
                                      PermissionType type,
                                      std::vector<SecurityOrigin> *result) {
  return false;
}

bool PermissionsDB::GetPermissionsSorted(PermissionsList *permission_list_out) {
  return false;
}


bool PermissionsDB::EnableGearsForWorker(const SecurityOrigin &worker_origin,
                                         const SecurityOrigin &host_origin) {
  return false;
}

bool PermissionsDB::TryAllow(const SecurityOrigin &origin,
                             PermissionType type) {
  return false;
}

bool PermissionsDB::SetShortcut(const SecurityOrigin &origin,
                                const char16 *name, const char16 *app_url,
                                const char16 *icon16x16_url,
                                const char16 *icon32x32_url,
                                const char16 *icon48x48_url,
                                const char16 *icon128x128_url,
                                const char16 *msg,
                                const bool allow) {
  return false;
}

bool PermissionsDB::GetOriginsWithShortcuts(
    std::vector<SecurityOrigin> *result) {
  return true;
}

bool PermissionsDB::GetOriginShortcuts(const SecurityOrigin &origin,
                                       std::vector<std::string16> *names) {
  return false;
}

bool PermissionsDB::GetShortcut(const SecurityOrigin &origin,
                                const char16 *name, std::string16 *app_url,
                                std::string16 *icon16x16_url,
                                std::string16 *icon32x32_url,
                                std::string16 *icon48x48_url,
                                std::string16 *icon128x128_url,
                                std::string16 *msg,
                                bool *allow) {
  return false;
}

bool PermissionsDB::DeleteShortcut(const SecurityOrigin &origin,
                                   const char16 *name) {
  return false;
}

bool PermissionsDB::DeleteShortcuts(const SecurityOrigin &origin) {
  return false;
}

// TODO(shess): Should these just be inline in the .h?
bool PermissionsDB::GetDatabaseBasename(const SecurityOrigin &origin,
                                        const char16 *database_name,
                                        std::string16 *basename) {
  return false;
}

bool PermissionsDB::MarkDatabaseCorrupt(const SecurityOrigin &origin,
                                        const char16 *database_name,
                                        const char16 *basename) {
  return false;
}

bool PermissionsDB::CreateDatabase() {
  return false;
}

bool PermissionsDB::UpgradeDatabase(int from_version) {

  return true;
}

bool PermissionsDB::UpgradeToVersion2() {
  return true;
}

bool PermissionsDB::UpgradeToVersion3() {
  return false;
}

bool PermissionsDB::UpgradeToVersion4() {
  return false;
}

bool PermissionsDB::UpgradeToVersion5() {
  return false;
}

bool PermissionsDB::UpgradeToVersion6() {
  return false;
}

bool PermissionsDB::UpgradeToVersion7() {
  return false;
}

bool PermissionsDB::UpgradeToVersion8() {
  return false;
}

bool PermissionsDB::UpgradeToVersion9() {
  return false;
}

NameValueTable* PermissionsDB::GetTableForPermissionType(PermissionType type) {
  return NULL;
}
