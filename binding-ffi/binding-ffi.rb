require "rubygems"
require "ffi"

module LibC 
  extend FFI::Library
  ffi_lib FFI::Library::LIBC
  
  # memory allocators
  attach_function :malloc, [:size_t], :pointer
  attach_function :calloc, [:size_t], :pointer
  attach_function :valloc, [:size_t], :pointer
  attach_function :realloc, [:pointer, :size_t], :pointer
  attach_function :free, [:pointer], :void
  
  # memory movers
  attach_function :memcpy, [:pointer, :pointer, :size_t], :pointer
  attach_function :bcopy, [:pointer, :pointer, :size_t], :void
end

def _mkxp_clamp(x, a, b)
  [a, x, b].sort[1]
end

class FFICParam
  attr_accessor :type, :name
  
  def initialize(t, n = nil)
    @type = t
    @name = n
  end
  
  def return_type
    (type.is_a?(FFI::Struct) ? type.by_ref : type)
  end
  
  def argument_type
    (type.is_a?(FFI::Struct) ? type.by_value : type)
  end
  
  def self.param_array(types)
    types.map do |t|
      FFICParam.new t
    end
  end
  
  def self.c_params(paramarray)
    paramarray.map do |param|
      param.argument_type
    end
  end
  def self.r_params(paramarray)
    retarray = []
    paramarray.length.times do |i|
      retarray.push :"arg#{i}"
    end
    retarray
  end
end

class FFICMethod
  attr_accessor :moduleContext, :namespaceName, :klass, :cMethodName, :rMethodName, :methodParams, :retParam
  
  def initialize(mc, nn, cn, cmn, rmn, mp, rp)
    @moduleContext = mc
    @namespaceName = nn
    @klass = cn
    @cMethodName = cmn
    @rMethodName = rmn
    @methodParams = FFICParam.param_array mp
    @retParam = FFICParam.new rp
  end
  
  def cName
    :"#{namespaceName}#{klass.name.split("::").at(-1)}#{cMethodName}"
  end
  def rName
    rMethodName
  end
  
  def bindC
    me = self
    moduleContext.module_exec do
      attach_function me.cName, FFICParam.c_params(me.methodParams), me.retParam.return_type
    end
  end
  
  def bindRuby
    me = self
    moduleContext.module_exec do
      me.klass.class_exec do
        define_method(me.rName, &Proc.new)
      end
    end
  end
end

class FFICStaticMethod < FFICMethod
  def bindC
    me = self
    moduleContext.module_exec do
      attach_function me.cName, FFICParam.c_params(me.methodParams), me.retParam.return_type
    end
  end
  
  def bindRuby
    me = self
    moduleContext.module_exec do
      (class << me.klass; self; end).instance_exec do
        define_method(me.rName, &Proc.new)
      end
    end
  end
end

class FFICModule
  attr_accessor :moduleContext, :namespaceName, :klass
  
  def initialize(modc,nn,cn)
    @moduleContext = modc
    @namespaceName = nn
    @klass = cn
    @ptrKlass = Class.new(FFI::Pointer)
  end
  
  def create
    self.instance_eval(&Proc.new)
  end
  
  def ffi_visible
  end
  
  def class_body
  end
  def module_body
  end
  
  def marshal_load
    FFICStaticMethod.new(moduleContext, namespaceName, klass, :Deserialize, :_load, [:pointer, :int], :pointer)
  end
  
  def constructor(ctrtypes)
    FFICMethod.new(moduleContext, namespaceName, klass, :New, :initialize, ctrtypes, :pointer)
  end
  def ruby_constructor
    FFICMethod.new(moduleContext, namespaceName, klass, :New, :initialize, [], :pointer)
  end
  def copy_constructor
    FFICMethod.new(moduleContext, namespaceName, klass, :NewCopy, :initialize_copy, [:pointer], :pointer)
  end
  
  def destructor
    FFICStaticMethod.new(moduleContext, namespaceName, klass, :Delete, :finalize, [:pointer], :void)
  end
  
  def assign
    FFICStaticMethod.new(moduleContext, namespaceName, klass, :Assign, :_do_c_assign, [:pointer, :pointer], :void)
  end
  
  def method(rtype, mname, argtypes)
    FFICMethod.new(moduleContext, namespaceName, klass, c_method_base_name(mname), mname, argtypes.insert(0, :pointer), rtype)
  end
  def ruby_method(mname)
  end
  def static_method(rtype, mname, argtypes)
    FFICStaticMethod.new(moduleContext, namespaceName, klass, c_method_base_name(mname), mname, argtypes, rtype)
  end
  def ruby_static_method(mname)
  end
  
  def property(type, pname)
    [FFICMethod.new(moduleContext, namespaceName, klass, :"Get#{c_method_base_name(pname)}", :"#{pname}", [:pointer], type), FFICMethod.new(moduleContext, namespaceName, klass, :"Set#{c_method_base_name(pname)}", :"#{pname}=", [:pointer, type], :void)]
  end
  def dynamic_property(type, pname)
    [FFICMethod.new(moduleContext, namespaceName, klass, :"Get#{c_method_base_name(pname)}", :"#{pname}", [:pointer], type), FFICMethod.new(moduleContext, namespaceName, klass, :"Set#{c_method_base_name(pname)}", :"#{pname}=", [:pointer, type], :void)]
  end
  
  def static_property(type, pname)
    [FFICStaticMethod.new(moduleContext, namespaceName, klass, :"Get#{c_method_base_name(pname)}", :"#{pname}", [], type), FFICStaticMethod.new(moduleContext, namespaceName, klass, :"Set#{c_method_base_name(pname)}", :"#{pname}=", [type], :void)]
  end
  def dynamic_static_property(type, pname)
    [FFICStaticMethod.new(moduleContext, namespaceName, klass, :"Get#{c_method_base_name(pname)}", :"#{pname}", [], type), FFICStaticMethod.new(moduleContext, namespaceName, klass, :"Set#{c_method_base_name(pname)}", :"#{pname}=", [type], :void)]
  end
  
  private
  def c_method_base_name(mname)
    ("#{mname}".split('_').map{|w| w.capitalize}).join.gsub('?', "")
  end
