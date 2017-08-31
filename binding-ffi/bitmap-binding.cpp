#include "binding-util.h"

#include "bitmap.h"

#include <cstdlib>

extern "C" {
	BINDING_DESTRUCTOR(Bitmap)
	
	void* mkxpBitmapInitializeFilename(const char* filename) {
		return new Bitmap(filename);
	}
	
	void* mkxpBitmapInitializeExtent(int width, int height) {
		return new Bitmap(width, height);
	}
	
	int mkxpBitmapWidth(void* ptr) {
		return reinterpret_cast<Bitmap*>(ptr)->width();
	}
	
	int mkxpBitmapHeight(void* ptr) {
		return reinterpret_cast<Bitmap*>(ptr)->height();
	}
	
	Rect* mkxpBitmapRect(void* ptr) {
		return new Rect(reinterpret_cast<Bitmap*>(ptr)->rect());
	}
	
	void mkxpBitmapBlt(void* ptr, int x, int y, Bitmap* src_bitmap, Rect* src_rect, int opacity) {
		return reinterpret_cast<Bitmap*>(ptr)->blt(x, y, *src_bitmap, src_rect->toIntRect(), opacity);
	}
	
	void mkxpBitmapStretchBlt(void* ptr, Rect* dest_rect, Bitmap* src_bitmap, Rect* src_rect, int opacity) {
		return reinterpret_cast<Bitmap*>(ptr)->stretchBlt(dest_rect->toIntRect(), *src_bitmap, src_rect->toIntRect(), opacity);
	}
	
	void mkxpBitmapFillRect(void* ptr, Rect* rect, Color* c) {
		return reinterpret_cast<Bitmap*>(ptr)->fillRect(rect->toIntRect(), Vec4(c->red, c->green, c->blue, c->alpha));
	}
	
	void mkxpBitmapGradientFillRect(void* ptr, Rect* rect, Color* c1, Color* c2, int vertical) {
		return reinterpret_cast<Bitmap*>(ptr)->gradientFillRect(rect->toIntRect(), Vec4(c1->red, c1->green, c1->blue, c1->alpha), Vec4(c2->red, c2->green, c2->blue, c2->alpha), vertical);
	}
	
	void mkxpBitmapClear(void* ptr) {
		return reinterpret_cast<Bitmap*>(ptr)->clear();
	}
	
	void mkxpBitmapClearRect(void* ptr, Rect* r) {
		return reinterpret_cast<Bitmap*>(ptr)->clearRect(r->toIntRect());
	}
	
	void mkxpBitmapGetPixel(void* ptr, int x, int y, Color* out) {
		*out = reinterpret_cast<Bitmap*>(ptr)->getPixel(x, y);
	}
	
	void mkxpBitmapSetPixel(void* ptr, int x, int y, Color* in) {
		return reinterpret_cast<Bitmap*>(ptr)->setPixel(x, y, *in);
	}
	
	void mkxpBitmapHueChange(void* ptr, int hue) {
		return reinterpret_cast<Bitmap*>(ptr)->hueChange(hue);
	}
	
	void mkxpBitmapBlur(void* ptr) {
		return reinterpret_cast<Bitmap*>(ptr)->blur();
	}
	
	void mkxpBitmapRadialBlur(void* ptr, int angle, int divisions) {
		return reinterpret_cast<Bitmap*>(ptr)->radialBlur(angle, divisions);
	}
	
	void mkxpBitmapDrawText(void* ptr, Rect* r, const char* str, int align) {
		return reinterpret_cast<Bitmap*>(ptr)->drawText(r->toIntRect(), str, align);
	}
	
	Rect* mkxpBitmapTextSize(void* ptr, const char* str) {
		return new Rect(reinterpret_cast<Bitmap*>(ptr)->textSize(str));
	}
	
	BINDING_PROPERTY_REF(Bitmap, Font, Font)
}
