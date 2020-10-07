#include <dlfcn.h>

#include <cassert>

#include <string>

#include <fmt/format.h>

#include <ruby.h>

#include <unicodestring.hpp>

#include "binding-util.h"

namespace {
  inline bool ends_with(unicode::string_view sv, unicode::string_view pattern) noexcept {
    if(pattern.size() > sv.size())
      return false;

    return sv.substr(sv.size() - pattern.size()) == pattern;
  }

  namespace rb {
    using func_type = VALUE (*)(...);

    size_t ary_len(VALUE v) noexcept {
#if RUBY_API_VERSION_MAJOR == 1
      return RARRAY(v)->len;
#else
      return rb_array_len(v);
#endif
    }

    const char* str_cstr(VALUE v) noexcept {
      return StringValueCStr(v);
    }

    size_t str_len(VALUE v) noexcept {
      return RSTRING_LEN(v);
    }

    const char* str_ptr(VALUE v) noexcept {
      return StringValuePtr(v);
    }

    VALUE to_num(int v) noexcept {
      return INT2NUM(v);
    }
    VALUE to_num(unsigned int v) noexcept {
      return UINT2NUM(v);
    }
    VALUE to_num(long v) noexcept {
      return LONG2NUM(v);
    }
    VALUE to_num(unsigned long v) noexcept {
      return ULONG2NUM(v);
    }
    VALUE to_num(long long v) noexcept {
      return LL2NUM(v);
    }
    VALUE to_num(unsigned long long v) noexcept {
      return ULL2NUM(v);
    }

    template <typename T>
    T from_num(VALUE v) noexcept;
    template <>
    int from_num<int>(VALUE v) noexcept {
      return NUM2INT(v);
    }
    template <>
    unsigned int from_num<unsigned int>(VALUE v) noexcept {
      return NUM2UINT(v);
    }
    template <>
    long from_num<long>(VALUE v) noexcept {
      return NUM2LONG(v);
    }
    template <>
    unsigned long from_num<unsigned long>(VALUE v) noexcept {
      return NUM2ULONG(v);
    }
    template <>
    long long from_num<long long>(VALUE v) noexcept {
      return NUM2LL(v);
    }
    template <>
    unsigned long long from_num<unsigned long long>(VALUE v) noexcept {
      return NUM2ULL(v);
    }
  }    // namespace rb

  enum class param_type { eVoid, eNumber, ePointer, eInteger };

  class win32_function {
   public:
    using param_type = long;
    using proc_type = param_type (*)(...);

    struct info {
      bool return_void;
      param_type params[16];
      size_t params_len;
    };

    explicit win32_function(void* proc) noexcept : mp_Proc{reinterpret_cast<proc_type>(proc)} {}