end

class FFICModuleC < FFICModule
  def marshal_load
      super.bindC
  end
  
  def constructor(ctrtypes)
    super.bindC
  end
  def ruby_constructor
  end
  def copy_constructor
      super.bindC
  end
  
  def destructor
    super.bindC
  end
  
  def assign
    super.bindC
  end
  
  def method(rtype, mname, argtypes)
    super.bindC
  end
  
  def static_method(rtype, mname, argtypes)
    super.bindC
  end
  
  def property(type, pname)
    super.each do |mth|
      mth.bindC
    end
  end
  def dynamic_property(type, pname)
      super[0].bindC
  end
  
  def static_property(type, pname)
    super.each do |mth|
      mth.bindC
    end
  end
  def dynamic_static_property(type, pname)
      super[0].bindC
  end
end

class FFICModuleR < FFICModule
  def ffi_visible
    me = self
    klass.class_exec do
      extend FFI::DataConverter
      
      native_type FFI::Type::POINTER
      
      def self.from_native(value, ctx)
        self.new_ref(value)
      end
      
      def self.to_native(value, ctx)
        (value == nil ? FFI::Pointer::NULL : value.ptr)
      end
    end
  end
  
  def marshal_load
      me = self
      method = super
      method.bindRuby do |str|
        strptr = FFI::MemoryPointer.new(:char, str.length)
        strptr.write_bytes(str)
        
        instance = allocate
        instance.send(:ptr=, me.moduleContext.send(method.cName, strptr, str.length))
        
        instance
      end
  end

  def constructor(ctrtypes)
    me = self
    if block_given?
      super.bindRuby do |*args|
        @ptr = yield(*args)
      end
    else
      method = super
      method.bindRuby do |*args|
        @ptr = moduleContext.send(method.cName, *args)
      end
    end
  end
  def ruby_constructor(&block)
    me = self
    super.bindRuby do |*args|
      @ptr = yield(*args)
    end
  end
  def copy_constructor
    me = self
    method = super
    method.bindRuby do |cp|
      @ptr = moduleContext.send(method.cName, cp.ptr)
    end
  end
  
  def destructor
  end
  
  def assign
    me = self
    method = super
    method.bindRuby do |curr, oth|
      if !curr.is_a?(me.klass) or !oth.is_a?(me.klass)
        raise ArgumentError.new "Arguments must be of class #{me.klass}"
      end
      if curr.ptr.null? or oth.ptr.null?
        raise ArgumentError.new "Arguments cannot be null"
      end
      me.moduleContext.send(method.cName, curr.ptr, oth.ptr)
    end
  end
  
  def class_body(&block)
    me = self
    klass.class_exec &block
  end
  def module_body(&block)
    me = self
    klass.module_exec &block
  end
  
  def method(rtype, mname, argtypes)
    me = self
    method = super
    if block_given?
      method.bindRuby(&Proc.new)
    else
      method.bindRuby do |*args|
        me.moduleContext.send(method.cName, ptr, *args)
      end
    end
  end
  def ruby_method(mname, &block)
    me = self
    moduleContext.module_exec do
      me.klass.class_exec do
        define_method(mname,&block)
      end
    end
  end
  
  def static_method(rtype, mname, argtypes, &block)
    me = self
    method = super
    if block_given?
      method.bindRuby(&Proc.new)
    else
      method.bindRuby do |*args|
        me.moduleContext.send(method.cName, *args)
      end
    end
  end
  def ruby_static_method(mname, &block)
    me = self
    moduleContext.module_exec do
      (class << me.klass; self; end).instance_exec do
        define_method(mname, &block)
      end
    end
  end
  
  def property(type, pname)
    gs = super
    me = self
    
    gs.at(0).bindRuby do
      me.moduleContext.send(gs.at(0).cName, ptr)
    end
    gs.at(1).bindRuby do |value|
      me.moduleContext.send(gs.at(1).cName, ptr, value)
    end
  end
