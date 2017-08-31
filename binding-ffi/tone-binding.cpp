#include "binding-util.h"

#include "etc.h"

extern "C" {
	BINDING_CONSTRUCTOR(Tone)(float r, float g, float b, float gr) {
		return new Tone(r, g, b, gr);
	}
	BINDING_DESTRUCTOR(Tone)
	
	BINDING_PROPERTY(Tone, float, Red)
	BINDING_PROPERTY(Tone, float, Green)
	BINDING_PROPERTY(Tone, float, Blue)
	BINDING_PROPERTY(Tone, float, Gray)
    
    Tone* mkxpToneDeserialize(char* ptr, int len) {
        return Tone::deserialize(ptr, len);
    }
}
