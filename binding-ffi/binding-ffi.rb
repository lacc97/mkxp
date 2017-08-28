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

class Color
	attr_reader :red, :green, :blue, :alpha
	
	def initialize(*args)
		if args.length == 3 or args.length == 4
			@red = _mkxp_clamp(args[0], 0, 255)
			@green = _mkxp_clamp(args[1], 0, 255)
			@blue = _mkxp_clamp(args[2], 0, 255)
			@alpha = (args.length == 4 ? _mkxp_clamp(args[3], 0, 255) : 255)
		elsif args.length == 0
			@red = 0
			@green = 0
			@blue = 0
			@alpha = 0
		else
			raise ArgumentException.new("Wrong number of arguments")
		end
	end
	
	def set(*args)
		if args.length == 3 or args.length == 4
			@red = _mkxp_clamp(args[0], 0, 255)
			@green = _mkxp_clamp(args[1], 0, 255)
			@blue = _mkxp_clamp(args[2], 0, 255)
			@alpha = (args.length == 4 ? _mkxp_clamp(args[3], 0, 255) : 255)
		elsif args.length == 1
			@red = args[0].red
			@green = args[0].green
			@blue = args[0].blue
			@alpha = args[0].alpha
		else
			raise ArgumentException.new("Wrong number of arguments")
		end
	end
	
	def red= r
		@red = _mkxp_clamp(r, 0, 255)
	end
	def green= g
		@green = _mkxp_clamp(g, 0, 255)
	end
	def blue= b
		@blue = _mkxp_clamp(b, 0, 255)
	end
	def alpha= a
		@alpha = _mkxp_clamp(a, 0, 255)
	end
end

class Rect
	attr_accessor :x, :y, :width, :height
	
	def initialize(ix = 0, iy = 0, iw = 0, ih = 0)
		@x = ix
		@y = iy
		@width = iw
		@height = ih
	end
	
	def set(*args)
		if args.length == 1
			x = args[0].x
			y = args[0].y
			width = args[0].width
			height = args[0].height
		elsif args.length == 4
			x = args[0]
			y = args[1]
			width = args[2]
			height = args[3]
		end
	end
	
	def empty
		set(0, 0, 0, 0)
	end
end

class Tone
	attr_reader :red, :green, :blue, :gray
	
	def initialize(*args)
		if args.length == 3 or args.length == 4
			@red = _mkxp_clamp(args[0], -255, 255)
			@green = _mkxp_clamp(args[1], -255, 255)
			@blue = _mkxp_clamp(args[2], -255, 255)
			@gray = (args.length == 4 ? _mkxp_clamp(args[3], 0, 255) : 255)
		elsif args.length == 0
			@red = 0
			@green = 0
			@blue = 0
			@gray = 0
		else
			raise ArgumentException.new("Wrong number of arguments")
		end
	end
	
	def set(*args)
		if args.length == 3 or args.length == 4
			@red = _mkxp_clamp(args[0], -255, 255)
			@green = _mkxp_clamp(args[1], -255, 255)
			@blue = _mkxp_clamp(args[2], -255, 255)
			@gray = (args.length == 4 ? _mkxp_clamp(args[3], 0, 255) : 255)
		elsif args.length == 1
			@red = args[0].red
			@green = args[0].green
			@blue = args[0].blue
			@gray = args[0].gray
		else
			raise ArgumentException.new("Wrong number of arguments")
		end
	end
	
	def red= r
		@red = _mkxp_clamp(r, -255, 255)
	end
	def green= g
		@green = _mkxp_clamp(g, -255, 255)
	end
	def blue= b
		@blue = _mkxp_clamp(b, -255, 255)
	end
	def gray= a
		@gray = _mkxp_clamp(a, 0, 255)
	end
end

