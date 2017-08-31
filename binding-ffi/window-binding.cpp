#include "binding-util.h"

#include "window.h"
#include "windowvx.h"

extern "C" {
	BINDING_CONSTRUCTOR(Window)(Viewport* vp, int x, int y, int w, int h) {
		if(shState->rgssVersion > 1) {
            WindowVX* win = new WindowVX(x, y, w, h);
            win->initDynAttribs();
			return win;
        }
        else if(shState->rgssVersion == 1){
            Window* win = new Window(vp);
            win->initDynAttribs();
			return win;
        }
		return nullptr;
	}
	BINDING_DESTRUCTOR_VX(Window)
	
	BINDING_PROPERTY_VX(Window, Bitmap*, Windowskin)
	BINDING_PROPERTY_VX(Window, Bitmap*, Contents)
	BINDING_PROPERTY_VX(Window, Viewport*, Viewport)
	BINDING_PROPERTY_VX(Window, int, Active)
	BINDING_METHOD(Window, int, GetVisible)(void* p) {
		if(shState->rgssVersion > 1) {
			return reinterpret_cast<WindowVX*>(p)->getVisible();
		} else {
			return reinterpret_cast<Window*>(p)->getVisible();
		}
	}
	BINDING_METHOD(Window, void, SetVisible)(void* p, int v) {
		if(shState->rgssVersion > 1) {
			reinterpret_cast<WindowVX*>(p)->setVisible(v);
		}
	}
	BINDING_PROPERTY_VXONLY(Window, int, ArrowsVisible)
	BINDING_PROPERTY_VX(Window, int, Pause)
	BINDING_PROPERTY_VX(Window, int, X)
	BINDING_PROPERTY_VX(Window, int, Y)
	BINDING_METHOD(Window, int, GetZ)(void* p) {
		if(shState->rgssVersion > 1) {
			return reinterpret_cast<WindowVX*>(p)->getZ();
		} else {
			return reinterpret_cast<Window*>(p)->getZ();
		}
	}
	BINDING_METHOD(Window, void, SetZ)(void* p, int z) {
		if(shState->rgssVersion > 1) {
			reinterpret_cast<WindowVX*>(p)->setZ(z);
		}
	}
	BINDING_PROPERTY_VX(Window, int, Width)
	BINDING_PROPERTY_VX(Window, int, Height)
	BINDING_PROPERTY_VX_DET(Window, int, Ox, OX)
	BINDING_PROPERTY_VX_DET(Window, int, Oy, OY)
	BINDING_PROPERTY_VXONLY(Window, int, Padding)
	BINDING_PROPERTY_VXONLY(Window, int, PaddingBottom)
	BINDING_PROPERTY_VX(Window, int, Opacity)
	BINDING_PROPERTY_VX(Window, int, BackOpacity)
	BINDING_PROPERTY_VX(Window, int, ContentsOpacity)
	BINDING_PROPERTY_VXONLY(Window, int, Openness)
	BINDING_PROPERTY_XPONLY(Window, int, Stretch)
	BINDING_PROPERTY_VXONLY_REF(Window, Tone, Tone)
	
	BINDING_METHOD(Window, Rect*, GetCursorRect)(void* p) {
		if(shState->rgssVersion > 1) {
			return &reinterpret_cast<WindowVX*>(p)->getCursorRect();
		} else {
			return &reinterpret_cast<Window*>(p)->getCursorRect();
		}
	}
	BINDING_METHOD(Window, void, SetCursorRect)(void* p, Rect* r) {
		if(shState->rgssVersion > 1) {
			reinterpret_cast<WindowVX*>(p)->setCursorRect(*r);
		} else {
			reinterpret_cast<Window*>(p)->setCursorRect(*r);
		}
	}
	
	BINDING_METHOD(Window, void, Update)(void* p) {
		if(shState->rgssVersion > 1) {
			reinterpret_cast<WindowVX*>(p)->update();
		} else {
			reinterpret_cast<Window*>(p)->update();
		}
	}
	BINDING_METHOD(Window, void, Move)(void* p, int x, int y, int w, int h) {
		if(shState->rgssVersion > 1) {
			reinterpret_cast<WindowVX*>(p)->move(x, y, w, h);
		}
	}
	
	BINDING_METHOD(Window, int, Open)(void* p) {
		if(shState->rgssVersion > 1) {
			return reinterpret_cast<WindowVX*>(p)->isOpen();
		}
		return false;
	}
	BINDING_METHOD(Window, int, Close)(void* p) {
		if(shState->rgssVersion > 1) {
			reinterpret_cast<WindowVX*>(p)->isOpen();
		}
		return false;
	}
}
