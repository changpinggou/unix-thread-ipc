#if !defined(BOOST_PP_IS_ITERATING)
# error CEasyDispBinder.invoker - do not include this file!
#endif

#define EASY_DISP_NUM_ARGS BOOST_PP_ITERATION()


// Comma if nonzero number of arguments
#if EASY_DISP_NUM_ARGS == 0
#  define EASY_DISP_COMMA
#else
#  define EASY_DISP_COMMA ,
#endif // EASY_DISP_NUM_ARGS > 0


#ifdef EASY_DISP_CONST_ITERATION
#   define EASY_CONST_FUNCTION_QUALIFIER const
#else
#   define EASY_CONST_FUNCTION_QUALIFIER
#endif //EASY_DISP_CONST_ITERATION


#define EASY_DISP_HELPER_FUNCTION_PARMS_HELPER(z,n,data)\
    typename BOOST_PP_CAT(P,n) BOOST_PP_CAT(arg,n)

#define EASY_DISP_ARGV_TABLE_DEF_ENTRY(z,n,data)\
    { JSPARAM_REQUIRED,\
      (JsParamType)JsTypeTraits<var_arg_wrapper<BOOST_PP_CAT(P,n)>::arg_type>\
      ::value_type, JsTypeTraits<var_arg_wrapper<BOOST_PP_CAT(P,n)>::arg_type>\
      ::as_out_parameter(BOOST_PP_CAT(arg,n)) }

#define EASY_DISP_TEMPLATE_PARMS \
            BOOST_PP_ENUM_PARAMS(EASY_DISP_NUM_ARGS, typename P)

#define EASY_DISP_TEMPLATE_ARGS BOOST_PP_ENUM_PARAMS(EASY_DISP_NUM_ARGS, P)

#define EASY_DISP_FUNCTION_ARGS BOOST_PP_ENUM_PARAMS(EASY_DISP_NUM_ARGS, arg)

#define EASY_DISP_HELPER_FUNCTION_PARMS\
            BOOST_PP_ENUM_TRAILING(EASY_DISP_NUM_ARGS,\
            EASY_DISP_HELPER_FUNCTION_PARMS_HELPER, unused_)


#define EASY_DISP_ARG_DEF_ENTRY(n)\
  typedef typename var_arg_wrapper<BOOST_PP_CAT(P,n)>::arg_type BOOST_PP_CAT(arg_type_, n);\
  BOOST_PP_CAT(arg_type_, n) BOOST_PP_CAT(arg,n);
//= BOOST_PP_CAT(arg_type_, n)();

#define EASY_DISP_REFS_ENTRY(n)\
  if(JsTypeTraits<BOOST_PP_CAT(arg_type_, n)>::need_add_refs) { \
    JsTypeTraits<BOOST_PP_CAT(arg_type_, n)>::DoRef(BOOST_PP_CAT(arg,n));\
  }

#define EASY_DISP_ARGV_TABLE()\
  BOOST_PP_ENUM(EASY_DISP_NUM_ARGS, EASY_DISP_ARGV_TABLE_DEF_ENTRY, unused_)

template <typename RT, typename Ty_
  EASY_DISP_COMMA
    EASY_DISP_TEMPLATE_PARMS
    >
class invoker<RT(Ty_::*)(EASY_DISP_TEMPLATE_ARGS)EASY_CONST_FUNCTION_QUALIFIER>
{
 public:
  typedef Ty_ ImplClass;
  typedef void(*invoker_type)(void* , JsCallContext *, ImplClass *);
  typedef RT(Ty_::*my_func_t)(EASY_DISP_TEMPLATE_ARGS)
    EASY_CONST_FUNCTION_QUALIFIER;
  typedef mem_func_holder<my_func_t> my_holder_t;

    template<typename _ResultType>
    static inline void invoke_helper(my_func_t func, Ty_* pThis,
                                     JsCallContext * context
                                     EASY_DISP_HELPER_FUNCTION_PARMS){
        RT ret = (pThis->*func)(EASY_DISP_FUNCTION_ARGS);
        typedef var_arg_wrapper<RT>::arg_type unqualified_ret_type;
        context->SetReturnValue((JsParamType)JsTypeTraits<unqualified_ret_type>
              ::value_type, JsTypeTraits<unqualified_ret_type>::as_retval(ret));
    }

