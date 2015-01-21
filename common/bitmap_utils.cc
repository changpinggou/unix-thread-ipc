#include "gears/base/common/bitmap_utils.h"

HBITMAP BitmapUtils::ChangeBitmapColorDepth(HBITMAP source_handle, uint32 color_depth) {
  // Only support for 8, 16, 24, 32 color depth
  if ((color_depth != 8) && (color_depth != 16) && (color_depth != 24) && (color_depth != 32))
    return NULL;

  BITMAP bitmap = {0};
  BITMAPINFOHEADER bi = {0};
  if (0 == ::GetObject(source_handle, sizeof(BITMAP), &bitmap))
    return NULL;
  
  // If origin depth equal with new depth, return origin handle
  if (bitmap.bmBitsPixel == color_depth)
    return source_handle;

  bi.biSize          = sizeof(BITMAPINFOHEADER);
  bi.biWidth         = bitmap.bmWidth;
  bi.biHeight        = bitmap.bmHeight;
  bi.biPlanes        = bitmap.bmPlanes;
  bi.biBitCount      = color_depth;
  bi.biCompression   = BI_RGB;
  bi.biSizeImage     = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrImportant  = 0;
  bi.biClrUsed       = 0;

  HDC hdc = NULL;
  HPALETTE palette = (HPALETTE)::GetStockObject(DEFAULT_PALETTE);
  HPALETTE old_palette = NULL;
  if (palette) {
    hdc = ::GetDC(NULL);
    old_palette = ::SelectPalette(hdc, palette, FALSE);
    ::RealizePalette(hdc);
  }

  LPBITMAPINFOHEADER lpbi = NULL;
  HBITMAP dib_bitmap = ::CreateDIBSection(hdc, (const BITMAPINFO *)&bi, DIB_RGB_COLORS, (void**)&lpbi, NULL, 0);
  if (dib_bitmap == NULL)
    return NULL;
    
  ::GetDIBits(hdc, source_handle, 0, bitmap.bmHeight, lpbi, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
  // Restore palette
  if (old_palette) {
    ::SelectPalette(hdc, old_palette, TRUE);
    ::RealizePalette(hdc); 
    ::ReleaseDC(NULL, hdc);
  }

  return dib_bitmap;
}
