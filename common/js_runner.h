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
//
// Provides a platform-independent interface for instantiating a JavaScript
// execution engine and running user-provided code.

#ifndef GEARS_BASE_COMMON_JS_RUNNER_H__
#define GEARS_BASE_COMMON_JS_RUNNER_H__

#include "gears/base/common/js_types.h"
#include "gears/base/common/leak_counter.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"

class DispatcherInterface;
class ModuleImplBaseClass;

#if BROWSER_FF
// Wraps a set of calls to JS_BeginRequest and JS_EndRequest.
class JsRequest {
 public:
  JsRequest(JSContext *cx) : cx_(cx) {
    JS_BeginRequest(cx_);
  }
  ~JsRequest() {
    JS_EndRequest(cx_);
  }

 private:
  JSContext *cx_;
};
#endif


// Represents an error that occured inside a JsRunner.
struct JsErrorInfo {
  int line;
  std::string16 message;
  // TODO(aa): code, so that people can detect certain errors programatically.
  // TODO(aa): file, when workers can have multiple files?
};


// Interface for clients of JsRunner to implement if they want to handle errors.
class JsErrorHandlerInterface {
 public:
  virtual void HandleError(const JsErrorInfo &error_info) = 0;
};

enum JsEventType {
  JSEVENT_UNLOAD,
  MAX_JSEVENTS
};

// Interface for clients of JsRunner to implement if they want to handle events.
class JsEventHandlerInterface {
 public:
  virtual void HandleEvent(JsEventType event) = 0;
};

// Declares the platform-independent interface that Gears internals require
// for running JavaScript code.
class JsRunnerInterface {
 public:
  virtual ~JsRunnerInterface() {};

  // Some JsRunnerInterface implementations are self-deleting, but will not
  // delete themselves whilst the ModuleEnvironment for that JsRunnerInterface
  // is attached.
  virtual void OnModuleEnvironmentAttach() = 0;
  virtual void OnModuleEnvironmentDetach() = 0;

  // increments refcount
  virtual bool AddGlobal(const std::string16 &name,
                         ModuleImplBaseClass *object) = 0;
  virtual bool Start(const std::string16 &full_script) = 0;
  virtual bool Start(const std::string16 &name, const std::string16 &full_script) { return Start(full_script); }
  virtual bool Stop() = 0;
  virtual JsContextPtr GetContext() = 0;
  virtual bool Eval(const std::string16 &script) = 0;
  virtual void SetErrorHandler(JsErrorHandlerInterface *error_handler) = 0;

#if BROWSER_FF
  virtual bool EvalFuncWithModuleArg(
      const std::string16 &script,
      ModuleImplBaseClass *module,
      jsval *return_value) = 0;
#endif

  // Creates a new object in the JavaScript engine. The caller takes ownership
  // of the returned value.
  virtual JsObject *NewObject(// TODO(zork): Remove this when we find the error.
                              bool dump_on_error = false) = 0;
  // Creates a new Error object with the specified message.
  virtual JsObject *NewError(const std::string16 &message,
                             // TODO(zork): Remove this when we find the error.
                             bool dump_on_error = false) = 0;
  // Creates a new Date object with the specified time.
  virtual JsObject *NewDate(int64 milliseconds_since_epoch) = 0;

  // Creates a new Array object in JavaScript engine.
  // The caller takes ownership of the returned value.
  virtual JsArray* NewArray() = 0;

  // Convert a JsObject to our date type.
  virtual bool ConvertJsObjectToDate(JsObject *obj,
                                     int64 *milliseconds_since_epoch) = 0;

#if BROWSER_FF
  virtual bool GetModuleFromJsToken(JsToken token,
                                    ModuleImplBaseClass **module_out) = 0;
#endif

  // Creates and associates the ModuleWrapper for the ModuleImplBaseClass.
  virtual bool InitializeModuleWrapper(ModuleImplBaseClass *module,
                                       DispatcherInterface *dispatcher,
                                       JsCallContext *context) = 0;

  // Invokes a callback. If optional_alloc_retval is specified, this method will
  // create a new JsRootedToken that the caller is responsible for deleting.
  // The this_arg parameter determines the value of the special JavaScript
  // keyword 'this', when executing the callback. Almost always, the caller
  // should just pass NULL, which means 'this' is just the global JS object.
  virtual bool InvokeCallback(const JsRootedCallback *callback,
                              JsObject *this_arg, int argc, JsParamToSend *argv,
                              JsRootedToken **optional_alloc_retval) = 0;

  virtual bool AddEventHandler(JsEventType event_type,
                               JsEventHandlerInterface *handler) = 0;
  virtual bool RemoveEventHandler(JsEventType event_type,
                                  JsEventHandlerInterface *handler) = 0;

#ifdef DEBUG
  virtual void ForceGC() = 0;
#endif

