#ifndef GEARS_BASE_COMMON_READ_DLL_VERSION_H__
#define GEARS_BASE_COMMON_READ_DLL_VERSION_H__

#include "gears/base/common/string16.h"

class ReadDllVersion {
public:
  static std::string16 GetDllBuildID();
};

#endif  // GEARS_BASE_COMMON_READ_DLL_VERSION_H__