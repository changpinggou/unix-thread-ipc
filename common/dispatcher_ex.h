/////////////////////////////////////////////////////////////////////
//Copyright 2009, NetEase.com Inc.
//All Rights Reserved.
//Author : Laurence Von
/////////////////////////////////////////////////////////////////////

//EaseDispatcher provide another choise of DispatcherInterface implemenetation
//This impl is easier to use, all value types supportted by JsCallContext
//can be used directly as c++ function argument type.
//they are listed below:
//std::string16
//bool
//int32
//int64
//double
//ModuleImplBaseClass*
//AbstractJsToken
//--notice: following three types have different param/return value type
//JsCallbackRef/JsRootToken*
//JsArrayRef/JsArray*
//JsObjectRef/JsObject*

//when use as argument types
//built-in types and ModuleImplBaseClass*, AbstractJsToken can be used
//both as a by-value/by const reference parameter
//JsCallbackRef, JsArrayRef,JsObjectRef are in fact a reference
//use the directly

//when use as return value
//if your return value is an object which is owned by other object
//you can return it as a const reference
//the dispatcher will copy it before give it to the JsRunner
//if your return value is an newly created JsObject/JsArray/JsRootToken
//return the its raw object pointer, dispatcher will rip contained value
//from it and free the wrapping object


#ifndef GEARS_BASE_COMMON_DISPATCHER_EX_H__
#define GEARS_BASE_COMMON_DISPATCHER_EX_H__

#include <boost/preprocessor.hpp>
#include <boost/type_traits.hpp>

#include "gears/base/common/dispatcher_exception.h"
#include "gears/base/common/dispatcher.h"
#include "gears/base/common/dispatch_invoker.h"

using std::exception;

#define DECLARE_EASE_DISPATCHER(ImplClass) \
class ImplClass; \
template <> \
const ThreadLocals::Slot EaseDispatcher<ImplClass>::\
        kThreadLocalsKey = ThreadLocals::Alloc()


//more close to cpp style usage, rely on boost
template <typename T>
class EaseDispatcher :public DispatcherInterface{
 public:
  typedef T ImplClass;
  typedef DispatcherInterface MyBase;
  typedef void (ImplClass::*ImplCallback)(JsCallContext *);
  typedef void(*invoker_type)(void* , JsCallContext *, ImplClass *);
  typedef invoker_detail::deallocator deallocator;

  struct invoke_data{
    invoker_type  pInvoke;
    void         *pFunc;
    deallocator   pDealloc;

    invoke_data()
      :pInvoke(NULL), pFunc(NULL), pDealloc(NULL){
    };

    ~invoke_data(){
      if(pDealloc && pFunc)
        pDealloc((void*)pFunc);
    }

    template <typename func_>
    void fill(func_ callback){
      //for type checking, for more detail
      const ImplClass* pNULL = NULL;
      pInvoke = invoker_detail::make_invoker(callback, pNULL);
      pFunc = (void*)invoker_detail::make_mem_func_holder(callback);
      pDealloc = invoker_detail::make_mem_func_deallocator(callback);
    }
  };

  typedef std::map<DispatchId, invoke_data> IDList;

  struct ThreadLocalVariables {
    bool did_init_class;
    IDList property_getters;
    IDList property_setters;
    IDList methods;
    DispatcherNameList members;
    ThreadLocalVariables() : did_init_class(false) {}
  };

  static void DeleteThreadLocals(void *context);
  static ThreadLocalVariables &GetThreadLocals();

  EaseDispatcher(ImplClass* impl);

  //Generic version, for our needs
  template<typename func_type>
  static bool IsImplCallbackValid(func_type callback) {
    if (sizeof (func_type) == sizeof (void *)) {
      // If this is an architecture where pointer-to-member is a plain
      // function pointer to thunks, then this is just a comparison to
      // NULL and this function inlines away.
      return callback != NULL;
    } else {
      // Otherwise, we need to compare the raw binary value with zero
      // using a union.
      const int words =
          (sizeof (func_type) + sizeof (int) - 1) / sizeof (int);
      union {
        func_type value;
        int data[words];
      } combined;
      // Initialize the union to zero.
      for (int i = 0; i < words; ++i) {
        combined.data[i] = 0;
      }
      // Set the ImplCallback.
      combined.value = callback;
      // Check if the union is now non-zero.
      for (int i = 0; i < words; ++i) {
        if (combined.data[i] != 0) {
          return true;
        }
      }
      return false;
    }
  }

