# add_library(binding-ffi SHARED
# 		binding-ffi.cpp)
# 	target_include_directories(binding-ffi PRIVATE
# 		../src)

list(APPEND DEFINES
	BINDING_FFI
)

# set(BINDING_HEADERS
# 	binding-mruby/binding-util.h
# 	binding-mruby/disposable-binding.h
# 	binding-mruby/flashable-binding.h
# 	binding-mruby/binding-types.h
# 	binding-mruby/sceneelement-binding.h
# 	binding-mruby/viewportelement-binding.h
# 	binding-mruby/serializable-binding.h
# 	binding-mruby/mrb-ext/file.h
# 	binding-mruby/mrb-ext/rwmem.h
# 	binding-mruby/mrb-ext/marshal.h
# )
set(BINDING_SOURCE
	binding-ffi/binding-ffi.cpp
	binding-ffi/bitmap-binding.cpp
	binding-ffi/disposable-binding.cpp
)