#   def dynamic_property(type, pname)
#     gs = super
#     me = self
#     gs.at(0).bindRuby do
#       instvar = instance_variable_get(:"@#{pname}")
#       if instvar == nil
#         dptr = me.moduleContext.send(gs.at(0).cName, ptr)
#         typeklassc = FFICModule.new(me.moduleContext, me.namespaceName, dptr.class)
#         dptr.send(:ptr=, FFI::AutoPointer.new(dptr.ptr, me.moduleContext.method(:"#{typeklassc.destructor.cName}")))
#         instance_variable_set(:"@#{pname}", dptr)
#         instvar = instance_variable_get(:"@#{pname}")
#       end
#       instvar
#     end
#     gs.at(1).bindRuby do |val|
#       instvar = instance_variable_get(:"@#{pname}")
#       if instvar == nil
#         dptr = me.moduleContext.send(gs.at(0).cName, ptr)
#         typeklassc = FFICModule.new(me.moduleContext, me.namespaceName, dptr.class)
#         dptr.send(:ptr=, FFI::AutoPointer.new(dptr.ptr, me.moduleContext.method(:"#{typeklassc.destructor.cName}")))
#         instance_variable_set(:"@#{pname}", dptr)
#         instvar = instance_variable_get(:"@#{pname}")
#       end
#       instance_variable_set(:"@#{pname}", val)
#     end
#   end
  def dynamic_property(type, pname)
    gs = super
    me = self
    gs.at(0).bindRuby do
      instvar = instance_variable_get(:"@#{pname}")
      puts("#{me.klass.to_s}::#{gs.at(1).rName}:")
      puts("\tinstvar: #{instvar.inspect}")
      if !ptr.is_a?(FFI::AutoPointer)
        instvar = me.moduleContext.send(gs.at(0).cName, ptr)
        puts("\tinstvar <- #{instvar} = #{me.moduleContext}.send(#{gs.at(0).cName}, #{ptr})")
      elsif instvar == nil
        dptr = me.moduleContext.send(gs.at(0).cName, ptr)
        puts("\tdptr <- #{dptr.inspect} = #{me.moduleContext}.send(#{gs.at(0).cName}, #{ptr})")
        typeklassc = FFICModule.new(me.moduleContext, me.namespaceName, dptr.class)
        dptr.send(:ptr=, FFI::AutoPointer.new(dptr.ptr, me.moduleContext.method(:"#{typeklassc.destructor.cName}")))
        instance_variable_set(:"@#{pname}", dptr)
        instvar = instance_variable_get(:"@#{pname}")
      end
      instvar
    end
    gs.at(1).bindRuby do |val|
      instvar = instance_variable_get(:"@#{pname}")
      puts("#{me.klass.to_s}::#{gs.at(1).rName}:")
      puts("\tinstvar: #{instvar.inspect}; val: #{val.inspect}")
      if !ptr.is_a?(FFI::AutoPointer)
        instvar = me.moduleContext.send(gs.at(0).cName, ptr)
        puts("\tinstvar <- #{instvar} = #{me.moduleContext}.send(#{gs.at(0).cName}, #{ptr})")
      elsif instvar == nil
        dptr = me.moduleContext.send(gs.at(0).cName, ptr)
        puts("\tdptr <- #{dptr.inspect} = #{me.moduleContext}.send(#{gs.at(0).cName}, #{ptr})")
        typeklassc = FFICModule.new(me.moduleContext, me.namespaceName, dptr.class)
        dptr.send(:ptr=, FFI::AutoPointer.new(dptr.ptr, me.moduleContext.method(:"#{typeklassc.destructor.cName}")))
        instance_variable_set(:"@#{pname}", dptr)
        instvar = instance_variable_get(:"@#{pname}")
      end
      instvar.class._do_c_assign(instvar, val)
    end
  end
  
  def static_property(type, pname)
    gs = super
    me = self
    
    gs.at(0).bindRuby do
      me.moduleContext.send(gs.at(0).cName)
    end
    gs.at(1).bindRuby do |value|
      me.moduleContext.send(gs.at(1).cName, value)
    end
  end
  def dynamic_static_property(type, pname)
    gs = super
    me = self
    
    gs.at(0).bindRuby do
      instvar = instance_variable_get(:"@@#{pname}")
      puts("instvar: #{instvar.inspect}; val: #{val.inspect}")
      if instvar == nil
        dptr = me.moduleContext.send(gs.at(0).cName, ptr)
        puts("dptr <- #{dptr.inspect} = #{me.moduleContext}.send(#{gs.at(0).cName}, #{ptr})")
        typeklassc = FFICModule.new(me.moduleContext, me.namespaceName, dptr.class)
        dptr.send(:ptr=, FFI::AutoPointer.new(dptr.ptr, me.moduleContext.method(:"#{typeklassc.destructor.cName}")))
        instance_variable_set(:"@@#{pname}", dptr)
        instvar = instance_variable_get(:"@@#{pname}")
      end
      instvar
    end
    gs.at(1).bindRuby do |val|
      instvar = instance_variable_get(:"@@#{pname}")
      puts("instvar: #{instvar.inspect}; val: #{val.inspect}")
      if instvar == nil
        dptr = me.moduleContext.send(gs.at(0).cName, ptr)
        puts("dptr <- #{dptr.inspect} = #{me.moduleContext}.send(#{gs.at(0).cName}, #{ptr})")
        typeklassc = FFICModule.new(me.moduleContext, me.namespaceName, dptr.class)
        dptr.send(:ptr=, FFI::AutoPointer.new(dptr.ptr, me.moduleContext.method(:"#{typeklassc.destructor.cName}")))
        instance_variable_set(:"@@#{pname}", dptr)
        instvar = instance_variable_get(:"@@#{pname}")
      end
      instvar.class._do_c_assign(instvar, val)
    end
  end
end

def class_binding(klass, parent = Object)
  mymod = self
  self.const_set(klass, Class.new(parent))
  klass = self.const_get(klass)
  
  if !klass.method_defined?(:ptr)
    klass.instance_exec do
      attr_accessor :ptr
      protected :ptr=
    end
  end
  
  klass.instance_exec do
    def self.new_ref(pointer)
      instance = allocate

      instance.send(:ptr=, pointer)

      instance
    end
  end
  
  cbind = FFICModuleC.new(self, :mkxp, klass)
  rbind = FFICModuleR.new(self, :mkxp, klass)
  
  cbind.create(&Proc.new)
  rbind.create(&Proc.new)
