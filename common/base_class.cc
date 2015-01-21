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

#include <assert.h>

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/leak_counter.h"
#include "gears/base/common/security_model.h"  // for kUnknownDomain
#include "gears/base/common/string_utils.h"
#include "gears/base/common/Dispatcher_exception.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

#if BROWSER_FF
#include "gears/base/firefox/dom_utils.h"
#if defined(WIN32)
#include "gears/desktop/drag_and_drop_utils_win32.h"
#endif

#elif BROWSER_IE && !defined(OS_WINCE)
#include "gears/base/ie/activex_utils.h"
#include "gears/desktop/drag_and_drop_utils_win32.h"

#elif BROWSER_IEMOBILE
#include "gears/base/ie/activex_utils.h"

#elif BROWSER_NPAPI
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/np_utils.h"
#endif

ModuleEnvironment::ModuleEnvironment(SecurityOrigin security_origin,
#if BROWSER_IE || BROWSER_IEMOBILE
                                     IUnknown *iunknown_site,
#endif
                                     bool is_worker,
                                     JsRunnerInterface *js_runner,
                                     BrowsingContext *browsing_context)
    : security_origin_(security_origin),
#if BROWSER_FF || BROWSER_NPAPI
      js_context_(js_runner->GetContext()),
#elif BROWSER_IE || BROWSER_IEMOBILE
      iunknown_site_(iunknown_site),
#endif
      is_worker_(is_worker),
      js_runner_(js_runner),
      browsing_context_(browsing_context),
      permissions_manager_(security_origin, is_worker),
      use_specified_native_window_(false),
      native_window_(NULL){
  LEAK_COUNTER_INCREMENT(ModuleEnvironment);
#if BROWSER_FF
  assert(js_context_ != NULL);
#endif

#if (BROWSER_IE || BROWSER_FF) && defined(WIN32)
  if (!is_worker) {
    drop_target_interceptor_.reset(DropTargetInterceptor::Intercept(this));
  }
#endif
  js_runner_->OnModuleEnvironmentAttach();
}


ModuleEnvironment::~ModuleEnvironment() {
  LEAK_COUNTER_DECREMENT(ModuleEnvironment);
  if (js_runner_)
    js_runner_->OnModuleEnvironmentDetach();
}


