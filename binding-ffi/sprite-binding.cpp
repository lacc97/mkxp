#include "binding-util.h"

#include "sprite.h"

extern "C" {
	BINDING_CONSTRUCTOR(Sprite)(Viewport* vp) {
        Sprite* s = new Sprite(vp);
        s->initDynAttribs();
		return s;
	}
	BINDING_DESTRUCTOR(Sprite)
//     BINDING_ASSIGN(Sprite)
	
	BINDING_PROPERTY(Sprite, Bitmap*, Bitmap)
	BINDING_PROPERTY_REF(Sprite, Rect, SrcRect)
	BINDING_PROPERTY(Sprite, Viewport*, Viewport)
	BINDING_PROPERTY(Sprite, int, Visible)
	BINDING_PROPERTY(Sprite, int, X)
	BINDING_PROPERTY(Sprite, int, Y)
	BINDING_PROPERTY(Sprite, int, Z)
	BINDING_PROPERTY_DET(Sprite, int, Ox, OX)
	BINDING_PROPERTY_DET(Sprite, int, Oy, OY)
	BINDING_PROPERTY(Sprite, float, ZoomX)
	BINDING_PROPERTY(Sprite, float, ZoomY)
	BINDING_PROPERTY(Sprite, int, Angle)
	BINDING_PROPERTY(Sprite, int, WaveAmp)
	BINDING_PROPERTY(Sprite, int, WaveLength)
	BINDING_PROPERTY(Sprite, int, WaveSpeed)
	BINDING_PROPERTY(Sprite, int, WavePhase)
	BINDING_PROPERTY(Sprite, int, Mirror)
	BINDING_PROPERTY(Sprite, int, BushDepth)
	BINDING_PROPERTY(Sprite, int, BushOpacity)
	BINDING_PROPERTY(Sprite, int, Opacity)
	BINDING_PROPERTY(Sprite, int, BlendType)
	BINDING_PROPERTY_REF(Sprite, Color, Color)
	BINDING_PROPERTY_REF(Sprite, Tone, Tone)
	
	BINDING_METHOD(Sprite, void, Flash)(Sprite* ptr, Color* c, int duration) {
		if(!c)
			ptr->flash(nullptr, duration);
		else {
			Vec4 vc(c->red, c->green, c->blue, c->alpha);
			ptr->flash(&vc, duration);
		}
	}
	BINDING_METHOD(Sprite, void, Update)(Sprite* ptr) {
		ptr->update();
	}
	BINDING_METHOD(Sprite, int, Width)(Sprite* ptr) {
		return ptr->getWidth();
	}
	BINDING_METHOD(Sprite, int, Height)(Sprite* ptr) {
		return ptr->getHeight();
	}
}
