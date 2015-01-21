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

#include <algorithm>
#include <cctype>
#include <string>

#include "gears/base/common/security_model.h"
#include "gears/base/common/string_utils.h"
#include "gears/localserver/common/http_constants.h"

#include "third_party/googleurl/src/url_parse.h"
#include <atlbase.h> //put this file before string16.h makes ff2 link error!!!



const char16* kUnknownDomain      = STRING16(L"_null_.localdomain");
const char*   kUnknownDomainAscii =           "_null_.localdomain";
static const char16* kAuthorized[] = {
#ifdef DEBUG
  STRING16(L"http://127.0.0.1"),
#endif
  STRING16(L"http://mail.163.com"),
  STRING16(L"http://email.163.com"),
  STRING16(L"http://126.com"),
  STRING16(L"http://mail.126.net"),
  STRING16(L"http://yeah.net"),
  STRING16(L"http://vip.163.com"),
  STRING16(L"http://188.com"),
  STRING16(L"http://e.163.com"),
  STRING16(L"http://webim.163.com"),
  STRING16(L"http://mailease.163.com"),
  STRING16(L"https://mail.163.com"),
  STRING16(L"https://email.163.com"),
  STRING16(L"https://126.com"),
  STRING16(L"https://mail.126.net"),
  STRING16(L"https://yeah.net"),
  STRING16(L"https://vip.163.com"),
  STRING16(L"https://188.com"),
  STRING16(L"https://e.163.com"),
  STRING16(L"https://webim.163.com"),
  STRING16(L"https://mailease.163.com"),
  NULL
  };


//---------------------------------------------------------------------------
// Assignment constructor 
//---------------------------------------------------------------------------
SecurityOrigin& SecurityOrigin::operator=(const SecurityOrigin& so){
  if(this == &so){
    return *this;
  }
  CopyFrom(so);
  return *this;
}

//---------------------------------------------------------------------------
// Copy Constructor
//---------------------------------------------------------------------------
SecurityOrigin::SecurityOrigin(const SecurityOrigin& so){
  CopyFrom(so);
}


//------------------------------------------------------------------------------
// CopyFrom
//------------------------------------------------------------------------------
void SecurityOrigin::CopyFrom(const SecurityOrigin &security_origin) {
  initialized_ = security_origin.initialized_;
  url_ = security_origin.url_.c_str();
  full_url_ = security_origin.full_url_.c_str();
  scheme_ = security_origin.scheme_.c_str();
  host_ = security_origin.host_.c_str();
  port_ = security_origin.port_;
  port_string_ = security_origin.port_string_.c_str();
}

//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
bool SecurityOrigin::Init(const char16 *full_url, const char16 *scheme,
                          const char16 *host, int port) {
  assert(full_url && scheme && host); // file URLs pass 0 for 'port'
  if (!full_url[0] || !scheme[0] || !host[0])
    return false;

  full_url_ = full_url;
  scheme_ = scheme;
  host_ = host;
  port_ = port;

  port_string_ = IntegerToString16(port_);
  LowerString(scheme_);
  LowerString(host_);

  url_ = scheme_;
  url_ += STRING16(L"://");
  url_ += host;
  if (!IsDefaultPort(scheme_, port_)) {
    url_ += STRING16(L":");
    url_ += port_string_;
  }

  initialized_ = true;
  return true;
}


//------------------------------------------------------------------------------
// InitFromUrl
//------------------------------------------------------------------------------
bool SecurityOrigin::InitFromUrl(const char16 *full_url) {
  initialized_ = false;

  int url_len = char16_wcslen(full_url);

  url_parse::Component scheme_comp;
  if (!url_parse::ExtractScheme(full_url, url_len, &scheme_comp)) {
    return false;
  }

  std::string16 scheme(full_url + scheme_comp.begin, scheme_comp.len);
  LowerString(scheme);
  if (scheme == STRING16(L"http") || scheme == STRING16(L"https")) {
    url_parse::Parsed parsed;
    url_parse::ParseStandardURL(full_url, url_len, &parsed);

    // Disallow urls with embedded username:passwords. These are disabled by
    // default in IE such that InternetCrackUrl fails. To have consistent
    // behavior, do the same for all browsers.
    if (parsed.username.len != -1 || parsed.password.len != -1) {
      return false;
    }

    if (parsed.host.len == -1) {
      return false;
    }

    int port;
    if (parsed.port.len > 0) {
      std::string16 port_str(full_url + parsed.port.begin, parsed.port.len);
      port = ParseLeadingInteger(port_str.c_str(), NULL);
    } else if (scheme == HttpConstants::kHttpsScheme) {
      port = HttpConstants::kHttpsDefaultPort;
    } else {
      port = HttpConstants::kHttpDefaultPort;
    }

    std::string16 host(full_url + parsed.host.begin, parsed.host.len);
    return Init(full_url, scheme.c_str(), host.c_str(), port);
  } else if (scheme == STRING16(L"file")) {
    return Init(full_url, HttpConstants::kFileScheme, kUnknownDomain, 0);
  } else if (scheme == STRING16(L"res")) {
    // if the full_url has url escaped, system can care, need't convert
    return Init(full_url, HttpConstants::kResScheme, kUnknownDomain, 0);
  }

  return false;
}


bool SecurityOrigin::IsAuthorizedOrigin() const{
    static std::vector<SecurityOrigin> authorized_domains;
    //initialize the authorized list, if it's yet inited
    if(authorized_domains.size() == 0){
      SecurityOrigin tmp;
      for(const char16 **ppAuth = kAuthorized; NULL != *ppAuth; ++ppAuth) {
        authorized_domains.push_back(tmp);
        authorized_domains.back().InitFromUrl(*ppAuth);
      }
    }

    bool result = false;

    for(size_t i = 0; i < authorized_domains.size(); ++i){
      if(IsSubDomainOf(authorized_domains[i])){
        result = true;
        break;
      }
    }

    return result;
}

bool SecurityOrigin::IsSubDomainOf(const SecurityOrigin &parent) const {
  typedef std::string16::const_reverse_iterator const_reverse_iterator;

  //negative cases
  if((port_ != parent.port_) || (scheme_ != parent.scheme_) ||
          host_.length() < parent.host_.length()){
    return false;
  }

  const_reverse_iterator rhost = host_.rbegin();
  const_reverse_iterator parent_rhost = parent.host_.rbegin();

  while (true){
    if (*rhost != *parent_rhost){
      return false;
    }

    ++rhost;
    ++parent_rhost;

    if (parent_rhost == parent.host_.rend()){
      if (rhost == host_.rend() || *rhost == '.'){
        return  true;
      } else {
        return false;
      }
    }
  }

  return false;
}