#if BROWSER_FF
ModuleEnvironment *ModuleEnvironment::CreateFromDOM() {
#elif BROWSER_IE || BROWSER_IEMOBILE
ModuleEnvironment *ModuleEnvironment::CreateFromDOM(IUnknown *site) {
#elif BROWSER_NPAPI
ModuleEnvironment *ModuleEnvironment::CreateFromDOM(JsContextPtr instance) {
#endif
  bool is_worker = false;
  SecurityOrigin security_origin;
  scoped_refptr<BrowsingContext> browsing_context;
  // Regardlesss of BROWSER_XX, we call NewDocumentJsRunner to create a new
  // DocumentJsRunner instance. Such an object is self-deleting, so that we do
  // not need to, for example, put the resultant object inside a scoped_ptr.
#if BROWSER_FF
  JsContextPtr cx;
  bool succeeded = DOMUtils::GetJsContext(&cx) &&
                   DOMUtils::GetPageOrigin(&security_origin) &&
                   DOMUtils::GetPageBrowsingContext(&browsing_context);
  if (!succeeded) { return NULL; }
  return new ModuleEnvironment(
      security_origin, is_worker, NewDocumentJsRunner(NULL, cx),
      browsing_context.get());
#elif BROWSER_IE || BROWSER_IEMOBILE
  bool succeeded =
      ActiveXUtils::GetPageOrigin(site, &security_origin) &&
      ActiveXUtils::GetPageBrowsingContext(site, &browsing_context);
  if (!succeeded) { return NULL; }
  return new ModuleEnvironment(
      security_origin, site, is_worker, NewDocumentJsRunner(site, NULL),
      browsing_context.get());
#elif BROWSER_NPAPI
  bool succeeded =
      BrowserUtils::GetPageSecurityOrigin(instance, &security_origin) &&
      BrowserUtils::GetPageBrowsingContext(instance, &browsing_context);
  if (!succeeded) { return NULL; }
  return new ModuleEnvironment(
      security_origin, is_worker,
      NewDocumentJsRunner(NULL, instance), browsing_context.get());
#endif
}


ModuleImplBaseClass::ModuleImplBaseClass(const std::string &name)
    : module_name_(name) {
  LEAK_COUNTER_INCREMENT(ModuleImplBaseClass);
  ADD_LEAK_COUNTER_DETAIL(this);
}


ModuleImplBaseClass::~ModuleImplBaseClass() {
  LEAK_COUNTER_DECREMENT(ModuleImplBaseClass);
  REMOVE_LEAK_COUNTER_DETAIL(this);
}


void ModuleImplBaseClass::InitModuleEnvironment(
    ModuleEnvironment *source_module_environment) {
  // First, check that we weren't previously initialized.
  assert(!module_environment_.get());
  assert(source_module_environment);
  // We want to make sure security_origin_ is initialized, but that isn't
  // exposed directly.  So access any member to trip its internal
  // 'initialized_' assert.
  assert(source_module_environment->security_origin_.port() != -1);
  module_environment_.reset(source_module_environment);
}


void ModuleImplBaseClass::GetModuleEnvironment(
    scoped_refptr<ModuleEnvironment> *out) const {
  assert(module_environment_.get());
  out->reset(module_environment_.get());
}

bool ModuleImplBaseClass::EnvIsWorker() const {
  assert(module_environment_.get());
  return module_environment_->is_worker_;
}

const std::string16& ModuleImplBaseClass::EnvPageLocationUrl() const {
  assert(module_environment_.get());
  return module_environment_->security_origin_.full_url();
}

const SecurityOrigin& ModuleImplBaseClass::EnvPageSecurityOrigin() const {
  assert(module_environment_.get());
  return module_environment_->security_origin_;
}

BrowsingContext *ModuleImplBaseClass::EnvPageBrowsingContext() const {
  assert(module_environment_.get());
  return module_environment_->browsing_context_.get();
}

JsRunnerInterface *ModuleImplBaseClass::GetJsRunner() const {
  assert(module_environment_.get());
  return module_environment_->js_runner_;
}

PermissionsManager *ModuleImplBaseClass::GetPermissionsManager() const {
  assert(module_environment_.get());
  return &(module_environment_->permissions_manager_);
}

void ModuleImplBaseClass::Ref() {
  assert(js_wrapper_);
  js_wrapper_->Ref();
}

void ModuleImplBaseClass::Unref() {
  assert(js_wrapper_);
  js_wrapper_->Unref();
}

JsToken ModuleImplBaseClass::GetWrapperToken() const {
  return js_wrapper_->GetWrapperToken();
}

bool ModuleImplBaseClass::InvokeCallback(const ScopedJsCallback& callback, int argc,
                                    JsParamToSend *argv) {
  JsRootedCallback *handler = callback.get();
  if(handler){
    JsRunnerInterface *runner = GetJsRunner();
    if(runner){
      return runner->InvokeCallback(handler, NULL, argc, argv, NULL);
    }
  }
  return false;
}

void ModuleImplBaseClass::InvokeCallbackUnsafe(const ScopedJsCallback& callback, int argc,
    JsParamToSend *argv) {
  JsRunnerInterface *runner = GetJsRunner();
  if(!runner) {
    throw Exception16(L"js runner is NULL");
  }
  JsRootedCallback *handler = callback.get();
  if(!handler) {
    runner->ThrowGlobalError(L"callback is empty");
    return;
  }
  runner->InvokeCallback(handler, NULL, argc, argv, NULL);
}

