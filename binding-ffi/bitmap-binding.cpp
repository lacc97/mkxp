#include "binding-util.h"

#include "bitmap.h"

#include <cstdlib>

extern "C" {
	void mkxpBitmapDelete(void* ptr) {
		delete reinterpret_cast<Bitmap*>(ptr);
	}
	
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
	
	FFIRect mkxpBitmapRect(void* ptr) {
		IntRect r = reinterpret_cast<Bitmap*>(ptr)->rect();
		return {r.x, r.y, r.w, r.h};
	}
	
	void mkxpBitmapBlt(void* ptr, int x, int y, void* src_bitmap, int src_x, int src_y, int src_width, int src_height, int opacity) {
		return reinterpret_cast<Bitmap*>(ptr)->blt(x, y, *reinterpret_cast<Bitmap*>(src_bitmap), IntRect(src_x, src_y, src_width, src_height), opacity);
	}
	
	void mkxpBitmapStretchBlt(void* ptr, int dst_x, int dst_y, int dst_width, int dst_height, void* src_bitmap, int src_x, int src_y, int src_width, int src_height, int opacity) {
		return reinterpret_cast<Bitmap*>(ptr)->stretchBlt(IntRect(dst_x, dst_y, dst_width, dst_height), *reinterpret_cast<Bitmap*>(src_bitmap), IntRect(src_x, src_y, src_width, src_height), opacity);
	}
	
	void mkxpBitmapFillRect(void* ptr, int x, int y, int width, int height, int r, int g, int b, int a) {
		return reinterpret_cast<Bitmap*>(ptr)->fillRect(x, y, width, height, Vec4(r, g, b, a));
	}
	
	void mkxpBitmapGradientFillRect(void* ptr, int x, int y, int width, int height, int r1, int g1, int b1, int a1, int r2, int g2, int b2, int a2, int vertical) {
		return reinterpret_cast<Bitmap*>(ptr)->gradientFillRect(x, y, width, height, Vec4(r1, g1, b1, a1), Vec4(r2, g2, b2, a2), vertical);
	}
	
	void mkxpBitmapClear(void* ptr) {
		return reinterpret_cast<Bitmap*>(ptr)->clear();
	}
	
	void mkxpBitmapClearRect(void* ptr, int x, int y, int width, int height) {
		return reinterpret_cast<Bitmap*>(ptr)->clearRect(x, y, width, height);
	}
	
	FFIColor mkxpBitmapGetPixel(void* ptr, int x, int y) {
		Color c = reinterpret_cast<Bitmap*>(ptr)->getPixel(x, y);
		return {static_cast<int>(c.getRed()), static_cast<int>(c.getGreen()), static_cast<int>(c.getBlue()), static_cast<int>(c.getAlpha())};
	}
	
	void mkxpBitmapSetPixel(void* ptr, int x, int y, int r, int g, int b, int a) {
		return reinterpret_cast<Bitmap*>(ptr)->setPixel(x, y, Vec4(r, g, b, a));
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
	
	void mkxpBitmapDrawText(void* ptr, int x, int y, int width, int height, const char* str, int align) {
		return reinterpret_cast<Bitmap*>(ptr)->drawText(x, y, width, height, str, align);
	}
	
	FFIRect mkxpBitmapTextSize(void* ptr, const char* str) {
		IntRect r = reinterpret_cast<Bitmap*>(ptr)->textSize(str);
		return {r.x, r.y, r.w, r.h};
	}
	
	void* mkxpBitmapGetFont(void* ptr) {
		return &reinterpret_cast<Bitmap*>(ptr)->getFont();
	}
	
	void mkxpBitmapSetFont(void* ptr, void* font) {
		return reinterpret_cast<Bitmap*>(ptr)->setFont(reinterpret_cast<FFIFont*>(font)->font);
	}
}
