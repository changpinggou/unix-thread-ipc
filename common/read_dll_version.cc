#include "gears/base/common/read_dll_version.h"
#include "gears/base/common/paths.h"
#include <boost/scoped_array.hpp>


#ifdef WIN32
std::string16 ReadDllVersion::GetDllBuildID() {
  std::string16 path;
  if (!GetCurrentModuleFilename(&path)) {
    return std::string16(L"0");
  }

  DWORD version_size = 0;
  version_size = ::GetFileVersionInfoSize(path.c_str(), NULL);
  if (version_size == 0) {
    return std::string16(L"0");
  }
  boost::scoped_array<BYTE> version_info(new BYTE[version_size]);
  if (::GetFileVersionInfo(path.c_str(), 0, version_size, version_info.get()) == 0) {
    return std::string16(L"0");
  }
  WORD* lang_ptr;
  unsigned int lang_length;
  if (::VerQueryValue(version_info.get(), L"\\VarFileInfo\\Translation",
          (LPVOID*)&lang_ptr, &lang_length) == 0) {
    return std::string16(L"0");
  }
  TCHAR info_path[100];
  swprintf_s(info_path, 100, L"\\StringFileInfo\\%04x%04x\\BuildID", lang_ptr[0], lang_ptr[1]);
  TCHAR* buildid;
  unsigned int buildid_length;
  if (::VerQueryValue(version_info.get(), info_path, (LPVOID*)&buildid, &buildid_length) == 0) {
    return std::string16(L"0");
  }

  return std::string16(buildid);
}

#else
std::string16 ReadDllVersion::GetDllBuildID() {
  // NOT implemented
  assert(false);
}
#endif