end

def module_binding(modulen)
  self.const_set(modulen, Module.new)
  modulen = self.const_get(modulen)
  
  cbind = FFICModuleC.new(self, :mkxp, modulen)
  rbind = FFICModuleR.new(self, :mkxp, modulen)
  
  cbind.create(&Proc.new)
  rbind.create(&Proc.new)
end
  

module MkxpBinding
  extend FFI::Library
  
  ffi_lib FFI::CURRENT_PROCESS
  
  module FFIBoolean
    extend FFI::DataConverter
    
    native_type FFI::Type::INT
    
    def self.from_native(value, ctx)
      (value != 0 ? true : false)
    end
    
    def self.to_native(value, ctx)
      (value == true ? 1 : 0)
    end
  end
  typedef FFIBoolean, :bool
  
  attach_function :mkxpRgssMain, [], :void
  attach_function :mkxpMsgboxString, [:string], :void
  attach_function :mkxpStdCout, [:string], :void
  
  attach_function :mkxpSDLReadFile, [:pointer, :string], :int
  
  def run_custom_script(filename)
    scriptPtr = FFI::MemoryPointer.new(:pointer)
    if MkxpBinding::mkxpSDLReadFile(scriptPtr, filename.to_s()) == 0 or scriptPtr.read_pointer.null?
      return MkxpBinding::mkxpMsgboxString("Unable to open '" + filename.to_s() + "'")
    end
    
    eval(scriptPtr.read_pointer.read_string)
    
    LibC::free(scriptPtr.read_pointer)
  end
  
  attach_function :mkxpScriptDecode, [:pointer, :pointer, :int], :int
  attach_function :mkxpScriptBacktraceInsert, [:pointer, :string, :string], :void
  attach_function :mkxpProcessReset, [], :void
  
  attach_function :mkxpStringVectorHelperNew, [], :pointer
  attach_function :mkxpStringVectorHelperPushBack, [:pointer, :string], :void
  attach_function :mkxpStringVectorHelperSize, [:pointer], :size_t
  attach_function :mkxpStringVectorHelperAt, [:pointer, :size_t], :string
  
  module StringVector
    extend FFI::DataConverter
    
    native_type FFI::Type::POINTER
    
    def self.from_native(value, ctx)
      if value.null?
        nil
      end
      
      strarray = Array.new(MkxpBinding::mkxpStringVectorHelperSize(value))
      strarray.length.times do |i|
        strarray[i] = MkxpBinding::mkxpStringVectorHelperAt(value, i)
      end
      strarray
    end
    
    def self.to_native(value, ctx)
      strvec = MkxpBinding::mkxpStringVectorHelperNew()
      
      if strvec.null?
        return nil
      end
      
      if value.is_a?(Array)
        value.each do |obj|
          MkxpBinding::mkxpStringVectorHelperPushBack(strvec, obj.to_s())
        end
      elsif value != nil
        MkxpBinding::mkxpStringVectorHelperPushBack(strvec, value.to_s())
      end
      
      strvec
    end
  end
  
  class_binding :Disposable do
    method :void, :dispose, []
    method :bool, :"disposed?", []
  end
  
  
  attach_function :mkxpSerializableSerialize, [:pointer, :pointer], :void
  attach_function :mkxpSerializableSerialSize, [:pointer], :int
  class_binding :Serializable do
    ruby_method :_dump do |level|
      strptrlen = MkxpBinding::mkxpSerializableSerialSize(ptr)
      strptr = FFI::MemoryPointer.new(:char, strptrlen)
      MkxpBinding::mkxpSerializableSerialize(ptr, strptr)
      strptr.read_string(strptrlen)
    end
  end
  
  class_binding :Color, Serializable do
    ffi_visible
    
    marshal_load
    
    constructor [:float, :float, :float, :float] do |*args|
      if args.length == 3 or args.length == 4
        r = args[0]
        g = args[1]
        b = args[2]
        a = (args.length == 4 ? args[3] : 255)
      elsif args.length == 0
        r = 0
        g = 0
        b = 0
        a = 0
      end
      MkxpBinding::mkxpColorNew(r, g, b, a)
    end
    destructor
    
    assign
    
    property :float, :red
    property :float, :green
    property :float, :blue
    property :float, :alpha
    
    ruby_method :set do |*args|
      if args.length == 1
        red = args[0].red
        green = args[0].green
        blue = args[0].blue
        alpha = args[0].alpha
      elsif args.length == 3 or args.length == 4
        red = args[0]
        green = args[1]
        blue = args[2]
        alpha = (args.length == 4 ? args[3] : 255)
      elsif args.length == 0
        red = 0
        green = 0
        blue = 0
        alpha = 0
      end
    end
  end
  
  class_binding :Rect, Serializable do
    ffi_visible
    
    marshal_load
    
    constructor [:int, :int, :int, :int] do |*args|
      if args.length == 1
        r = args[0].red
        g = args[0].green
        b = args[0].blue
        a = args[0].alpha
      elsif args.length == 3 or args.length == 4
        r = args[0]
        g = args[1]
        b = args[2]
        a = (args.length == 4 ? args[3] : 255)
      elsif args.length == 0
        r = 0
        g = 0
        b = 0
        a = 0
      end
      MkxpBinding::mkxpRectNew(r, g, b, a)
    end
    destructor
    
    assign
    
    property :int, :x
    property :int, :y
    property :int, :width
    property :int, :height
    
    ruby_method :set do |*args|
      if args.length == 1
        x = args[0].x
        y = args[0].y
        width = args[0].width
        height = args[0].height
      elsif args.length == 3 or args.length == 4
        x = args[0]
        y = args[1]
        width = args[2]
        height = (args.length == 4 ? args[3] : 0)
      elsif args.length == 0
        x = 0
        y = 0
        width = 0
        height = 0
      end
    end
    
    ruby_method :empty do
        set
    end
  end
  
  class_binding :Tone, Serializable do
    ffi_visible
    
    marshal_load
    
    constructor [:float, :float, :float, :float] do |*args|
      if args.length == 1
        r = args[0].red
        g = args[0].green
        b = args[0].blue
        gr = args[0].gray
      elsif args.length == 3 or args.length == 4
        r = args[0]
        g = args[1]
        b = args[2]
        gr = (args.length == 4 ? args[3] : 255)
      elsif args.length == 0
        r = 0
        g = 0
        b = 0
        gr = 0
      end
      MkxpBinding::mkxpToneNew(r, g, b, gr)
    end
    destructor
    
    assign
    
    property :float, :red
    property :float, :green
    property :float, :blue
    property :float, :gray
    
    ruby_method :set do |*args|
      if args.length == 1
        red = args[0].red
        green = args[0].green
        blue = args[0].blue
        gray = args[0].gray
      elsif args.length == 3 or args.length == 4
        red = args[0]
        green = args[1]
        blue = args[2]
        gray = (args.length == 4 ? args[3] : 0)
      elsif args.length == 0
        red = 0
        green = 0
        blue = 0
        gray = 0
      end
    end
  end
  
  attach_function :mkxpFileExists, [:string], :int
  
  attach_function :mkxpIOInitialize, [:string], :pointer
  attach_function :mkxpIORead, [:pointer, :pointer, :int], :pointer
  attach_function :mkxpIOLengthToEnd, [:pointer], :int
  attach_function :mkxpIOReadByte, [:pointer, :pointer], :uint8
  attach_function :mkxpIOClose, [:pointer], :void
  class FFIIO < IO
    attr_accessor :streamPtr, :successPtr
    
    def initialize(filename)
      @successPtr = FFI::MemoryPointer.new(:int)
      @streamPtr = MkxpBinding::mkxpIOInitialize(filename.to_s())
      ObjectSpace.define_finalizer(self, FFIIO.finalize(streamPtr, successPtr))
      
      successPtr.write_int(0)
    end
  
    def read(length = nil, buffer = nil)
      if length == nil or length == -1
        length = MkxpBinding::mkxpIOLengthToEnd(streamPtr)
      end
      readptr = MkxpBinding::mkxpIORead(streamPtr, successPtr, length)
      
      if successPtr.read_int == 0
        LibC::free(readptr)
        return nil
      end
      if readptr.null?
        successPtr.write_int(0)
        return String.new
      end
      
      buffer = readptr.read_string(length)
      
      successPtr.write_int(0)
      LibC::free(readptr)
      
      buffer
    end
    
    def getbyte
      byte = MkxpBinding::mkxpIOReadByte(streamPtr, successPtr)
      
      if successPtr.read_int == 0
        byte = nil
      end
      successPtr.write_int(0)
      byte
    end
    
    def binmode
      self
    end
    
    def close
      MkxpBinding::mkxpIOClose(streamPtr)
      streamPtr = FFI::Pointer::NULL
    end
    
    def self.exists?(filename)
      MkxpBinding::mkxpFileExists(filename.to_s()) != 0
    end
    
    def self.finalize(streamPointer, successPointer)
      proc {
        MkxpBinding::mkxpIOClose(streamPointer)
      }
    end
  end
  
  attach_function :mkxpTableGet, [:pointer, :int, :int, :int], :int
  attach_function :mkxpTableSet, [:pointer, :int, :int, :int, :int], :void
  class_binding :Table, Serializable do
    ffi_visible
    
    marshal_load
    
    constructor [:int, :int, :int] do |*args|
      x = args[0]
      y = 1
      z = 1
      if args.length == 2 or args.length == 3
        y = args[1]
        if args.length == 3
          z = args[2]
        end
      end
      MkxpBinding::mkxpTableNew(x, y, z)
    end
    copy_constructor
    destructor
    
    method :void, :resize, [:int, :int, :int]
    method :int, :xsize, []
    method :int, :ysize, []
    method :int, :zsize, []
    ruby_method :"[]" do |*args|
      x = args[0]
      y = 0
      z = 0
      if args.length == 2 or args.length == 3
        y = args[1]
        if args.length == 3
          z = args[2]
        end
      end
      MkxpBinding::mkxpTableGet(ptr, x, y, z)
    end
    ruby_method :"[]=" do |*args|
      x = args[0]
      y = 0
      z = 0
      if args.length == 3 or args.length == 4
        y = args[1]
        if args.length == 4
          z = args[2]
        end
      end
      MkxpBinding::mkxpTableSet(ptr, x, y, z, args[3])
    end
        
  end
  
  attach_function :mkxpFontSetName, [:pointer, StringVector], :void
  attach_function :mkxpFontSetDefaultName, [StringVector], :void
  class_binding :Font, Disposable do
    ffi_visible
    
    class_body do
      def name
        @name
      end
      
      def name= n
        @name = n
        MkxpBinding::mkxpFontSetName(ptr, n)
      end
      
      def self.default_name
        @@default_name
      end
      
      def self.default_name= dn
        @@default_name = dn
        MkxpBinding::mkxpFontSetDefaultName(dn)
      end
    end
    
    constructor [StringVector, :int] do |*args|
      @name = args.length >= 1 ? args[0] : nil
      p = MkxpBinding::mkxpFontNew(@name, args.length == 2 ? args[1] : 0)
