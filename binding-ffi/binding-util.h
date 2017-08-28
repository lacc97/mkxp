#ifndef BINDING_UTIL_H
#define BINDING_UTIL_H

#include <vector>

#include "font.h"
#include "etc.h"

struct FFIColor {
	int r, g, b, a;
};
struct FFIRect {
	int x, y, width, height;
};
struct FFIFont {
	Font font;
	std::vector<std::string> namevec;
	Color color;
	Color outcolor;
	
	static std::vector<std::string> default_namevec;
	static Color default_color;
	static Color default_outcolor;
};

#endif
