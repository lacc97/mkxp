#include "binding.h"
#include "binding-util.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "filesystem.h"
#include "util.h"
#include "sdl-util.h"
#include "debugwriter.h"
#include "graphics.h"
#include "audio.h"
#include "boost-hash.h"
#include "exception.h"

#include <ruby.h>
#include <ruby/encoding.h>

#include <cassert>
#include <cstdlib>

#include <vector>
#include <string>
#include <sstream>
#include <zlib.h>

#include <SDL_filesystem.h>

#include "binding-ffi.rb.xxd"
#include "module_rpg1.rb.xxd"
#include "module_rpg2.rb.xxd"
#include "module_rpg3.rb.xxd"
#include "run-mkxp.rb.xxd"

struct BacktraceData {
    BoostHash<std::string, std::string> scriptNames;
};

extern "C" {
    void mkxpRgssMain() {
    }

    void mkxpMsgboxString(const char* str) {
        shState->eThread().showMessageBox(str);
    }

    int mkxpSDLReadFile(char** dest, const char* src) {
        SDL_RWops* f = RWFromFile(src, "rb");

        if(!f)
            return 0;

        long size = SDL_RWsize(f);
        *dest = reinterpret_cast<char*>(malloc(size));

        SDL_RWread(f, *dest, 1, size);
        SDL_RWclose(f);

        return 1;
    }

    int mkxpScriptDecode(unsigned char** dest, unsigned char* src, int length) {
        int result = Z_OK;
        unsigned long bufferLength = 0x1000;
        unsigned char* buffer = reinterpret_cast<unsigned char*>(malloc(bufferLength));

        *dest = NULL;

        while(true) {
            unsigned long bl = bufferLength;
            result = uncompress(buffer, &bl, src, length);
            buffer[bl] = '\0';

            if(result != Z_BUF_ERROR)
                break;

            bufferLength *= 2;
            buffer = reinterpret_cast<unsigned char*>(realloc(buffer, bufferLength));
        }

        if(result != Z_OK) {
            free(buffer);

            return 0;
        }

        *dest = buffer;

        return 1;
    }

    void mkxpScriptBacktraceInsert(void* ptr, const char* buf, const char* name) {
        reinterpret_cast<BacktraceData*>(ptr)->scriptNames.insert(buf, name);
    }

    void mkxpProcessReset() {
        shState->graphics().reset();
        shState->audio().reset();

        shState->rtData().rqReset.clear();
        shState->graphics().repaintWait(shState->rtData().rqResetFinish, false);
    }


    void* mkxpStringVectorHelperNew() {
        return new std::vector<std::string>();
    }

    void mkxpStringVectorHelperPushBack(void* ptr, const char* str) {
        return reinterpret_cast<std::vector<std::string>*>(ptr)->push_back(str);
    }

    std::size_t mkxpStringVectorHelperSize(void* ptr) {
        return reinterpret_cast<std::vector<std::string>*>(ptr)->size();
    }

    const char* mkxpStringVectorHelperAt(void* ptr, std::size_t index) {
        return reinterpret_cast<std::vector<std::string>*>(ptr)->at(index).c_str();
    }
}

void showMsg(const std::string& str) {
    shState->eThread().showMessageBox(str.c_str());
}

enum RbException {
    RGSS = 0,
    Reset,
    PHYSFS,
    SDL,
    MKXP,

    ErrnoENOENT,

    IOError,

    TypeError,
    ArgumentError,

    RbExceptionsMax
};

struct {
    RbException id;
    const char* name;
} static customExc[] = {
    { MKXP,   "MKXPError"   },
    { PHYSFS, "PHYSFSError" },
    { SDL,    "SDLError"    }
};

/* Indexed with Exception::Type */
static const RbException excToRbExc[] = {
    RGSS,        /* RGSSError   */
    ErrnoENOENT, /* NoFileError */
    IOError,

    TypeError,
    ArgumentError,

    PHYSFS,      /* PHYSFSError */
    SDL,         /* SDLError    */
    MKXP         /* MKXPError   */
};

