#include <sharedstate.h>
#include "binding-util.h"

#include <vector>

// std::vector<std::string> Font::default_namevec;
// Color Font::default_color;
// Color Font::default_outcolor;

extern "C" {

    int mkxpFontExists(const char* str) {
        return Font::doesExist(str);
    }

    void* mkxpFontNew(void* strvec, int size) {
        std::vector<std::string>* vecptr = reinterpret_cast<std::vector<std::string>*>(strvec);
        Font* fptr = new Font(vecptr, size);
        delete vecptr;
//         fptr->initDynAttribs();
        return fptr;
    }
    void* mkxpFontNewCopy(Font* other) {
        return new Font(*other);
    }
    BINDING_DESTRUCTOR(Font)

    void mkxpFontSetName(void* ptr, void* strvec) {
        if(strvec) {
            std::vector<std::string>* vecptr = reinterpret_cast<std::vector<std::string>*>(strvec);
            Font* fptr = reinterpret_cast<Font*>(ptr);
            fptr->setName(*vecptr);
            delete vecptr;
        }
	}
	BINDING_PROPERTY(Font, int, Size)
	BINDING_PROPERTY(Font, int, Bold)
	BINDING_PROPERTY(Font, int, Italic)
	BINDING_PROPERTY(Font, int, Outline)
	BINDING_PROPERTY(Font, int, Shadow)
	BINDING_PROPERTY_REF(Font, Color, Color)
	BINDING_PROPERTY_REF(Font, Color, OutColor)

    void mkxpFontSetDefaultName(void* strvec) {
        if(strvec) {
            std::vector<std::string>* vecptr = reinterpret_cast<std::vector<std::string>*>(strvec);
            Font::setDefaultName(*vecptr, shState->fontState());
            delete vecptr;
        }
    }
    BINDING_STATIC_PROPERTY(Font, int, DefaultSize)
	BINDING_STATIC_PROPERTY(Font, int, DefaultBold)
	BINDING_STATIC_PROPERTY(Font, int, DefaultItalic)
	BINDING_STATIC_PROPERTY(Font, int, DefaultOutline)
	BINDING_STATIC_PROPERTY(Font, int, DefaultShadow)
	BINDING_STATIC_PROPERTY_REF(Font, Color, DefaultColor)
	BINDING_STATIC_PROPERTY_REF(Font, Color, DefaultOutColor)
}