  // DispatcherInterface:
  virtual bool HasMethod(DispatchId method_id);
  virtual bool HasPropertyGetter(DispatchId property_id);
  virtual bool HasPropertySetter(DispatchId property_id);
  virtual bool CallMethod(DispatchId method_id, JsCallContext *context);
  virtual bool GetProperty(DispatchId property_id, JsCallContext *context);
  virtual bool SetProperty(DispatchId property_id, JsCallContext *context);
  virtual const DispatcherNameList &GetMemberNames();
  virtual DispatchId GetDispatchId(const std::string &member_name);

 protected:
  template <typename U>
  static void BindProperty(const char *name, U(ImplClass::*getter)()){
    typedef void(ImplClass::*V)(U);
    DispatchId id = GetStringIdentifier(name);
    GetPropertyGetterList()[id].fill(getter);
    GetPropertySetterList()[id].fill((V)NULL);
    GetThreadLocals().members[name] = id;
  }

  template <typename U>
  static void BindProperty(const char *name, U(ImplClass::*getter)() const){
    typedef void(ImplClass::*V)(U);
    DispatchId id = GetStringIdentifier(name);
    GetPropertyGetterList()[id].fill(getter);
    GetPropertySetterList()[id].fill((V)NULL);
    GetThreadLocals().members[name] = id;
  }

  template <typename U, typename V>
  static void BindProperty(const char *name, U(ImplClass::*getter)(),
                          void(ImplClass::*setter)(V)){
    typedef boost::remove_reference<U>::type U_no_ref;
    typedef boost::remove_reference<V>::type V_no_ref;
    typedef boost::remove_cv<U_no_ref>::type U_no_ref_cv;
    typedef boost::remove_cv<V_no_ref>::type V_no_ref_cv;
    BOOST_STATIC_ASSERT((boost::is_same<U_no_ref_cv, V_no_ref_cv>::value));
    DispatchId id = GetStringIdentifier(name);
    GetPropertyGetterList()[id].fill(getter);
    GetPropertySetterList()[id].fill(setter);
    GetThreadLocals().members[name] = id;
  }

  template <typename U, typename V>
  static void BindProperty(const char *name, U(ImplClass::*getter)() const,
                          void(ImplClass::*setter)(V)){
    typedef boost::remove_reference<U>::type U_no_ref;
    typedef boost::remove_reference<V>::type V_no_ref;
    typedef boost::remove_cv<U_no_ref>::type U_no_ref_cv;
    typedef boost::remove_cv<V_no_ref>::type V_no_ref_cv;
    BOOST_STATIC_ASSERT((boost::is_same<U_no_ref_cv, V_no_ref_cv>::value));
    DispatchId id = GetStringIdentifier(name);
    GetPropertyGetterList()[id].fill(getter);
    GetPropertySetterList()[id].fill(setter);
    GetThreadLocals().members[name] = id;
  }

  template <typename func_type>
  static void BindMethod(const char *name, func_type callback){
    DispatchId id = GetStringIdentifier(name);
    GetMethodList()[id].fill(callback);
    GetThreadLocals().members[name] = id;
  }

  void InvokeCallback(const invoke_data* callback, JsCallContext *context);

  static IDList& GetPropertyGetterList() {
    return GetThreadLocals().property_getters;
  }
  static IDList& GetPropertySetterList() {
    return GetThreadLocals().property_setters;
  }
  static IDList& GetMethodList() {
    return GetThreadLocals().methods;
  }
  static void Init();

 private:
   ImplClass *impl_;
   static const ThreadLocals::Slot kThreadLocalsKey;

   DISALLOW_EVIL_CONSTRUCTORS(EaseDispatcher<T>);
};