struct RbData {
    VALUE exc[RbExceptionsMax];

    /* Input module (RGSS3) */
    VALUE buttoncodeHash;

    RbData() {
        for(size_t i = 0; i < ARRAY_SIZE(customExc); ++i)
            exc[customExc[i].id] = rb_define_class(customExc[i].name, rb_eException);

        exc[RGSS]  = rb_define_class("RGSSError", rb_eStandardError);
        exc[Reset] = rb_define_class(rgssVer >= 3 ? "RGSSReset" : "Reset", rb_eException);

        exc[ErrnoENOENT] = rb_const_get(rb_const_get(rb_cObject, rb_intern("Errno")), rb_intern("ENOENT"));
        exc[IOError] = rb_eIOError;
        exc[TypeError] = rb_eTypeError;
        exc[ArgumentError] = rb_eArgError;
    }
    ~RbData() {}
};

static VALUE newStringUTF8(const char* string, long length) {
    return rb_enc_str_new(string, length, rb_utf8_encoding());
}

RbData* getRbData() {
    return static_cast<RbData*>(shState->bindingData());
}

void raiseRbExc(const Exception& exc) {
    RbData* data = getRbData();
    VALUE excClass = data->exc[excToRbExc[exc.type]];

    rb_raise(excClass, "%s", exc.msg.c_str());
}

#define RBOOL(boolean) ((boolean) ? "true" : "false")

static void showExc(VALUE exc, const BacktraceData& btData) {
    VALUE bt = rb_funcall2(exc, rb_intern("backtrace"), 0, NULL);
    VALUE msg = rb_funcall2(exc, rb_intern("message"), 0, NULL);
    VALUE bt0 = rb_ary_entry(bt, 0);
    VALUE name = rb_class_path(rb_obj_class(exc));

    VALUE ds = rb_sprintf("%" PRIsVALUE ": %" PRIsVALUE " (%" PRIsVALUE ")",
                          bt0, exc, name);

    /* omit "useless" last entry (from ruby:1:in `eval') */
    for(long i = 1, btlen = RARRAY_LEN(bt) - 1; i < btlen; ++i)
        rb_str_catf(ds, "\n\tfrom %" PRIsVALUE, rb_ary_entry(bt, i));

    Debug() << StringValueCStr(ds);

    char* s = RSTRING_PTR(bt0);

    char line[16];
    std::string file(512, '\0');

    char* p = s + strlen(s);
    char* e;

    while(p != s)
        if(*--p == ':')
            break;

    e = p;

    while(p != s)
        if(*--p == ':')
            break;

    /* s         p  e
     * SectionXXX:YY: in 'blabla' */

    *e = '\0';
    strncpy(line, *p ? p + 1 : p, sizeof(line));
    line[sizeof(line) - 1] = '\0';
    *e = ':';
    e = p;

    /* s         e
     * SectionXXX:YY: in 'blabla' */

    *e = '\0';
    strncpy(&file[0], s, file.size());
    *e = ':';

    /* Shrink to fit */
    file.resize(strlen(file.c_str()));
    file = btData.scriptNames.value(file, file);

    std::string ms(640, '\0');
    snprintf(&ms[0], ms.size(), "Script '%s' line %s: %s occured.\n\n%s",
             file.c_str(), line, RSTRING_PTR(name), RSTRING_PTR(msg));

    showMsg(ms);
}

