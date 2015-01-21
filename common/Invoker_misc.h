#ifndef GEARS_BASE_COMMON_INVOKER_MISC_H_
#define GEARS_BASE_COMMON_INVOKER_MISC_H_

#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_reference.hpp>

#include "gears/base/common/js_types.h"
#include "gears/base/common/js_dom_element.h"




namespace invoker_detail{

//traits to perform type to JsParamType mapping
template<typename Ty_>
struct JsTypeTraits{
  //for unsupported type, generate compile-time error
};

#define __BYPASS_AS_OUT_PARAMETER(type_)\
  static inline type_* as_retval(type_* p) {\
    return p;\
  } \
  enum {need_add_refs = 0}; \
  static void DoRef(const type_&) {}

#define __DUMMY_AS_OUT_PARAMETER(type_)\
  static inline type_* as_out_parameter(type_ &p){\
    return &p;\
  }\
  static inline type_* as_retval(const type_ &p){\
    return (type_*) &p;\
  } \
  enum {need_add_refs = 0}; \
  static void DoRef(const type_&) {}

#define __SCOPED_PTR_AS_OUT_PARAMETER(type_)\
  static inline type_** as_out_parameter(scoped_ptr<type_>& p) {\
    return ::as_out_parameter(p);\
  }\
  static inline type_* as_retval(const scoped_ptr<type_>& p){\
    scoped_ptr<type_>* non_const = (scoped_ptr<type_>*) &p;\
    return non_const->release();\
  } \
  enum {need_add_refs = 0}; \
  static void DoRef(const scoped_ptr<type_>&) {}

template<>
struct JsTypeTraits<bool>{
  enum{value_type = JSPARAM_BOOL};
  __DUMMY_AS_OUT_PARAMETER(bool);
};


template<>
struct JsTypeTraits<int32>{
  enum{value_type = JSPARAM_INT};
  __DUMMY_AS_OUT_PARAMETER(int32);
};


template<>
struct JsTypeTraits<int64>{
  enum{value_type = JSPARAM_INT64};
  __DUMMY_AS_OUT_PARAMETER(int64);
};

template<>
struct JsTypeTraits<double>{
  enum{value_type = JSPARAM_DOUBLE};
  __DUMMY_AS_OUT_PARAMETER(double);
};

template<>
struct JsTypeTraits<std::string16>{
  enum{value_type = JSPARAM_STRING16};
  __DUMMY_AS_OUT_PARAMETER(std::string16);
};

template<>
struct JsTypeTraits<scoped_ptr<Json::Value> >{
  enum{value_type = JSPARAM_CPPJSON};
  enum {need_add_refs = 0};

  static void DoRef(const scoped_ptr<Json::Value> &) {}
  static inline Json::Value** as_out_parameter(scoped_ptr<Json::Value>& p) {
    return ::as_out_parameter(p);
  }
};



template<>
struct JsTypeTraits<ScopedJsObject>{
  enum{value_type = JSPARAM_OBJECT};
  __SCOPED_PTR_AS_OUT_PARAMETER(JsObject);
};


template<>
struct JsTypeTraits<ScopedJsArray>{
  enum{value_type = JSPARAM_ARRAY};
  __SCOPED_PTR_AS_OUT_PARAMETER(JsArray);
};

template<>
struct JsTypeTraits<JsObject*>{
  enum{value_type = JSPARAM_OBJECT};
  __BYPASS_AS_OUT_PARAMETER(JsObject);
};


template<>
struct JsTypeTraits<JsArray*>{
  enum{value_type = JSPARAM_ARRAY};
  __BYPASS_AS_OUT_PARAMETER(JsArray);
};


template<>
struct JsTypeTraits<ModuleImplBaseClass*>{
  typedef ModuleImplBaseClass* pointer_type;
  enum{value_type = JSPARAM_MODULE};
  enum {need_add_refs = 0};
  //ModuleImplBaseClass has a very different return value type
  //as a result, we can not use those *_AS_OUT_PARAMETER macros
  static inline pointer_type* as_out_parameter(pointer_type &p){
    return &p;
  }
  static void DoRef(ModuleImplBaseClass*) {}
};

void DoRefModule(ScopedModule &ref);

template<>
struct JsTypeTraits<ScopedModule>{
  typedef ModuleImplBaseClass* pointer_type;
  enum {need_add_refs = 1};
  enum{value_type = JSPARAM_MODULE};
  static inline pointer_type* as_out_parameter(ScopedModule& p){
    return ::as_out_parameter(p);
  }
  static inline pointer_type as_retval(const ScopedModule &p){
    return p.get();
  }
  static void DoRef(ScopedModule &ref) { DoRefModule(ref); }
};

//this facility is DEPRECATED and used only with FileSumitter
//thus, we decided not to support it
#if 0
template<>
struct JsTypeTraits<JsDomElement>{
  enum{value_type = JSPARAM_DOM_ELEMENT};
};

#endif

template<>
struct JsTypeTraits<JsToken>{
  typedef JsToken* pointer_type;
  enum{value_type = JSPARAM_TOKEN};
  __DUMMY_AS_OUT_PARAMETER(JsToken);
};

template<>
struct JsTypeTraits<JsScopedToken*>{
  typedef JsScopedToken* pointer_type;
  enum{value_type = JSPARAM_TOKEN};
  static inline pointer_type as_retval(pointer_type p){
    return p;
  }
};


template<>
struct JsTypeTraits<AbstractJsToken>{
  enum{value_type = JSPARAM_ABSTRACT_TOKEN};
  __DUMMY_AS_OUT_PARAMETER(AbstractJsToken);
};


template<>
struct JsTypeTraits<ScopedJsCallback>{
  enum{value_type = JSPARAM_FUNCTION};
  __SCOPED_PTR_AS_OUT_PARAMETER(JsRootedToken);
};

template<>
struct JsTypeTraits<JsRootedToken*>{
  enum{value_type = JSPARAM_FUNCTION};
  __BYPASS_AS_OUT_PARAMETER(JsRootedToken);
};



//const type and const reference type shall be extracted
template<typename _Ty, typename _IsConst, typename _IsRef>
struct _Var_arg_quailifier;

template<typename _Ty>
struct _Var_arg_quailifier<_Ty, boost::true_type, boost::true_type>
{
    typedef  typename boost::remove_const
      <typename boost::remove_reference<_Ty>::type>::type type;
};

template<typename _Ty>
struct _Var_arg_quailifier<_Ty, boost::true_type, boost::false_type>
{
    typedef  typename boost::remove_const<_Ty>::type  type;
};

template<typename _Ty>
struct _Var_arg_quailifier<_Ty, boost::false_type, boost::false_type>
{
    typedef  _Ty  type;
};


template<typename _Ty>
struct _Var_arg_quailifier<_Ty, boost::false_type, boost::true_type>
{
    typedef typename boost::remove_reference<_Ty>::type  type;
};


//built-in array support is droped
//this thing serve as a cv/ref qualifier removal facility
//make const/const ref available as function arg
template<typename _Ty>
struct var_arg_wrapper
{
  //if a compile-time error gen here, check arguement type
  //we only accept value/const value/const reference
  typedef typename _Var_arg_quailifier<_Ty, typename boost::is_const<
        typename boost::remove_reference<_Ty>::type >::type,
        typename boost::is_reference<_Ty>::type >::type arg_type;
};

};


#undef __DUMMY_AS_OUT_PARAMETER
#undef __SCOPED_PTR_AS_OUT_PARAMETER

#endif //GEARS_BASE_COMMON_INVOKER_MISC_H_
