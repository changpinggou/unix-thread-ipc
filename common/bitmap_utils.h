#ifndef GEARS_BASE_COMMON_BITMAP_UTILS_H__
#define GEARS_BASE_COMMON_BITMAP_UTILS_H__
#include <Windows.h>
#include "gears/base/common/basictypes.h"

class BitmapUtils {
 public:
  // Convert source_handle color depth to color_depth
  static HBITMAP ChangeBitmapColorDepth(HBITMAP source_handle, uint32 color_depth);
};

#endif // GEARS_BASE_COMMON_BITMAP_UTILS_H__