static void ffiBindingExecute() {
    /* Normally only a ruby executable would do a sysinit,
     * but not doing it will lead to crashes due to closed
     * stdio streams on some platforms (eg. Windows) */
    int argc = 0;
    char** argv = 0;
    ruby_sysinit(&argc, &argv);

    ruby_setup();
    ruby_init_loadpath();
    rb_enc_set_default_external(rb_enc_from_encoding(rb_utf8_encoding()));

    Config& conf = shState->rtData().config;

    RbData rbData;
    shState->setBindingData(&rbData);
    BacktraceData btData;

    if(!conf.rubyLoadpaths.empty()) {
        /* Setup custom load paths */
        VALUE lpaths = rb_gv_get(":");

        for(size_t i = 0; i < conf.rubyLoadpaths.size(); ++i) {
            std::string& path = conf.rubyLoadpaths[i];

            VALUE pathv = rb_str_new(path.c_str(), path.size());
            rb_ary_push(lpaths, pathv);
        }
    }

    int state;

    rb_eval_string_protect(reinterpret_cast<const char*>(binding_ffi_binding_ffi_rb), &state);

    if(state) {
        VALUE exc = rb_errinfo();

        if(!NIL_P(exc) && !rb_obj_is_kind_of(exc, rb_eSystemExit))
            showExc(exc, btData);

        return;
    }

    Font::setDefaultColor(FFIFont::default_color);
    Font::setDefaultOutColor(FFIFont::default_outcolor);

    if(rgssVer == 1)
        rb_eval_string_protect(module_rpg1, &state);
    else if(rgssVer == 2)
        rb_eval_string_protect(module_rpg2, &state);
    else if(rgssVer == 3) {
        rb_define_global_const("RGSS_VERSION", rb_str_new_cstr("3.0.1"));
        rb_eval_string_protect(module_rpg3, &state);
    } else
        assert(!"unreachable");

    if(state) {
        VALUE exc = rb_errinfo();

        if(!NIL_P(exc) && !rb_obj_is_kind_of(exc, rb_eSystemExit))
            showExc(exc, btData);

        return;
    }

    std::stringstream preamble;
    preamble << "module MkxpData" << std::endl;
    preamble << "  RGSSVersion = " << rgssVer << std::endl;
    preamble << "  ScriptPack = %q|" << conf.game.scripts  << '|' << std::endl;
    preamble << "  PreloadedScripts = [";

    for(std::set<std::string>::iterator i = conf.preloadScripts.begin(); i != conf.preloadScripts.end(); i++) {
        preamble << "%q|" << (*i) << '|';

        if(i != (--conf.preloadScripts.end())) preamble << ", ";
    }

    preamble << "]" << std::endl;
    preamble << "  BacktraceData = FFI::Pointer.new(:void, " << uint64_t(&btData) << ")" << std::endl;
    preamble << "  module Config" << std::endl;
    preamble << "    CustomScript = %q|" << conf.customScript << '|' << std::endl;
    preamble << "    UseScriptNames = " << RBOOL(conf.useScriptNames) << std::endl;
    preamble << "  end" << std::endl;
    preamble << "end" << std::endl;

    rb_eval_string_protect(preamble.str().c_str(), &state);

    if(state) {
        VALUE exc = rb_errinfo();

        if(!NIL_P(exc) && !rb_obj_is_kind_of(exc, rb_eSystemExit))
            showExc(exc, btData);

        return;
    }

    rb_eval_string_protect(reinterpret_cast<const char*>(binding_ffi_run_mkxp_rb), &state);

    if(state) {
        VALUE exc = rb_errinfo();

        if(!NIL_P(exc) && !rb_obj_is_kind_of(exc, rb_eSystemExit))
            showExc(exc, btData);

        return;
    }

    ruby_cleanup(0);

    shState->rtData().rqTermAck.set();
}

static void ffiBindingTerminate() {
    rb_raise(rb_eSystemExit, " ");
}

static void ffiBindingReset() {
    rb_raise(getRbData()->exc[Reset], " ");
}

ScriptBinding scriptBindingImpl = {
    ffiBindingExecute,
    ffiBindingTerminate,
    ffiBindingReset
};

ScriptBinding* scriptBinding = &scriptBindingImpl;