#       @color = MkxpBinding::mkxpFontGetColor(p)
#       @color.ptr = FFI::AutoPointer.new(@color.ptr, MkxpBinding.method(:mkxpColorDelete))
#       @out_color = MkxpBinding::mkxpFontGetOutColor(p)
#       @out_color.ptr = FFI::AutoPointer.new(@color.ptr, MkxpBinding.method(:mkxpColorDelete))
      p
    end
    copy_constructor
    destructor
    
    property :int, :size
    property :bool, :bold
    property :bool, :italic
    property :bool, :outline
    property :bool, :shadow
    dynamic_property Color, :color
    dynamic_property Color, :out_color
    
    static_property :int, :default_size
    static_property :bool, :default_bold
    static_property :bool, :default_italic
    static_property :bool, :default_outline
    static_property :bool, :default_shadow
    dynamic_static_property Color, :default_color
    dynamic_static_property Color, :default_out_color
    static_method :bool, :exists?, [:string]
  end
  
  attach_function :mkxpBitmapInitializeFilename, [:string], :pointer
  attach_function :mkxpBitmapInitializeExtent, [:int, :int], :pointer
  class_binding :Bitmap, Disposable do
    ffi_visible
    
    ruby_constructor do |*args|
      if args.length == 1
        MkxpBinding::mkxpBitmapInitializeFilename(args[0])
      elsif args.length == 2
        MkxpBinding::mkxpBitmapInitializeExtent(args[0], args[1])
      end
    end
    destructor
    
    assign
    
    dynamic_property Font, :font
    
    method :int, :width, []
    method :int, :height, []
    method Rect, :rect, [] do
      rp = MkxpBinding::mkxpBitmapRect()
      rp.send(:ptr=, FFI::AutoPointer(rp.ptr, MkxpBinding.method(mkxpRectDelete)))
      rp
    end
    method :void, :blt, [:int, :int, :pointer, Rect, :int] do |x, y, src_bitmap, src_rect, *args|
      op = 255
      if args.length != 1
        op = args[0]
      end
      MkxpBinding::mkxpBitmapBlt(ptr, x, y, src_bitmap.ptr, src_rect, op)
    end
    method :void, :stretch_blt, [Rect, :pointer, Rect, :int] do |dest_rect, src_bitmap, src_rect, *args|
      op = 255
      if args.length != 1
        op = args[0]
      end
      MkxpBinding::mkxpBitmapStretchBlt(ptr, dest_rect, src_bitmap.ptr, src_rect, op)
    end
    method :void, :fill_rect, [Rect, Color] do |*args|
      drect = args[0]
      col = args[1]
      if args.length == 5
        drect = ::Rect.new(args[0], args[1], args[2], args[3])
        col = args[4]
      end
      MkxpBinding::mkxpBitmapFillRect(ptr, drect, col)
    end
    method :void, :gradient_fill_rect, [Rect, Color, Color, :bool] do |*args|
      drect = args[0]
      color1 = args[1]
      color2 = args[2]
      vertical = false
      if args.length == 4
        vertical = args[3]
      end
      if args.length == 6 or args.length == 7
        drect = Rect.new(args[0], args[1], args[2], args[3])
        color1 = args[4]
        color2 = args[5]
        if args.length == 7
          vertical = args[6]
        end
      end
      MkxpBinding::mkxpBitmapFillRect(ptr, drect, color1, color2, vertical)
    end
    method :void, :clear, []
    method :void, :clear_rect, [Rect] do |*args|
      r = args[0]
      if args.length == 4
        r = ::Rect.new(args[0], args[1], args[2], args[3])
      end
      MkxpBinding::mkxpBitmapClearRect(ptr, r)
    end
    method :void, :get_pixel, [:int, :int, Color] do |x, y|
      col = Color.new
      MkxpBinding::mkxpBitmapGetPixel(ptr, x, y, col)
      col
    end
    method :void, :set_pixel, [:int, :int, Color]
    method :void, :hue_change, [:int]
    method :void, :blur, []
    method :void, :radial_blur, [:int, :int]
    method :void, :draw_text, [Rect, :string, :bool] do |*args|
      MkxpBinding::mkxpMsgboxString(args.to_s + "\nfont: {#{font.name.to_s}, #{font.size}} [#{Font.default_name.to_s}, #{Font.default_size}]")
      r = args[0]
      s = args[1]
      al = 0
      if args.length == 3
        al = args[2]
      end
      if args.length == 5 or args.length == 6
        r = Rect.new(args[0], args[1], args[2], args[3])
        s = args[4]
        if args.length == 6
          al = args[5]
        end
      end
      MkxpBinding::mkxpBitmapDrawText(ptr, r, s, al)
    end
    method Rect, :text_size, [:string] do |txt|
      rp = MkxpBinding::mkxpBitmapTextSize(ptr, txt)
      rp.send(:ptr=, FFI::AutoPointer(rp.ptr, MkxpBinding.method(mkxpRectDelete)))
      rp
    end
  end
  
  class_binding :Viewport, Disposable do
    ffi_visible
    
    constructor [Rect] do |*args|
      if args.length == 0
        r = Rect.new(0,0, -1, -1)
      elsif args.length == 1
        r = args[0] != nil ? args[0] : Rect.new(0,0, -1, -1)
      else
        r = Rect.new(args[0], args[1], args[2], args[3])
      end
      MkxpBinding::mkxpViewportNew(r)
    end
    destructor
    
    dynamic_property Rect, :rect
    property :bool, :visible
    property :int, :z
    property :int, :ox
    property :int, :oy
    dynamic_property Color, :color
    dynamic_property Tone, :tone
    
    method :void, :flash, [Color, :int] do |col, dur|
      MkxpBinding::mkxpViewportFlash(ptr, col, dur)
    end
    method :void, :update, [] 
  end
  
  attach_function :mkxpTilemapGetBitmap, [:pointer, :int], Bitmap
  attach_function :mkxpTilemapSetBitmap, [:pointer, :int, Bitmap], :void
  class_binding :Tilemap, Disposable do
    ffi_visible
    
    constructor [Viewport] do |*args|
      vp = nil
      if args.length == 1
        vp = args[0]
      end
      MkxpBinding::mkxpTilemapNew(vp)
    end
    destructor
    
    property Table, :map_data
    property Table, :flash_data
    property Table, :flags
    property Viewport, :viewport
    property :bool, :visible
    property :int, :ox
    property :int, :oy
    
    method :void, :update, []
    ruby_method :"bitmaps[]" do |index|
      MkxpBinding::mkxpTilemapGetBitmap(ptr, index)
    end
    ruby_method :"bitmaps[]=" do |index, bm|
      MkxpBinding::mkxpTilemapSetBitmap(ptr, index, bm)
    end
  end
  
  class_binding :Plane, Disposable do
    ffi_visible
    
    constructor [Viewport] do |*args|
      vp = nil
      if args.length == 1
        vp = args[0]
      end
      MkxpBinding::mkxpPlaneNew(vp)
    end
    destructor
    
    property Bitmap, :bitmap
    property Viewport, :viewport
    property :bool, :visible
    property :int, :z
    property :int, :ox
    property :int, :oy
    property :float, :zoom_x
    property :float, :zoom_y
    property :int, :opacity do |op|
      MkxpBinding::mkxpPlaneOpacity(ptr, _mkxp_clamp(op, 0, 255))
    end
    property :int, :blend_type
    dynamic_property Color, :color
    dynamic_property Tone, :tone
  end
  
  class_binding :Sprite, Disposable do
    ffi_visible
    
    constructor [Viewport] do |*args|
      vp = nil
      if args.length == 1
        vp = args[0]
      end
      MkxpBinding::mkxpSpriteNew(vp)
    end
    destructor
    
    property Bitmap, :bitmap
    dynamic_property Rect, :src_rect
    property Viewport, :viewport
    property :bool, :visible
    property :int, :x
    property :int, :y
    property :int, :z
    property :int, :ox
    property :int, :oy
    property :float, :zoom_x
    property :float, :zoom_y
    property :int, :angle
    property :int, :wave_amp
    property :int, :wave_length
    property :int, :wave_speed
    property :int, :wave_phase
    property :bool, :mirror
    property :int, :bush_depth
    property :int, :bush_opacity
    property :int, :opacity
    property :int, :blend_type
    dynamic_property Color, :color
    dynamic_property Tone, :tone
    
    method :void, :flash, [Color, :int] do |col, dur|
      MkxpBinding::mkxpSpriteFlash(ptr, col, dur)
    end
    method :void, :update, []
    method :int, :width, []
    method :int, :height, []
  end
  
  class_binding :Window, Disposable do
    ffi_visible
    
    constructor [Viewport, :int, :int, :int, :int] do |*args|
      vp = nil
      ix = 0
      iy = 0
      iw = 0
      ih = 0
      if args.length == 1
        vp = args[0]
      elsif args.length == 4
        ix = args[0]
        iy = args[1]
        iw = args[2]
        ih = args[3]
      end
      MkxpBinding::mkxpWindowNew(vp, ix, iy, iw, ih)
    end
    destructor
    
    property Bitmap, :windowskin
    property Bitmap, :contents
    dynamic_property Rect, :cursor_rect
    property Viewport, :viewport
    property :bool, :active
    property :bool, :visible
    property :bool, :arrows_visible
    property :bool, :pause
    property :int, :x
    property :int, :y
    property :int, :z
    property :int, :width
    property :int, :height
    property :int, :ox
    property :int, :oy
    property :int, :padding
    property :int, :padding_bottom
    property :int, :opacity
    property :int, :back_opacity
    property :int, :contents_opacity
    property :int, :openness
    property :bool, :stretch
    dynamic_property Tone, :tone
    
    method :void, :update, []
    method :void, :move, [:int, :int, :int, :int]
    method :bool, :"open?", []
    method :bool, :"close?", []
  end
  
  module_binding :Audio do
    static_method :void, :setup_midi, []
    static_method :void, :bgm_play, [:string, :int, :int, :float] do |*args|
      filename = args[0]
      volume = 100
      pitch = 100
      pos = 0.0
      if args.length == 2 or args.length == 3 or args.length == 4
        volume = args[1]
        if args.length == 3 or args.length == 4
          pitch = args[2]
          if args.length == 4
            pos = args[3]
          end
        end
      end
      MkxpBinding::mkxpAudioBgmPlay(filename, volume, pitch, pos)
    end
    static_method :void, :bgm_stop, []
    static_method :void, :bgm_fade, [:int]
    static_method :float, :bgm_pos, []
    static_method :void, :bgs_play, [:string, :int, :int, :float] do |*args|
      filename = args[0]
      volume = 100
      pitch = 100
      pos = 0.0
      if args.length == 2 or args.length == 3 or args.length == 4
        volume = args[1]
        if args.length == 3 or args.length == 4
          pitch = args[2]
          if args.length == 4
            pos = args[3]
          end
        end
      end
      MkxpBinding::mkxpAudioBgsPlay(filename, volume, pitch, pos)
    end
    static_method :void, :bgs_stop, []
    static_method :void, :bgs_fade, [:int]
    static_method :float, :bgs_pos, []
    static_method :void, :me_play, [:string, :int, :int] do |*args|
      filename = args[0]
      volume = 100
      pitch = 100
      pos = 0.0
      if args.length == 2 or args.length == 3 or args.length == 4
        volume = args[1]
        if args.length == 3 or args.length == 4
          pitch = args[2]
          if args.length == 4
            pos = args[3]
          end
        end
      end
      MkxpBinding::mkxpAudioMePlay(filename, volume, pitch)
    end
    static_method :void, :me_stop, []
    static_method :void, :me_fade, [:int]
    static_method :void, :se_play, [:string, :int, :int] do |*args|
      filename = args[0]
      volume = 100
      pitch = 100
      pos = 0.0
      if args.length == 2 or args.length == 3 or args.length == 4
        volume = args[1]
        if args.length == 3 or args.length == 4
          pitch = args[2]
          if args.length == 4
            pos = args[3]
          end
        end
      end
      MkxpBinding::mkxpAudioSePlay(filename, volume, pitch)
    end
    static_method :void, :se_stop, []
  end
  
  module_binding :Graphics do
    static_property :int, :frame_rate
    static_property :int, :frame_count
    static_property :int, :brightness
    
    static_method :void, :update, []
    static_method :void, :wait, [:int]
    static_method :void, :fadeout, [:int]
    static_method :void, :fadein, [:int]
    static_method :void, :freeze, []
    static_method :void, :transition, [:int,:string,:int] do |*args|
      dur = 10
      fn = ""
      vag = 40
      if args.length == 1 or args.length == 2 or args.length == 3
        dur = args[0]
        if args.length == 2 or args.length == 3
          fn = args[1]
          if args.length == 3
            vag = args[2]
          end
        end
      end
      MkxpBinding::mkxpGraphicsTransition(dur, fn, vag)
    end
    static_method Bitmap, :snap_to_bitmap, []
    static_method :void, :frame_reset, []
    static_method :int, :width, [] do
        i = MkxpBinding::mkxpGraphicsWidth()
