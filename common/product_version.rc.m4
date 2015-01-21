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

/////////////////////////////////////////////////////////////////////////////
//
// Version
//

m4_changequote(`[',`]')m4_dnl
m4_changequote([~`],[`~])m4_dnl
m4_define(~`m4_underscore`~, ~`m4_translit(~`$1`~,~`-`~,~`_`~)`~)m4_dnl
m4_divert(~`-1`~)
# foreach(x, (item_1, item_2, ..., item_n), stmt)
#   parenthesized list, simple version
m4_define(~`m4_foreach`~,
~`m4_pushdef(~`$1`~)_foreach($@)m4_popdef(~`$1`~)`~)
m4_define(~`_arg1`~, ~`$1`~)
m4_define(~`_foreach`~, ~`m4_ifelse(~`$2`~, ~`()`~, ~``~,
  ~`m4_define(~`$1`~, _arg1$2)$3~``~$0(~`$1`~, (m4_shift$2), ~`$3`~)`~)`~)
m4_divert~``~m4_dnl

VS_VERSION_INFO VERSIONINFO
 FILEVERSION    PRODUCT_VERSION_MAJOR,PRODUCT_VERSION_MINOR,PRODUCT_VERSION_BUILD,PRODUCT_VERSION_PATCH
 PRODUCTVERSION PRODUCT_VERSION_MAJOR,PRODUCT_VERSION_MINOR,PRODUCT_VERSION_BUILD,PRODUCT_VERSION_PATCH
 FILEFLAGSMASK 0x3fL
#ifdef DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x4L
 FILETYPE 0x2L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904e4"
        BEGIN
            VALUE "CompanyName", L"NetEase.com,Inc"
            VALUE "FileVersion", "PRODUCT_VERSION"
            VALUE "LegalCopyright", L"\x7f51\x6613\x516c\x53f8\x7248\x6743\x6240\x6709\x00a9 1997-2010"
            VALUE "ProductName", L"\x7F51\x6613\x90AE\x4EF6\x52A9\x624B PRODUCT_VERSION"
            VALUE "ProductVersion", "PRODUCT_VERSION"
            VALUE "FileDescription", L"\x7F51\x6613\x90AE\x4EF6\x52A9\x624B"

            VALUE "InternalName", "SPECIFIED_TARGET_FILENAME"
            VALUE "OriginalFilename", "SPECIFIED_TARGET_FILENAME"

m4_ifelse(SPECIFIED_TARGET_FILENAME,~`mailassist.dll`~,m4_dnl
~`
            VALUE "BuildID", "BUILD_BASE_SHA1"
`~,~`
`~)

#if BROWSER_NPAPI
            VALUE "MIMEType", "application/x-mailassist"
#endif
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1252
    END
END