    template<typename specialized_type>
    static inline void auto_delete_invoke_helper(my_func_t func, Ty_* pThis,
                                                 JsCallContext * context
                                                 EASY_DISP_HELPER_FUNCTION_PARMS) {
        specialized_type ret = (pThis->*func)(EASY_DISP_FUNCTION_ARGS);
        context->SetReturnValue((JsParamType)JsTypeTraits<specialized_type>
              ::value_type, JsTypeTraits<specialized_type>::as_retval(ret));
        delete ret;
    }

    template<>
    static inline void invoke_helper<JsObject*>(my_func_t func, Ty_* pThis,
                                                JsCallContext * context
                                                EASY_DISP_HELPER_FUNCTION_PARMS) {
      auto_delete_invoke_helper<JsObject*>(func, pThis, context
          EASY_DISP_COMMA EASY_DISP_FUNCTION_ARGS);
    }


    template<>
    static inline void invoke_helper<JsArray*>(my_func_t func, Ty_* pThis,
                                               JsCallContext * context
                                               EASY_DISP_HELPER_FUNCTION_PARMS) {
      auto_delete_invoke_helper<JsArray*>(func, pThis, context
          EASY_DISP_COMMA EASY_DISP_FUNCTION_ARGS);
    }

    template<>
    static inline void invoke_helper<JsScopedToken*>(my_func_t func, Ty_* pThis,
                                               JsCallContext * context
                                               EASY_DISP_HELPER_FUNCTION_PARMS) {
      auto_delete_invoke_helper<JsScopedToken*>(func, pThis, context
          EASY_DISP_COMMA EASY_DISP_FUNCTION_ARGS);
    }

    template<>
    static inline void invoke_helper<void>(my_func_t func, Ty_* pThis,
      JsCallContext *
      EASY_DISP_HELPER_FUNCTION_PARMS){
        //for void result type, do nothing to the return value
        (pThis->*func)(EASY_DISP_FUNCTION_ARGS);
    }

    static inline void invoke(void* func, JsCallContext *context,
                  ImplClass *pThis){
      //leave argument type/count checking to JsCallContext
      //restore type info, make the function call available
      my_holder_t* func_holder = reinterpret_cast<my_holder_t*>(func);
      Ty_* pActThis = static_cast<Ty_*>(pThis);
      my_func_t act_func = func_holder->content;

#if EASY_DISP_NUM_ARGS != 0

      //first, define arguments local variable
#define BOOST_PP_LOCAL_LIMITS (0, BOOST_PP_DEC(EASY_DISP_NUM_ARGS))
#define BOOST_PP_LOCAL_MACRO EASY_DISP_ARG_DEF_ENTRY

#include BOOST_PP_LOCAL_ITERATE()

      JsArgument argv[] = {EASY_DISP_ARGV_TABLE()};

      //then, extract them from context
      context->GetArguments(EASY_DISP_NUM_ARGS, argv);

      // Context will throw an exception if required arguments are missing or
      // argument types are mismatched. Check for this and return if exception has
      // been thrown.
      if (context->is_exception_set()) return;

      //now, we've got to handle got damn scoped_refptr as_out_parameter
#define BOOST_PP_LOCAL_LIMITS (0, BOOST_PP_DEC(EASY_DISP_NUM_ARGS))
#define BOOST_PP_LOCAL_MACRO EASY_DISP_REFS_ENTRY

#include BOOST_PP_LOCAL_ITERATE()

#endif
      invoke_helper<RT>(act_func, pActThis, context
              EASY_DISP_COMMA EASY_DISP_FUNCTION_ARGS);
    }
};



#undef EASY_CONST_FUNCTION_QUALIFIER
#undef EASY_DISP_NUM_ARGS
#undef EASY_DISP_COMMA
#undef EASY_DISP_HELPER_FUNCTION_PARMS_HELPER
#undef EASY_DISP_TEMPLATE_PARMS
#undef EASY_DISP_TEMPLATE_ARGS
#undef EASY_DISP_FUNCTION_ARGS
#undef EASY_DISP_HELPER_FUNCTION_PARMS
#undef EASY_DISP_ARGV_TABLE
#undef EASY_DISP_ARG_DEF_ENTRY
#undef EASY_DISP_REFS_ENTRY

