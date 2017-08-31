#include "binding-util.h"

#include "etc.h"

extern "C" {
	BINDING_CONSTRUCTOR(Color)(float r, float g, float b, float a) {
		return new Color(r, g, b, a);
	}
	BINDING_DESTRUCTOR(Color)
	
	BINDING_PROPERTY(Color, float, Red)
	BINDING_PROPERTY(Color, float, Green)
	BINDING_PROPERTY(Color, float, Blue)
	BINDING_PROPERTY(Color, float, Alpha)
    
    Color* mkxpColorDeserialize(char* ptr, int len) {
        return Color::deserialize(ptr, len);
    }
}
