#include "binding-util.h"

#include "etc.h"

extern "C" {
    BINDING_CONSTRUCTOR(Rect)(int x, int y, int w, int h) {
        return new Rect(x, y, w, h);
    }
    BINDING_DESTRUCTOR(Rect)
    
    BINDING_PROPERTY(Rect, int, X)
    BINDING_PROPERTY(Rect, int, Y)
    BINDING_PROPERTY(Rect, int, Width)
    BINDING_PROPERTY(Rect, int, Height)
    
    Rect* mkxpRectDeserialize(char* ptr, int len) {
        return Rect::deserialize(ptr, len);
    }
}
