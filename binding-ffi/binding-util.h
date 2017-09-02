#ifndef BINDING_UTIL_H
#define BINDING_UTIL_H

#include <vector>
#include <unordered_map>
#include <iostream>

#include "font.h"
#include "etc.h"
#include "input.h"

// struct FFIColor {
// 	float r, g, b, a;
// 	
// 	FFIColor(const Color& c) {
// 		r = c.red;
// 		g = c.green;
// 		b = c.blue;
// 		a = c.alpha;
// 	}
// 	
// 	operator Color() const {
// 		return Color(r, g, b, a);
// 	}
// 	operator Vec4() const {
// 		return Vec4(r, g, b, a);
// 	}
// };

extern std::unordered_map<std::string, Input::ButtonCode> buttonMap;

struct FFIRect {
	int x, y, width, height;
	
	FFIRect(const Rect& c) {
		x = c.getX();
		y = c.getY();
		width = c.getWidth();
		height = c.getHeight();
	}
	
	FFIRect(const IntRect& c) {
		x = c.x;
		y = c.y;
		width = c.w;
		height = c.h;
	}
	
	operator Rect() const {
		return Rect(x, y, width, height);
	}
	operator IntRect() const {
		return IntRect(x, y, width, height);
	}
};
struct FFITone {
	float r, g, b, gr;
	
	operator Tone() const {
		return Tone(r, g, b, gr);
	}
};

#define BINDING_CONSTRUCTOR(klass) void* mkxp##klass##New
#define BINDING_COPY_CONSTRUCTOR(klass) void* mkxp##klass##NewCopy(klass* orig) { \
    return new klass(*orig); \
}
#define BINDING_DESTRUCTOR(klass) void mkxp##klass##Delete(klass* ptr) { \
	delete ptr; \
}
#define BINDING_ASSIGN(klass) void mkxp##klass##Assign(klass* curr, klass* oth) { \
    *curr = *oth; \
}
#define BINDING_METHOD(klass, retType, methodName) retType mkxp##klass##methodName
#define BINDING_PROPERTY_DET(klass, propType, propName, libPropName) \
	propType mkxp##klass##Get##propName(void* ptr) { \
		return reinterpret_cast<klass*>(ptr)->get##libPropName(); \
	} \
	void mkxp##klass##Set##propName(void* ptr, propType val) { \
		reinterpret_cast<klass*>(ptr)->set##libPropName(val); \
	}
#define BINDING_STATIC_PROPERTY_DET(klass, propType, propName, libPropName) \
	propType mkxp##klass##Get##propName() { \
		return klass::get##libPropName(); \
	} \
	void mkxp##klass##Set##propName(propType val) { \
		klass::set##libPropName(val); \
	}
#define BINDING_PROPERTY_REF(klass, propType, propName) \
	propType* mkxp##klass##Get##propName(void* ptr) { \
		return &reinterpret_cast<klass*>(ptr)->get##propName(); \
	} \
	void mkxp##klass##Set##propName(void* ptr, propType* val) {\
		reinterpret_cast<klass*>(ptr)->set##propName(*val); \
	}
#define BINDING_STATIC_PROPERTY_REF(klass, propType, propName) \
	propType* mkxp##klass##Get##propName() { \
		return &klass::get##propName(); \
	} \
	void mkxp##klass##Set##propName(propType* val) {\
		klass::set##propName(*val); \
	}
#define BINDING_PROPERTY(klass, propType, propName) BINDING_PROPERTY_DET(klass, propType, propName, propName)
#define BINDING_STATIC_PROPERTY(klass, propType, propName) BINDING_STATIC_PROPERTY_DET(klass, propType, propName, propName)

#define BINDING_DESTRUCTOR_VX(klass) void mkxp##klass##Delete(void* ptr) { \
	if(shState->rgssVersion > 1) \
		delete reinterpret_cast<klass##VX*>(ptr); \
	else \
		delete reinterpret_cast<klass*>(ptr); \
}
#define BINDING_ASSIGN_VX(klass) void mkxp##klass##Assign(void* curr, void* oth) { \
    if(shState->rgssVersion > 1) \
		*reinterpret_cast<klass##VX*>(curr) = *reinterpret_cast<klass##VX*>(oth); \
	else \
		*reinterpret_cast<klass*>(curr) = *reinterpret_cast<klass*>(oth); \
}
#define BINDING_PROPERTY_VX_DET(klass, propType, propName, libPropName) propType mkxp##klass##Get##propName(void* ptr) { \
		if(shState->rgssVersion > 1) \
			return reinterpret_cast<klass##VX*>(ptr)->get##libPropName(); \
		else \
			return reinterpret_cast<klass*>(ptr)->get##libPropName(); \
	} \
	void mkxp##klass##Set##propName(void* ptr, propType val) {\
		if(shState->rgssVersion > 1) \
			reinterpret_cast<klass##VX*>(ptr)->set##libPropName(val); \
		else \
			reinterpret_cast<klass*>(ptr)->set##libPropName(val); \
	}
#define BINDING_PROPERTY_VX(klass, propType, propName) BINDING_PROPERTY_VX_DET(klass, propType, propName, propName)
#define BINDING_PROPERTY_VX_REF(klass, propType, propName) \
	propType* mkxp##klass##Get##propName(void* ptr) { \
		if(shState->rgssVersion > 1) \
			return &reinterpret_cast<klass##VX*>(ptr)->get##propName(); \
		else \
			return &reinterpret_cast<klass*>(ptr)->get##propName(); \
	} \
	void mkxp##klass##Set##propName(void* ptr, propType* val) {\
		if(shState->rgssVersion > 1) \
			reinterpret_cast<klass##VX*>(ptr)->set##propName(*val); \
		else \
			reinterpret_cast<klass*>(ptr)->set##propName(*val); \
	}

#define BINDING_PROPERTY_XPONLY_DET(klass, propType, propName, libPropName) propType mkxp##klass##Get##propName(void* ptr) { \
		if(shState->rgssVersion == 1) \
			return reinterpret_cast<klass*>(ptr)->get##libPropName(); \
	} \
	void mkxp##klass##Set##propName(void* ptr, propType val) {\
		if(shState->rgssVersion == 1) \
			reinterpret_cast<klass*>(ptr)->set##libPropName(val); \
	}
#define BINDING_PROPERTY_XPONLY(klass, propType, propName) BINDING_PROPERTY_XPONLY_DET(klass, propType, propName, propName)
#define BINDING_PROPERTY_VXONLY_DET(klass, propType, propName, libPropName) propType mkxp##klass##Get##propName(void* ptr) { \
		if(shState->rgssVersion > 1) \
			return reinterpret_cast<klass##VX*>(ptr)->get##libPropName(); \
	} \
	void mkxp##klass##Set##propName(void* ptr, propType val) {\
		if(shState->rgssVersion > 1) \
			reinterpret_cast<klass##VX*>(ptr)->set##libPropName(val); \
	}
#define BINDING_PROPERTY_VXONLY(klass, propType, propName) BINDING_PROPERTY_VXONLY_DET(klass, propType, propName, propName)
#define BINDING_PROPERTY_VXONLY_REF(klass, propType, propName) \
	propType* mkxp##klass##Get##propName(void* ptr) { \
		if(shState->rgssVersion > 1) \
			return &reinterpret_cast<klass##VX*>(ptr)->get##propName(); \
	} \
	void mkxp##klass##Set##propName(void* ptr, propType* val) {\
		if(shState->rgssVersion > 1) \
			reinterpret_cast<klass##VX*>(ptr)->set##propName(*val); \
	}

#endif
