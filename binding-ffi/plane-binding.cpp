#include "binding-util.h"

#include "plane.h"

extern "C" {
	BINDING_CONSTRUCTOR(Plane)(Viewport* v) {
		return new Plane(v);
	}
	BINDING_DESTRUCTOR(Plane)
	
	BINDING_PROPERTY(Plane, Bitmap*, Bitmap)
	BINDING_PROPERTY(Plane, Viewport*, Viewport)
	BINDING_PROPERTY(Plane, int, Visible)
	BINDING_PROPERTY(Plane, int, Z)
	BINDING_PROPERTY_DET(Plane, int, Ox, OX)
	BINDING_PROPERTY_DET(Plane, int, Oy, OY)
	BINDING_PROPERTY(Plane, float, ZoomX)
	BINDING_PROPERTY(Plane, float, ZoomY)
	BINDING_PROPERTY(Plane, int, Opacity)
	BINDING_PROPERTY(Plane, int, BlendType)
	BINDING_PROPERTY(Plane, Color, Color)
	BINDING_PROPERTY(Plane, Tone, Tone)
}