    void call(param_type* ret, const info& inf) {
      switch(inf.params_len) {
        case 0:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type0>(mp_Proc)();
          else
            *ret = reinterpret_cast<actual_proc_type0>(mp_Proc)();
          break;
        case 1:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type1>(mp_Proc)(inf.params[0]);
          else
            *ret = reinterpret_cast<actual_proc_type1>(mp_Proc)(inf.params[0]);
          break;
        case 2:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type2>(mp_Proc)(inf.params[0], inf.params[1]);
          else
            *ret = reinterpret_cast<actual_proc_type2>(mp_Proc)(inf.params[0], inf.params[1]);
          break;
        case 3:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type3>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2]);
          else
            *ret = reinterpret_cast<actual_proc_type3>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2]);
          break;
        case 4:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type4>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2],
                                                              inf.params[3]);
          else
            *ret =
              reinterpret_cast<actual_proc_type4>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2], inf.params[3]);
          break;
        case 5:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type5>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2],
                                                              inf.params[3], inf.params[4]);
          else
            *ret = reinterpret_cast<actual_proc_type5>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2],
                                                                inf.params[3], inf.params[4]);
          break;
        case 6:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type6>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2],
                                                              inf.params[3], inf.params[4], inf.params[5]);
          else
            *ret = reinterpret_cast<actual_proc_type6>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2],
                                                                inf.params[3], inf.params[4], inf.params[5]);
          break;
        case 7:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type7>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6]);
          else
            *ret = reinterpret_cast<actual_proc_type7>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6]);
          break;
        case 8:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type8>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2],
                                                              inf.params[3], inf.params[4], inf.params[5],
                                                              inf.params[6], inf.params[7]);
          else
            *ret =
              reinterpret_cast<actual_proc_type8>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2], inf.params[3],
                                                           inf.params[4], inf.params[5], inf.params[6], inf.params[7]);
          break;
        case 9:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type9>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2],
                                                              inf.params[3], inf.params[4], inf.params[5],
                                                              inf.params[6], inf.params[7], inf.params[8]);
          else
            *ret = reinterpret_cast<actual_proc_type9>(mp_Proc)(inf.params[0], inf.params[1], inf.params[2],
                                                                inf.params[3], inf.params[4], inf.params[5],
                                                                inf.params[6], inf.params[7], inf.params[8]);
          break;
        case 10:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type10>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9]);
          else
            *ret = reinterpret_cast<actual_proc_type10>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9]);
          break;
        case 11:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type11>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10]);
          else
            *ret = reinterpret_cast<actual_proc_type11>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10]);
          break;
        case 12:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type12>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11]);
          else
            *ret = reinterpret_cast<actual_proc_type12>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11]);
          break;
        case 13:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type13>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11], inf.params[12]);
          else
            *ret = reinterpret_cast<actual_proc_type13>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11], inf.params[12]);
          break;
        case 14:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type14>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11], inf.params[12],
              inf.params[13]);
          else
            *ret = reinterpret_cast<actual_proc_type14>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11], inf.params[12],
              inf.params[13]);
          break;
        case 15:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type15>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11], inf.params[12],
              inf.params[13], inf.params[14]);
          else
            *ret = reinterpret_cast<actual_proc_type15>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11], inf.params[12],
              inf.params[13], inf.params[14]);
          break;
        case 16:
          if(inf.return_void)
            reinterpret_cast<actual_void_proc_type16>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11], inf.params[12],
              inf.params[13], inf.params[14], inf.params[15]);
          else
            *ret = reinterpret_cast<actual_proc_type16>(mp_Proc)(
              inf.params[0], inf.params[1], inf.params[2], inf.params[3], inf.params[4], inf.params[5], inf.params[6],
              inf.params[7], inf.params[8], inf.params[9], inf.params[10], inf.params[11], inf.params[12],
              inf.params[13], inf.params[14], inf.params[15]);
          break;
      }
    }

    explicit operator bool() const noexcept {
      return bool(mp_Proc);
    }

   private:
    using actual_void_proc_type0 = void (*)();
    using actual_void_proc_type1 = void (*)(param_type);
    using actual_void_proc_type2 = void (*)(param_type, param_type);
    using actual_void_proc_type3 = void (*)(param_type, param_type, param_type);
    using actual_void_proc_type4 = void (*)(param_type, param_type, param_type, param_type);
    using actual_void_proc_type5 = void (*)(param_type, param_type, param_type, param_type, param_type);
    using actual_void_proc_type6 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type);
    using actual_void_proc_type7 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                            param_type);
    using actual_void_proc_type8 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                            param_type, param_type);
    using actual_void_proc_type9 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                            param_type, param_type, param_type);
    using actual_void_proc_type10 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type, param_type);
    using actual_void_proc_type11 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type, param_type, param_type);
    using actual_void_proc_type12 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type, param_type, param_type, param_type);
    using actual_void_proc_type13 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type);
    using actual_void_proc_type14 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type);
    using actual_void_proc_type15 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type);
    using actual_void_proc_type16 = void (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type, param_type);

    using actual_proc_type0 = param_type (*)();
    using actual_proc_type1 = param_type (*)(param_type);
    using actual_proc_type2 = param_type (*)(param_type, param_type);
    using actual_proc_type3 = param_type (*)(param_type, param_type, param_type);
    using actual_proc_type4 = param_type (*)(param_type, param_type, param_type, param_type);
    using actual_proc_type5 = param_type (*)(param_type, param_type, param_type, param_type, param_type);
    using actual_proc_type6 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type);
    using actual_proc_type7 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type);
    using actual_proc_type8 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type);
    using actual_proc_type9 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                             param_type, param_type, param_type);
    using actual_proc_type10 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type, param_type);
    using actual_proc_type11 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type, param_type, param_type);
    using actual_proc_type12 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type, param_type, param_type, param_type);
    using actual_proc_type13 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type);
    using actual_proc_type14 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type);
    using actual_proc_type15 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type);
    using actual_proc_type16 = param_type (*)(param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type, param_type, param_type, param_type,
                                              param_type, param_type, param_type, param_type);

    proc_type mp_Proc;
  };

  VALUE Win32API_initialize(VALUE self, VALUE dllname, VALUE proc, VALUE param, VALUE ret) {
    SafeStringValue(dllname);
    SafeStringValue(proc);

    bool validDll = false;
    VALUE sym;
    {
      auto dllfile = unicode::toLowerCase(utf16(rb::str_cstr(dllname))).value_or(u"");
      if(ends_with(dllfile, u".dll")) {
        validDll = true;
        unicode::string_view dllbasename = dllfile;

        std::string s = fmt::format("{0}_{1}", utf8(dllbasename.substr(0, dllbasename.size() - 4)), rb::str_cstr(proc));
        sym = rb_str_new(s.data(), s.length());
      }
    }
    if(!validDll)
      rb_raise(rb_eRuntimeError, "invalid argument: %s is not a dll\n", rb::str_cstr(dllname));
    void* procAddr = dlsym(RTLD_DEFAULT, rb::str_cstr(sym));
    if(!procAddr)
      procAddr = dlsym(RTLD_DEFAULT, (std::string(rb::str_cstr(sym)) + "A").c_str());
    if(!procAddr)
      rb_raise(rb_eRuntimeError, "dlsym: '%s' or '%sA' not found\n", rb::str_cstr(sym), rb::str_cstr(sym));
    rb_iv_set(self, "__procname__", sym);
    rb_iv_set(self, "__proc__", rb::to_num(reinterpret_cast<uintptr_t>(procAddr)));

    VALUE paramsType = rb_ary_new();
    switch(rb_type(param)) {
      case T_NIL:
        break;
      case T_ARRAY: {
        for(size_t i = 0; i < rb::ary_len(param); i++) {
          VALUE v = rb_ary_entry(param, i);
          SafeStringValue(v);
          switch(*rb::str_cstr(v)) {
            case 'N':
            case 'n':
            case 'L':
            case 'l':
              rb_ary_push(paramsType, INT2FIX(param_type::eNumber));
              break;
            case 'P':
            case 'p':
              rb_ary_push(paramsType, INT2FIX(param_type::ePointer));
              break;
            case 'I':
            case 'i':
              rb_ary_push(paramsType, INT2FIX(param_type::eInteger));
              break;
          }
        }
      } break;
      default: {
        SafeStringValue(param);

        const char* s = rb::str_ptr(param);
        for(size_t i = 0; i < rb::str_len(param); i++) {
          switch(*s++) {
            case 'N':
            case 'n':
            case 'L':
            case 'l':
              rb_ary_push(paramsType, INT2FIX(param_type::eNumber));
              break;
            case 'P':
            case 'p':
              rb_ary_push(paramsType, INT2FIX(param_type::ePointer));
              break;
            case 'I':
            case 'i':
              rb_ary_push(paramsType, INT2FIX(param_type::eInteger));
              break;
          }
        }
      } break;
    }
    if(16 < rb::ary_len(param))
      rb_raise(rb_eRuntimeError, "too many parameters: %ld\n", rb::ary_len(param));
    rb_iv_set(self, "__params__", paramsType);

    param_type retType = param_type::eVoid;
    if(!NIL_P(ret)) {
      SafeStringValue(ret);
      switch(*rb::str_ptr(ret)) {
        case 'V':
        case 'v':
          retType = param_type::eVoid;
          break;
        case 'N':
        case 'n':
        case 'L':
        case 'l':
          retType = param_type::eNumber;
          break;
        case 'P':
        case 'p':
          retType = param_type::ePointer;
          break;
        case 'I':
        case 'i':
          retType = param_type::eInteger;
          break;
      }
    }
    rb_iv_set(self, "__return__", INT2FIX(retType));

    return Qnil;
  }

  VALUE Win32API_call(int argc, VALUE* argv, VALUE obj) {
    VALUE vProc = rb_iv_get(obj, "__proc__");
    VALUE vParamsType = rb_iv_get(obj, "__params__");
    VALUE vRetType = rb_iv_get(obj, "__return__");

    win32_function fn(reinterpret_cast<void*>(rb::from_num<uintptr_t>(vProc)));

    VALUE args;
    size_t numCallArgs = rb_scan_args(argc, argv, "0*", &args);
    size_t numAllowedArgs = rb::ary_len(vParamsType);
    if(numCallArgs != numAllowedArgs)
      rb_raise(rb_eRuntimeError, "wrong number of parameters: expected %d, got %d", numAllowedArgs, numCallArgs);

    win32_function::info inf{};
    inf.return_void = (static_cast<param_type>(FIX2INT(vRetType)) == param_type::eVoid);
    inf.params_len = numAllowedArgs;

    for(size_t i = 0; i < numAllowedArgs; ++i) {
      unsigned long lParam = 0;
      switch(static_cast<param_type>(FIX2INT(rb_ary_entry(vParamsType, i)))) {
        VALUE str;
        case param_type::eNumber:
        case param_type::eInteger:
        default:
          lParam = rb::from_num<win32_function::param_type>(rb_ary_entry(args, i));
          break;
        case param_type::ePointer:
          str = rb_ary_entry(args, i);
          if(NIL_P(str)) {
            lParam = 0;
          } else if(FIXNUM_P(str)) {
            lParam = NUM2ULONG(str);
          } else {
            StringValue(str);
            rb_str_modify(str);
            lParam = (unsigned long) StringValuePtr(str);
          }
          break;
      }
      inf.params[i] = lParam;
    }

    win32_function::param_type r;
    fn.call(&r, inf);

    switch(static_cast<param_type>(FIX2INT(vRetType))) {
      case param_type::eNumber:
      case param_type::eInteger:
        return INT2NUM(r);
      case param_type::ePointer:
        return rb_str_new2((char*) r);
      case param_type::eVoid:
      default:
        return INT2NUM(0);
    }
  }
}    // namespace

void win32BindingInit() {
  VALUE cWin32API = rb_define_class("Win32API", rb_cObject);
  rb_define_method(cWin32API, "initialize", reinterpret_cast<rb::func_type>(&Win32API_initialize), 4);
  rb_define_method(cWin32API, "call", reinterpret_cast<rb::func_type>(&Win32API_call), -1);
  rb_define_alias(cWin32API, "Call", "call");
}