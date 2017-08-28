#include "binding-util.h"

#include <vector>

std::vector<std::string> FFIFont::default_namevec;
Color FFIFont::default_color;
Color FFIFont::default_outcolor;

extern "C" {
    void mkxpFontDelete(void* ptr) {
        delete reinterpret_cast<FFIFont*>(ptr);
    }

    int mkxpFontExists(const char* str) {
        return Font::doesExist(str);
    }

    void* mkxpFontInitialize(void* strvec, int size) {
        std::vector<std::string>* vecptr = reinterpret_cast<std::vector<std::string>*>(strvec);
        FFIFont* fptr = new FFIFont();
        fptr->font = Font(vecptr, size);
		fptr->font.setColor(fptr->color);
		fptr->font.setOutColor(fptr->outcolor);
        delete vecptr;
        return fptr;
    }

    void* mkxpFontGetName(void* ptr) {
        return &reinterpret_cast<FFIFont*>(ptr)->namevec;
    }

    void mkxpFontSetName(void* ptr, void* strvec) {
        if(strvec) {
            std::vector<std::string>* vecptr = reinterpret_cast<std::vector<std::string>*>(strvec);
            FFIFont* fptr = reinterpret_cast<FFIFont*>(ptr);
            fptr->font.setName(*vecptr);
            delete vecptr;
        }
    }
    
    int mkxpFontGetSize(void* ptr) {
		return reinterpret_cast<FFIFont*>(ptr)->font.getSize();
	}
	
	void mkxpFontSetSize(void* ptr, int v) {
		return reinterpret_cast<FFIFont*>(ptr)->font.setSize(v);
	}
	
	int mkxpFontGetBold(void* ptr) {
		return reinterpret_cast<FFIFont*>(ptr)->font.getBold();
	}
	
	void mkxpFontSetBold(void* ptr, int v) {
		return reinterpret_cast<FFIFont*>(ptr)->font.setBold(v);
	}
	
	int mkxpFontGetItalic(void* ptr) {
		return reinterpret_cast<FFIFont*>(ptr)->font.getItalic();
	}
	
	void mkxpFontSetItalic(void* ptr, int v) {
		return reinterpret_cast<FFIFont*>(ptr)->font.setItalic(v);
	}
	
	int mkxpFontGetOutline(void* ptr) {
		return reinterpret_cast<FFIFont*>(ptr)->font.getOutline();
	}
	
	void mkxpFontSetOutline(void* ptr, int v) {
		return reinterpret_cast<FFIFont*>(ptr)->font.setOutline(v);
	}
	
	int mkxpFontGetShadow(void* ptr) {
		return reinterpret_cast<FFIFont*>(ptr)->font.getShadow();
	}
	
	void mkxpFontSetShadow(void* ptr, int v) {
		return reinterpret_cast<FFIFont*>(ptr)->font.setShadow(v);
	}
	
	FFIColor mkxpFontGetColor(void* ptr) {
		const Color& c = reinterpret_cast<FFIFont*>(ptr)->color;
		return {static_cast<int>(c.red), static_cast<int>(c.green), static_cast<int>(c.blue), static_cast<int>(c.alpha)};
	}
	
	void mkxpFontSetColor(void* ptr, int r, int g, int b, int a) {
		reinterpret_cast<FFIFont*>(ptr)->color = Vec4(r, g, b, a);
	}
	
	FFIColor mkxpFontGetOutColor(void* ptr) {
		const Color& c = reinterpret_cast<FFIFont*>(ptr)->outcolor;
		return {static_cast<int>(c.red), static_cast<int>(c.green), static_cast<int>(c.blue), static_cast<int>(c.alpha)};
	}
	
	void mkxpFontSetOutColor(void* ptr, int r, int g, int b, int a) {
		reinterpret_cast<FFIFont*>(ptr)->outcolor = Vec4(r, g, b, a);
	}
	
	void* mkxpFontGetDefaultName() {
        return &FFIFont::default_namevec;
    }

    void mkxpFontSetDefaultName(void* strvec) {
        if(strvec) {
            std::vector<std::string>* vecptr = reinterpret_cast<std::vector<std::string>*>(strvec);
            FFIFont::default_namevec.swap(*vecptr);
            delete vecptr;
        }
    }
    
    int mkxpFontGetDefaultSize() {
		return Font::getDefaultSize();
	}
	
	void mkxpFontSetDefaultSize(int v) {
		return Font::setDefaultSize(v);
	}
	
	int mkxpFontGetDefaultBold() {
		return Font::getDefaultBold();
	}
	
	void mkxpFontSetDefaultBold(int v) {
		return Font::setDefaultBold(v);
	}
	
	int mkxpFontGetDefaultItalic() {
		return Font::getDefaultItalic();
	}
	
	void mkxpFontSetDefaultItalic(int v) {
		return Font::setDefaultItalic(v);
	}
	
	int mkxpFontGetDefaultOutline() {
		return Font::getDefaultOutline();
	}
	
	void mkxpFontSetDefaultOutline(int v) {
		return Font::setDefaultOutline(v);
	}
	
	int mkxpFontGetDefaultShadow() {
		return Font::getDefaultShadow();
	}
	
	void mkxpFontSetDefaultShadow(int v) {
		return Font::setDefaultShadow(v);
	}
	
	FFIColor mkxpFontGetDefaultColor() {
		const Color& c = FFIFont::default_color;
		return {static_cast<int>(c.red), static_cast<int>(c.green), static_cast<int>(c.blue), static_cast<int>(c.alpha)};
	}
	
	void mkxpFontSetDefaultColor(int r, int g, int b, int a) {
		FFIFont::default_color = Vec4(r, g, b, a);
	}
	
	FFIColor mkxpFontGetDefaultOutColor() {
		const Color& c = FFIFont::default_outcolor;
		return {static_cast<int>(c.red), static_cast<int>(c.green), static_cast<int>(c.blue), static_cast<int>(c.alpha)};
	}
	
	void mkxpFontSetDefaultOutColor(int r, int g, int b, int a) {
		FFIFont::default_outcolor = Vec4(r, g, b, a);
	}
}