  // Reports an error to the JsRunner's global scope. Equivalent to the
  // following JavaScript: eval("throw new Error('hello')");
  virtual void ThrowGlobalError(const std::string16 &message) = 0;

  virtual bool AbstractJsTokensAreEqual(AbstractJsToken token1,
                                        AbstractJsToken token2) = 0;

  virtual JsParamType JsTokenType(AbstractJsToken token) = 0;

  virtual bool JsTokenToBool(AbstractJsToken token, bool *out) = 0;
  virtual bool JsTokenToInt(AbstractJsToken token, int *out) = 0;
  virtual bool JsTokenToDouble(AbstractJsToken abstract_js_token,
                               double *out) = 0;
  virtual bool JsTokenToString(AbstractJsToken token, std::string16 *out) = 0;
  virtual bool JsTokenToObject(AbstractJsToken token, JsObject **out) = 0;
  virtual bool JsTokenToArray(AbstractJsToken token, JsArray **out) = 0;
  virtual bool JsTokenToModule(AbstractJsToken token,
                               ModuleImplBaseClass **out) = 0;

  virtual bool BoolToJsToken(bool value, JsScopedToken *out) = 0;
  virtual bool IntToJsToken(int value, JsScopedToken *out) = 0;
  virtual bool DoubleToJsToken(double value, JsScopedToken *out) = 0;
  virtual bool StringToJsToken(const char16 *value, JsScopedToken *out) = 0;
  virtual bool NullToJsToken(JsScopedToken *out) = 0;
  virtual bool UndefinedToJsToken(JsScopedToken *out) = 0;

#if BROWSER_NPAPI
  virtual NPObject *GetGlobalObject() = 0;
#endif
};

// Wraps the calls for adding and removing event handlers.  This is designed to
// be used with scoped_ptr<>, and also takes care of unregistering itself from
// the JsRunnerInterface, on receiving the JSEVENT_UNLOAD event.
class JsEventMonitor : public JsEventHandlerInterface {
 public:
  JsEventMonitor(JsRunnerInterface *js_runner,
                 JsEventType event_type,
                 JsEventHandlerInterface *handler)
      : js_runner_(js_runner), event_type_(event_type), handler_(handler) {
    assert(js_runner_);
    assert(handler_);
    LEAK_COUNTER_INCREMENT(JsEventMonitor);
    js_runner_->AddEventHandler(event_type_, this);
    if (event_type_ != JSEVENT_UNLOAD) {
      js_runner_->AddEventHandler(JSEVENT_UNLOAD, this);
    }
  }

  ~JsEventMonitor() {
    LEAK_COUNTER_DECREMENT(JsEventMonitor);
    if (js_runner_) {
      js_runner_->RemoveEventHandler(event_type_, this);
      if (event_type_ != JSEVENT_UNLOAD) {
        js_runner_->RemoveEventHandler(JSEVENT_UNLOAD, this);
      }
    }
  }

  virtual void HandleEvent(JsEventType event) {
    if (js_runner_) {
      JsEventHandlerInterface *handler = handler_;
      if (event == JSEVENT_UNLOAD) {
        js_runner_->RemoveEventHandler(event_type_, this);
        if (event_type_ != JSEVENT_UNLOAD) {
          js_runner_->RemoveEventHandler(JSEVENT_UNLOAD, this);
        }
        js_runner_ = NULL;
        handler_ = NULL;
      }
      // Note that this call may delete this JsEventMonitor.
      handler->HandleEvent(event);
    }
  }

 private:
  JsRunnerInterface *js_runner_;
  JsEventType event_type_;
  JsEventHandlerInterface *handler_;
};


// Callers: create instances using the following instead of 'new JsRunner'.
// The latter would require a full class description for JsRunner in this file,
// which is not always possible due to platform-specific implementation details.
// Destroy the object using regular 'delete'.
//
// This interface plays nicely with scoped_ptr, which was a design goal.

// This creates a JsRunner that is used in a worker.
#if BROWSER_FF
// js_runtime is a JSRuntime created via JS_NewRuntime.  The caller retains
// ownership of the runtime.  If NULL is passed in, the JsRunner will
// successfully initialize to a stable but unusable state.
JsRunnerInterface* NewJsRunner(JSRuntime *js_runtime);
#else
JsRunnerInterface* NewJsRunner();
#endif

// This creates a JsRunner that can be used with the script engine running in an
// document.
// TODO(nigeltao): void *base is unused for non-IE browsers, and JsContextPtr
// context is unused on IE. We should merge the two concepts.
JsRunnerInterface* NewDocumentJsRunner(
#if BROWSER_IE || BROWSER_IEMOBILE
    IUnknown *base,
#else
    void *base,
#endif
    JsContextPtr context);


#endif  // GEARS_BASE_COMMON_JS_RUNNER_H__
