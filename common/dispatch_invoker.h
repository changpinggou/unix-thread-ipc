#ifndef GEARS_BASE_COMMON_DISPATCH_INVOKER_H_
#define GEARS_BASE_COMMON_DISPATCH_INVOKER_H_


#include "gears/base/common/dispatcher.h"
#include "gears/base/common/invoker_misc.h"


namespace invoker_detail{


typedef void (*deallocator)(void*);


//helper object provide static deallocator, which allow stand-alone call
//this trick help to get through the standard rule
//which doesn't allow convert from mem-func-ptr to void* and back
//instead, we convert the holder into void*,
//invoke function holds necessary type info to cast it back.
//and we deallocate it with it's own static deallocator
//while holders go out of life time cycle
template<typename _FuncT>
struct mem_func_holder{
  typedef mem_func_holder<_FuncT> my_type;
  _FuncT content;

  explicit mem_func_holder(_FuncT pFunc)
      :content(pFunc){
  }

  static void deallocator(void* pMem){
      my_type* pObject = static_cast<my_type*>(pMem);
      delete pObject;
  }
};

template<typename _FuncT>
mem_func_holder<_FuncT>* make_mem_func_holder(_FuncT pFunc){
  return new mem_func_holder<_FuncT>(pFunc);
}

template<typename _FuncT>
deallocator make_mem_func_deallocator(_FuncT pFunc){
  return &mem_func_holder<_FuncT>::deallocator;
}


//non-specialized version, never use
//all partial-specializations are generate through latter file iterations
template <typename Ty_>
class invoker;


template <typename RT, typename Ty_>
class invoker<RT(Ty_::*)(JsCallContext *)>{
 public:
  typedef Ty_ ImplClass;
  typedef void(*invoker_type)(void* , JsCallContext *, ImplClass *);
  typedef RT(Ty_::*my_func_t)(JsCallContext *);
  typedef mem_func_holder<my_func_t> my_holder_t;


  template<typename _ResultType>
  static inline void invoke_helper(my_func_t func,
                  JsCallContext *context, ImplClass * pThis){
    RT ret = (pThis->*func)(context);

    //return value is yet set, set it
    if(!context->is_return_value_set()){
      typedef var_arg_wrapper<RT>::arg_type unqualified_ret_type;
      context->SetReturnValue((JsParamType)JsTypeTraits<unqualified_ret_type>
              ::value_type, JsTypeTraits<unqualified_ret_type>::as_retval(ret));
    }
  }

  template<>
  static inline void invoke_helper<void>(my_func_t func,
                  JsCallContext * context, ImplClass * pThis){
    (pThis->*func)(context);
  }


  static inline void invoke(void* func, JsCallContext *context,
                ImplClass * pThis){
    my_holder_t* func_holder = reinterpret_cast<my_holder_t*>(func);
    my_func_t act_func = func_holder->content;
    invoke_helper<RT>(act_func, context, pThis);
  }
};


template <typename FuncType_>
typename invoker<FuncType_>::invoker_type
  make_invoker(FuncType_ pFunc,
          const typename invoker<FuncType_>::ImplClass *_unused) {
  //parameter _unused is just for type checking. with this we can easily
  //ensure the FuncType_ type is a member function pointer of ImplClass type
  //before further template deduction
  //which will generate a subtle compile error
  return &invoker<FuncType_>::invoke;
}




//preprocessors below generate invokers we need to adapt kinds of function
//this idea is from Douglas Gregor's boost.Function
//codes are similar too

//at most 15 parameters support, 128 at most, it relied to boost::preprocessor
#define EASY_DISP_MAX_ARGS() 15

//generate functions have arguments from 0 to 15
#define BOOST_PP_ITERATION_LIMITS (0, EASY_DISP_MAX_ARGS())

//iterate to generate partial-specialized version of invoker
#define BOOST_PP_FILENAME_1 "dispatcher_invoker_template.h"
#include BOOST_PP_ITERATE()

//a second pass, for const member function

#define EASY_DISP_CONST_ITERATION

#define EASY_DISP_MAX_ARGS() 15

//generate functions have arguments from 0 to 15
#define BOOST_PP_ITERATION_LIMITS (0, EASY_DISP_MAX_ARGS())

//iterate to generate partial-specialized version of invoker
#define BOOST_PP_FILENAME_1 "dispatcher_invoker_template.h"
#include BOOST_PP_ITERATE()


#undef EASY_DISP_CONST_ITERATION

};

#endif  //GEARS_BASE_COMMON_DISPATCH_INVOKER_H_