#         MkxpBinding::mkxpMsgboxString("Graphics.width = #{i}")
        i
    end
    static_method :int, :height, []
    static_method :void, :resize_screen, [:int, :int]
    static_method :void, :play_movie, [:string]
  end
  
  module_binding :Input do
    module_body do
      DOWN = :DOWN
      LEFT = :LEFT
      RIGHT = :RIGHT
      UP = :RIGHT
      A = :A
      B = :B
      C = :C
      X = :X
      Y = :Y
      Z = :Z
      L = :L
      R = :R
      SHIFT = :SHIFT
      CTRL = :CTRL
      ALT = :ALT
      F5 = :F5
      F6 = :F6
      F7 = :F7
      F8 = :F8
      F9 = :F9
    end
    
    static_method :void, :update, []
    static_method :bool, :press?, [:string] do |sym|
      MkxpBinding::mkxpInputPress("#{sym}")
    end
    static_method :bool, :trigger?, [:string] do |sym|
      MkxpBinding::mkxpInputTrigger("#{sym}")
    end
    static_method :bool, :repeat?, [:string] do |sym|
      MkxpBinding::mkxpInputRepeat("#{sym}")
    end
    static_method :int, :dir4, []
    static_method :int, :dir8, []
  end
end

undef _mkxp_clamp

