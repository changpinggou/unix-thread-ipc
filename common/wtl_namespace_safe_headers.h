/*
 * =====================================================================================
 *
 *       Filename:  wtl_namespace_safe_headers.h
 *
 *    Description:  WTL namespace was confilct with ATL and other lib, always include this
 *                  header to use WTL are safe. Any WTL class must add WTL:: prefix.
 *
 *        Version:  1.0
 *        Created:  2010/6/29
 *       Revision:  none
 *       Compiler:  gcc/msvc
 *
 *         Author:  huchao
 *        Company:  Netease
 *
 * =====================================================================================
 */
#ifndef WTL_NAMESPACE_SAFE_HEADERS_H
#define WTL_NAMESPACE_SAFE_HEADERS_H

// define to not using min/max macro in minmax.h
#ifndef _INC_MINMAX
#define _INC_MINMAX
#endif

// define to not using min/max macro in WinDef.h
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm> // provide min/max algorithm and std namespace
using std::max;
using std::min;

// Not automatic use WTL namespace
#ifndef _WTL_NO_AUTOMATIC_NAMESPACE
#define _WTL_NO_AUTOMATIC_NAMESPACE
#endif

#include <atlbase.h>
#include <atlapp.h>
#include <atlframe.h>
#include <atlmisc.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atlddx.h>
#include <atldlgs.h>
#include <atlgdi.h>
#include <atlprint.h>
#include <atlres.h>
#include <atlscrl.h>
#include <atlsplit.h>
#include <atluser.h>
#include <atlwinx.h>
#include <atlfind.h>

#if _WIN32_WINNT >= 0x0501
#include <atltheme.h> // this want _WIN32_WINNT >= 0x0501
#endif

// use for windows ce only
#ifdef OS_WINCE
#include <atlwince.h>
#include <atlresce.h>
#endif

#endif  // WTL_NAMESPACE_SAFE_HEADERS_H
