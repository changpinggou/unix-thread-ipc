#include <assert.h>
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/thread_locals.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// Long Path Prefix - '\\?\' used for handling paths longer than MAX_PATH.
static const std::string16 kLongPathPrefix(STRING16(L"\\\\?\\"));

// Long Path Prefix -  '\\?\UNC\' used for network shares.
static const std::string16 kLongPathPrefixUNC(STRING16(L"\\\\?\\UNC\\"));

// '\\' Prefix for network shares.
static const std::string16 kSharePrefix(STRING16(L"\\\\"));

// Joins 2 paths together.
inline static std::string16 JoinPath(const std::string16 &path1, 
                              const std::string16 &path2) {
  static std::string16 path_sep(&kPathSeparator, 1);
  return path1 + path_sep + path2;
}

template<typename CallBack>
bool RecursiveForEachImpl(const std::string16 &iterate_path, 
    CallBack callback, bool go_on_with_error){
  assert(StartsWith(iterate_path, kLongPathPrefix));

  // First recursively delete directory contents
  std::string16 find_spec = iterate_path + L"\\*";
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = ::FindFirstFileW(find_spec.c_str(), 
      &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  do {
    if ((wcscmp(find_data.cFileName, L"..") == 0) ||
        (wcscmp(find_data.cFileName, L".") == 0)) {
      continue;  // nothing to do for parent or current directory
    }

    bool item_is_dir = (find_data.dwFileAttributes & 
        FILE_ATTRIBUTE_DIRECTORY) != 0;

    std::string16 child = JoinPath(iterate_path, find_data.cFileName);
    if (item_is_dir) {
      if (!RecursiveForEachImpl(child, callback, go_on_with_error) 
          && !go_on_with_error) {
        return false;
      }
    } else {
      if (!callback(child.c_str(), false) && !go_on_with_error) {
        return false;
      }
    }

  } while (::FindNextFile(find_handle, &find_data) != 0);
  if (::FindClose(find_handle) == 0) {
    return false;
  }
  if (!callback(iterate_path.c_str(), true) && !go_on_with_error) {
    return false;
  }
  return true;
}

template<typename CallBack>
bool File::RecursiveForEach(const char16 *full_path,
    CallBack callback, bool go_on_with_error) {
  std::string16 dir_to_iterate(full_path);
  dir_to_iterate = ToLongPath(dir_to_iterate);

  // Cut off trailing slashes.
  size_t path_length = dir_to_iterate.length();
  while (path_length > 0 && dir_to_iterate[path_length - 1] == kPathSeparator) {
    dir_to_iterate.erase(path_length - 1);
    path_length -= 1;
  }

  DWORD attributes = ::GetFileAttributesW(dir_to_iterate.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  bool is_dir = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  if (!is_dir) {  // We can only operate on a directory.
    return false;
  }

  return RecursiveForEachImpl(dir_to_iterate, callback, go_on_with_error);
}
