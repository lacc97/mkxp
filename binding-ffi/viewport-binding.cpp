#include "binding-util.h"

#include "viewport.h"

extern "C" {
	BINDING_CONSTRUCTOR(Viewport)(Rect* rect) {
        Viewport* v;
		if(rect->width < 0 || rect->height < 0)
			v = new Viewport();
		else
			v = new Viewport(rect);
        v->initDynAttribs();
        return v;
	}
	BINDING_DESTRUCTOR(Viewport)
	
	Rect* mkxpViewportGetRect(Viewport* ptr) {
		return &ptr->getRect();
	}
	void mkxpViewportSetRect(Viewport* ptr, Rect* r) {
		return ptr->setRect(*r);
	}
	BINDING_PROPERTY(Viewport, int, Visible)
	BINDING_PROPERTY(Viewport, int, Z)
	BINDING_PROPERTY_DET(Viewport, int, Ox, OX)
	BINDING_PROPERTY_DET(Viewport, int, Oy, OY)
	BINDING_PROPERTY_REF(Viewport, Color, Color)
	BINDING_PROPERTY_REF(Viewport, Tone, Tone)
	
	BINDING_METHOD(Viewport, void, Flash)(Viewport* ptr, Color* c, int duration) {
		if(!c)
			ptr->flash(nullptr, duration);
		else {
			Vec4 vc(c->red, c->green, c->blue, c->alpha);
			ptr->flash(&vc, duration);
		}
	}
	BINDING_METHOD(Viewport, void, Update)(Viewport* ptr) {
		ptr->update();
	}
}