module MkxpBinding
	extend FFI::Library
	
	ffi_lib FFI::CURRENT_PROCESS
	
	attach_function :mkxpRgssMain, [], :void
	attach_function :mkxpMsgboxString, [:string], :void
	
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
	
	class FFIColor < FFI::Struct
		layout 	:r, :int,
				:g, :int,
				:g, :int,
				:a, :int
	end
	class FFIRect < FFI::Struct
		layout 	:x, :int,
				:y, :int,
				:width, :int,
				:height, :int
	end
	
	module NativeObject
		attr_accessor :ptr
		private :ptr=
		
		def self.new(*args)
			instance = allocate
			
			instance.send(:initialize, args)
			ObjectSpace.define_finalizer(instance, proc {
															@@destructor.call(instance.ptr)
															instance.send(:ptr=, FFI::Pointer::NULL)
			                                            })
			
			instance
		end
		def self.new_ref(pointer)
			instance = allocate
			
			instance.send(:ptr=, pointer)
			
			instance
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
	
	attach_function :mkxpDisposableDispose, [:pointer], :void
	attach_function :mkxpDisposableIsDisposed, [:pointer], :int
	module Disposable
		def dispose
			MkxpBinding::mkxpDisposableDispose(ptr)
		end
		def disposed?
			MkxpBinding::mkxpDisposableIsDisposed(ptr) != 0
		end
	end
	
	attach_function :mkxpFontDelete, [:pointer], :void
	attach_function :mkxpFontExists, [:string], :int
	attach_function :mkxpFontInitialize, [:pointer, :int], :pointer
	attach_function :mkxpFontGetName, [:pointer], :pointer
	attach_function :mkxpFontSetName, [:pointer, :pointer], :void
	attach_function :mkxpFontGetSize, [:pointer], :int
	attach_function :mkxpFontSetSize, [:pointer, :int], :void
	attach_function :mkxpFontGetBold, [:pointer], :int
	attach_function :mkxpFontSetBold, [:pointer, :int], :void
	attach_function :mkxpFontGetItalic, [:pointer], :int
	attach_function :mkxpFontSetItalic, [:pointer, :int], :void
	attach_function :mkxpFontGetOutline, [:pointer], :int
	attach_function :mkxpFontSetOutline, [:pointer, :int], :void
	attach_function :mkxpFontGetShadow, [:pointer], :int
	attach_function :mkxpFontSetShadow, [:pointer, :int], :void
	attach_function :mkxpFontGetColor, [:pointer], FFIColor.by_value
	attach_function :mkxpFontSetColor, [:pointer, :int, :int, :int, :int], :void
	attach_function :mkxpFontGetOutColor, [:pointer], FFIColor.by_value
	attach_function :mkxpFontSetOutColor, [:pointer, :int, :int, :int, :int], :void
	attach_function :mkxpFontGetDefaultName, [], :pointer
	attach_function :mkxpFontSetDefaultName, [:pointer], :void
	attach_function :mkxpFontGetDefaultSize, [], :int
	attach_function :mkxpFontSetDefaultSize, [:int], :void
	attach_function :mkxpFontGetDefaultBold, [], :int
	attach_function :mkxpFontSetDefaultBold, [:int], :void
	attach_function :mkxpFontGetDefaultItalic, [], :int
	attach_function :mkxpFontSetDefaultItalic, [:int], :void
	attach_function :mkxpFontGetDefaultOutline, [], :int
	attach_function :mkxpFontSetDefaultOutline, [:int], :void
	attach_function :mkxpFontGetDefaultShadow, [], :int
	attach_function :mkxpFontSetDefaultShadow, [:int], :void
	attach_function :mkxpFontGetDefaultColor, [], FFIColor.by_value
	attach_function :mkxpFontSetDefaultColor, [:int, :int, :int, :int], :void
	attach_function :mkxpFontGetDefaultOutColor, [], FFIColor.by_value
	attach_function :mkxpFontSetDefaultOutColor, [:int, :int, :int, :int], :void
	class Font
		@@destructor = lambda {|pointer| MkxpBinding::mkxpFontDelete(pointer) }
		
		include MkxpBinding::NativeObject
		
		def self.exist?(name)
			MkxpBinding::mkxpFontExists(name.to_s()) != 0
		end
		
		def initialize(*args)
			strvec = nil
			fontsize = 0
			if args.length >= 1
				if args[0].is_a?(Array)
					strvec = MkxpBinding::mkxpStringVectorHelperNew()
					args[0].each do |obj|
						MkxpBinding::mkxpStringVectorHelperPushBack(strvec, obj.to_s())
					end
				elsif args[0] == nil
					strvec = nil
				else
					strvec = MkxpBinding::mkxpStringVectorHelperNew()
					MkxpBinding::mkxpStringVectorHelperPushBack(strvec, args[0].to_s())
				end
				
				if args.length == 2
					fontsize = args[1]
				end
			end
			@ptr = MkxpBinding::mkxpFontInitialize(strvec, fontsize)
		end
		
		def name
			strvec = MkxpBinding::mkxpFontGetName(ptr)
			strvecsize = MkxpBinding::mkxpStringVectorHelperSize(strvec)
			strarray = Array.new(strvecsize)
			strvecsize.times do |i|
				strarray[i] = MkxpBinding::mkxpStringVectorHelperAt(strvec, i).to_s()
			end
			strarray
		end
		def name= n
			strvec = nil
			if n.is_a?(Array)
				strvec = MkxpBinding::mkxpStringVectorHelperNew()
				n[0].each do |obj|
					MkxpBinding::mkxpStringVectorHelperPushBack(strvec, obj.to_s())
				end
			elsif n != nil
				strvec = MkxpBinding::mkxpStringVectorHelperNew()
				MkxpBinding::mkxpStringVectorHelperPushBack(strvec, n[0].to_s())
			end
			puts "hello"
			MkxpBinding::mkxpFontSetName(ptr, strvec)
		end
		
		def size
			MkxpBinding::mkxpFontGetSize(ptr)
		end
		def size= siz
			MkxpBinding::mkxpFontSetSize(ptr, siz)
		end
		
		def bold
			MkxpBinding::mkxpFontGetBold(ptr) != 0
		end
		def bold= bld
			MkxpBinding::mkxpFontSetBold(ptr, bld)
		end
		
		def italic
			MkxpBinding::mkxpFontGetItalic(ptr) != 0
		end
		def italic= itl
			MkxpBinding::mkxpFontSetItalic(ptr, itl)
		end
		
		def outline
			MkxpBinding::mkxpFontGetOutline(ptr) != 0
		end
		def outline= otl
			MkxpBinding::mkxpFontSetOutline(ptr, otl)
		end
		
		def shadow
			MkxpBinding::mkxpFontGetShadow(ptr) != 0
		end
		def shadow= shw
			MkxpBinding::mkxpFontSetShadow(ptr, shw)
		end
		
		def color
			intc = MkxpBinding::mkxpFontGetColor(ptr)
			Color.new(intc[:r],intc[:g],intc[:b],intc[:a])
		end
		def color= cl
			MkxpBindin::mkxpFontSetColor(ptr, cl.red, cl.green, cl.blue, cl.alpha)
		end
		
		def out_color
			intc = MkxpBinding::mkxpFontGetOutColor(ptr)
			Color.new(intc[:r],intc[:g],intc[:b],intc[:a])
		end
		def out_color= cl
			MkxpBindin::mkxpFontSetOutColor(ptr, cl.red, cl.green, cl.blue, cl.alpha)
		end
		
		def self.default_name
			strvec = MkxpBinding::mkxpFontGetDefaultName()
			strvecsize = MkxpBinding::mkxpStringVectorHelperSize(strvec)
			strarray = Array.new(strvecsize)
			strvecsize.times do |i|
				strarray[i] = MkxpBinding::mkxpStringVectorHelperAt(strvec, i).to_s()
			end
			strarray
		end
		def self.default_name= n
			strvec = nil
			if n.is_a?(Array)
				strvec = MkxpBinding::mkxpStringVectorHelperNew()
				n[0].each do |obj|
					MkxpBinding::mkxpStringVectorHelperPushBack(strvec, obj.to_s())
				end
			elsif n != nil
				strvec = MkxpBinding::mkxpStringVectorHelperNew()
				MkxpBinding::mkxpStringVectorHelperPushBack(strvec, n[0].to_s())
			end
			puts "hello"
			MkxpBinding::mkxpFontSetDefaultName(strvec)
		end
		
		def self.default_size
			MkxpBinding::mkxpFontGetDefaultSize()
		end
		def self.default_size= siz
			MkxpBinding::mkxpFontSetDefaultSize(siz)
		end
		
		def self.default_bold
			MkxpBinding::mkxpFontGetDefaultBold() != 0
		end
		def self.default_bold= bld
			MkxpBinding::mkxpFontSetDefaultBold(bld)
		end
		
		def self.default_italic
			MkxpBinding::mkxpFontGetDefaultItalic() != 0
		end
		def self.default_italic= itl
			MkxpBinding::mkxpFontSetDefaultItalic(itl)
		end
		
		def self.default_outline
			MkxpBinding::mkxpFontGetDefaultOutline() != 0
		end
		def self.default_outline= otl
			MkxpBinding::mkxpFontSetDefaultOutline(otl)
		end
		
		def self.default_shadow
			MkxpBinding::mkxpFontGetDefaultShadow() != 0
		end
		def self.default_shadow= shw
			MkxpBinding::mkxpFontSetDefaultShadow(shw)
		end
		
		def self.default_color
			intc = MkxpBinding::mkxpFontGetDefaultColor()
			Color.new(intc[:r],intc[:g],intc[:b],intc[:a])
		end
		def self.default_color= cl
			MkxpBindin::mkxpFontSetDefaultColor(cl.red, cl.green, cl.blue, cl.alpha)
		end
		
		def self.default_out_color
			intc = MkxpBinding::mkxpFontGetDefaultOutColor()
			Color.new(intc[:r],intc[:g],intc[:b],intc[:a])
		end
		def self.default_out_color= cl
			MkxpBindin::mkxpFontSetDefaultOutColor(cl.red, cl.green, cl.blue, cl.alpha)
		end
	end
	
	attach_function :mkxpBitmapDelete, [:pointer], :void
	attach_function :mkxpBitmapInitializeFilename, [:string], :pointer
	attach_function :mkxpBitmapInitializeExtent, [:int, :int], :pointer
	attach_function :mkxpBitmapWidth, [:pointer], :int
	attach_function :mkxpBitmapHeight, [:pointer], :int
	attach_function :mkxpBitmapRect, [:pointer], FFIRect.by_value
	attach_function :mkxpBitmapBlt, [:pointer, :int, :int, :pointer, :int, :int, :int, :int, :int], :void
	attach_function :mkxpBitmapStretchBlt, [:pointer, :int, :int, :int, :int, :pointer, :int, :int, :int, :int, :int], :void
	attach_function :mkxpBitmapFillRect, [:pointer, :int, :int, :int, :int, :int, :int, :int, :int], :void
	attach_function :mkxpBitmapGradientFillRect, [:pointer, :int, :int, :int, :int,:int, :int, :int, :int,:int, :int, :int, :int, :int], :void
	attach_function :mkxpBitmapClear, [:pointer], :void
	attach_function :mkxpBitmapClearRect, [:pointer, :int, :int, :int, :int], :void
	attach_function :mkxpBitmapGetPixel, [:pointer, :int, :int], FFIColor.by_value
	attach_function :mkxpBitmapSetPixel, [:pointer, :int, :int, :int,:int, :int, :int], :void
	attach_function :mkxpBitmapHueChange, [:pointer, :int], :void
	attach_function :mkxpBitmapBlur, [:pointer], :void
	attach_function :mkxpBitmapRadialBlur, [:pointer, :int, :int], :void
	attach_function :mkxpBitmapDrawText, [:pointer, :int, :int, :int, :int, :string, :int], :void
	attach_function :mkxpBitmapTextSize, [:pointer, :string], FFIRect.by_value
	attach_function :mkxpBitmapGetFont, [:pointer], :pointer
	attach_function :mkxpBitmapSetFont, [:pointer, :pointer], :void 
	class Bitmap
		@@destructor = lambda {|pointer| MkxpBinding::mkxpFontDelete(pointer) }
		
		include MkxpBinding::NativeObject
		include MkxpBinding::Disposable
		
		def initialize(*args)
			if args.length == 1
				@ptr = MkxpBinding::mkxpBitmapInitializeFilename(args[0])
			elsif args.length == 2
				@ptr = MkxpBinding::mkxpBitmapInitializeExtent(args[0], args[1])
			end
		end
		
		def width
			MkxpBinding::mkxpBitmapWidth(ptr)
		end
		
		def height
			MkxpBinding::mkxpBitmapHeight(ptr)
		end
		
		def rect
			rct = MkxpBinding::mkxpBitmapRect(ptr)
			Rect.new(rct[:x], rct[:y], rct[:width], rct[:height])
		end
		
		def blt(x, y, src_bitmap, src_rect, opacity = 255)
			MkxpBinding::mkxpBitmapBlt(ptr, x, y, src_bitmap.ptr, src_rect.x, src_rect.y, src_rect.width, src_rect.height, opacity)
		end
		
		def stretch_blt(dest_rect, src_bitmap, src_rect, opacity = 255)
			MkxpBinding::mkxpBitmapStretchBlt(ptr, dest_rect.x, dest_rect.y, dest_rect.width, dest_rect.height, src_bitmap.ptr, src_rect.x, src_rect.y, src_rect.width, src_rect.height, opacity)
		end
		def fill_rect(*args)
			if args.length == 2
				x = args[0].x
				y = args[0].y
				w = args[0].width
				h = args[0].height
				color = args[1]
			elsif args.length == 5
				x = args[0]
				y = args[1]
				w = args[2]
				h = args[3]
				color = args[4]
			end
			MkxpBinding::mkxpBitmapFillRect(ptr, x, y, w, h, color.red, color.green, color.blue, color.alpha)
		end
		
		def gradient_fill_rect(*args)
			vertical = false
			if args.length == 3 or args.length == 4
				x = args[0].x
				y = args[0].y
				w = args[0].width
				h = args[0].height
				color1 = args[1]
				color2 = args[2]
				if args.length == 4
					vertical = args[3]
				end
			elsif args.length == 6 or args.length == 7
				x = args[0]
				y = args[1]
				w = args[2]
				h = args[3]
				color1 = args[4]
				color2 = args[5]
				if args.length == 7
					vertical = args[6]
				end
			end
			MkxpBinding::mkxpBitmapGradientFillRect(ptr, x, y, w, h, color1.red, color1.green, color1.blue, color1.alpha, color2.red, color2.green, color2.blue, color2.alpha, vertical)
		end
		
		def clear
			MkxpBinding::mkxpBitmapClear(ptr)
		end
		
		def clear_rect(*args)
			if args.length == 1
				x = args[0].x
				y = args[0].y
				w = args[0].width
				h = args[0].height
			elsif args.length == 4
				x = args[0]
				y = args[1]
				w = args[2]
				h = args[3]
			end
			MkxpBinding::mkxpBitmapClearRect(ptr, x, y, w, h)
		end
		
		def get_pixel(x, y)
			intc = MkxpBinding::mkxpBitmapGetPixel(ptr, x, y)
			Color.new(intc[:r],intc[:g],intc[:b],intc[:a])
		end
		
		def set_pixel(x, y, color)
			MkxpBinding::mkxpBitmapSetPixel(ptr, x, y, color.red, color.green, color.blue, color.alpha)
		end
		
		def hue_change(hue)
			MkxpBinding::mkxpBitmapHueChange(ptr, hue)
		end
		
		def blur
			MkxpBinding::mkxpBitmapBlur(ptr)
		end
		
		def radial_blur(angle, divisions)
			MkxpBinding::mkxpBitmapRadialBlur(ptr, angle, divisions)
		end
		
		def draw_text(x, y, w, h, str, align = 0)
			align = 0
			if args.length == 2 or args.length == 3
				x = args[0].x
				y = args[0].y
				w = args[0].width
				h = args[0].height
				str = args[1]
				if args.length == 3
					align = args[2]
				end
			elsif args.length == 5 or args.length == 6
				x = args[0]
				y = args[1]
				w = args[2]
				h = args[3]
				str = args[4]
				if args.length == 6
					align = args[5]
				end
			end
			MkxpBinding::mkxpBitmapDrawText(ptr, x, y, w, h, str, align)
		end
		
		def text_size(str)
			MkxpBinding::mkxpBitmapTextSize(ptr, str)
		end
		
		def font
			MkxpBinding::Font.new_ref(MkxpBinding::mkxpBitmapGetFont(ptr))
		end
		def font= f
			MkxpBinding::mkxpBitmapSetFont(ptr, f.ptr)
		end
	end
	
	class Plane
		@@destructor = lambda {|pointer| MkxpBinding::mkxpPlaneDelete(pointer) }
		
		include NativeObject
		include Disposable
		
		def initialize(viewport = nil)
# 			@ptr = MkxpBinding::mkxpPlaneInitialize(viewport != nil ? viewport.ptr, : FFI::Pointer::NULL)
		end
	end
end

Bitmap = MkxpBinding::Bitmap

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

def rgss_main(block)
	block.call
end