template <typename T>
EaseDispatcher<T>::EaseDispatcher(ImplClass* impl):impl_(impl){
   // Ensure that property and method mappings are initialized.
   ThreadLocalVariables &locals = GetThreadLocals();
   if (!locals.did_init_class) {
     locals.did_init_class = true;
     BindProperty("__type__", &T::get_wide_module_name);
     Init();
   }
}

template<class T>
void EaseDispatcher<T>::DeleteThreadLocals(void *context) {
  ThreadLocalVariables *locals =
      reinterpret_cast<ThreadLocalVariables*>(context);
  delete locals;
}

template<class T>
typename EaseDispatcher<T>::ThreadLocalVariables &EaseDispatcher<T>::GetThreadLocals() {
  const ThreadLocals::Slot &key = kThreadLocalsKey;
  ThreadLocalVariables *locals =
      reinterpret_cast<ThreadLocalVariables*>(ThreadLocals::GetValue(key));
  if (!locals) {
    locals = new ThreadLocalVariables;
    ThreadLocals::SetValue(key, locals, &DeleteThreadLocals);
  }
  return *locals;
}

//simply duplicated from Dispatcher
template<class T>
bool EaseDispatcher<T>::HasMethod(DispatchId method_id) {
  const IDList &methods = GetMethodList();
  return methods.find(method_id) != methods.end();
}

template<class T>
bool EaseDispatcher<T>::HasPropertyGetter(DispatchId property_id) {
  const IDList &properties = GetPropertyGetterList();
  return properties.find(property_id) != properties.end();
}

template<class T>
bool EaseDispatcher<T>::HasPropertySetter(DispatchId property_id) {
  const IDList &properties = GetPropertySetterList();
  return properties.find(property_id) != properties.end();
}

template<class T>
const DispatcherNameList &EaseDispatcher<T>::GetMemberNames() {
  return GetThreadLocals().members;
}

template<class T>
DispatchId EaseDispatcher<T>::GetDispatchId(const std::string &member_name){
  const DispatcherNameList &member_names = GetMemberNames();
  DispatcherNameList::const_iterator result = member_names.find(member_name);
  if (result != member_names.end()) {
    return result->second;
  } else {
    return NULL;
  }
}


//our own overrides
template<class T>
bool EaseDispatcher<T>::CallMethod(DispatchId method_id, JsCallContext *context){
  const IDList &methods = GetMethodList();
  typename IDList::const_iterator method = methods.find(method_id);
  if (method == methods.end())
    return false;
  const invoke_data *callback = &method->second;

  assert(IsImplCallbackValid(callback->pInvoke));
  assert(IsImplCallbackValid(callback->pFunc));
  InvokeCallback(callback, context);
  return true;
}

template<class T>
bool EaseDispatcher<T>::GetProperty(DispatchId property_id,
                                JsCallContext *context) {
  const IDList &properties = GetPropertyGetterList();
  typename IDList::const_iterator property = properties.find(property_id);
  if (property == properties.end())
    return false;
  const invoke_data *callback = &property->second;

  assert(IsImplCallbackValid(callback->pInvoke));
  assert(IsImplCallbackValid(callback->pFunc));
  InvokeCallback(callback, context);
  return true;
}

template<class T>
bool EaseDispatcher<T>::SetProperty(DispatchId property_id,
                                JsCallContext *context) {
  const IDList &properties = GetPropertySetterList();
  typename IDList::const_iterator property = properties.find(property_id);
  if (property == properties.end()) {
    return false;
  }
  const invoke_data *callback = &property->second;

  if (!IsImplCallbackValid(callback->pInvoke)) {  // Read only property.
    context->SetException(
                 STRING16(L"Cannot assign value to a read only property."));
    return true;
  }

  InvokeCallback(callback, context);
  return true;
}


template<class T>
void EaseDispatcher<T>::InvokeCallback(const invoke_data* callback,
                                JsCallContext *context){
  try{
    callback->pInvoke(callback->pFunc, context, impl_);
  }
  catch(DispException &e){
    if (context->is_exception_set()) return;
    if(!e.what16().empty())
      context->SetException(e.what16());
    else
      context->SetException(L"Unknown Exception catched!");
  }
}


#endif //GEARS_BASE_COMMON_DISPATCHER_EX_H__