Color = MkxpBinding::Color
Rect = MkxpBinding::Rect
Tone = MkxpBinding::Tone

Bitmap = MkxpBinding::Bitmap
Font = MkxpBinding::Font
Plane = MkxpBinding::Plane
Sprite = MkxpBinding::Sprite
Table = MkxpBinding::Table
Tilemap = MkxpBinding::Tilemap
Viewport = MkxpBinding::Viewport
Window = MkxpBinding::Window

Audio = MkxpBinding::Audio
Graphics = MkxpBinding::Graphics
Input = MkxpBinding::Input

class RGSSError < StandardError
end

class RGSSReset < Exception
end

def msgbox(arg, *args)
  prntStr = arg.to_s()
  args.each do |arg_item|
    prntStr += arg_item.to_s()
  end
  MkxpBinding::mkxpMsgboxString(prntStr)
end

def msgbox_p(obj, *objs)
  prntStr = obj.inspect.to_s()
  objs.each do |obj_item|
    prntStr += obj_item.inspect.to_s()
  end
  MkxpBinding::mkxpMsgboxString(prntStr)
end

def save_data(obj, filename)
  File.open(filename, "wb") {|f| Marshal.dump(obj, f)}
end

def load_data(filename)
  port = MkxpBinding::FFIIO.new(filename)
  Marshal.load(port)
end

def rgss_stop
  
end

def rgss_main
  yield
end

def puts(arg, *args)
  prntStr = arg.to_s()
  args.each do |arg_item|
    prntStr += arg_item.to_s()
  end
  MkxpBinding::mkxpStdCout(prntStr)
end
