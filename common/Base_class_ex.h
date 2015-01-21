
#ifndef GEARS_BASE_COMMON_BASE_CLASS_EX_H__
#define GEARS_BASE_COMMON_BASE_CLASS_EX_H__

#include "gears/base/common/Base_class.h"
#include "gears/base/common/dispatcher_ex.h"

template<class OutType, class GearsClass>
scoped_refptr<OutType> InitializeModuleEx(ModuleEnvironment *module_environment,
                  JsCallContext *context,
                  GearsClass* module) {
  module->InitModuleEnvironment(module_environment);
  scoped_ptr<EaseDispatcher<GearsClass> > dispatcher(
      new EaseDispatcher<GearsClass>(module));

  if (!module_environment->js_runner_->
          InitializeModuleWrapper(module, dispatcher.get(), context)) {
    delete module;
    return scoped_refptr<OutType>();
  }

  dispatcher.release();

#if BROWSER_NPAPI
  // Can not Unref in here, because of module's ref count is 1, outer will use this object,
  // call Unref in CreateModuleEx.
  // module->GetWrapper()->Unref();
#endif

  return scoped_refptr<OutType>(module);
}

template<class GearsClass, class OutType>
bool CreateModuleEx(ModuleEnvironment *module_environment,
    JsCallContext *context,
    scoped_refptr<OutType>* module) {
  scoped_refptr<OutType> impl = InitializeModuleEx<OutType>(module_environment, context, new GearsClass);
  if (impl.get()) {
    *module = impl;

#if BROWSER_NPAPI
    // In NPAPI, objects are created with refcount 1.  We want module (the
    // scoped_refptr<OutType>) to have the only reference, so we Unref here,
    // after the scoped_refptr has taken a reference in the line above.
    (*module)->GetWrapper()->Unref();
#endif
    return true;
  } else {
    return false;
  }
}

#endif //GEARS_BASE_COMMON_BASE_CLASS_EX_H__
