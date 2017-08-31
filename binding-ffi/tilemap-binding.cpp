#include "binding-util.h"

#include "tilemap.h"
#include "tilemapvx.h"
#include "table.h"

extern "C" {
	BINDING_CONSTRUCTOR(Tilemap)(Viewport* vp) {
		if(shState->rgssVersion > 1)
			return new TilemapVX(vp);
		else
			return new Tilemap(vp);
	}
	BINDING_DESTRUCTOR_VX(Tilemap)
	
	BINDING_PROPERTY_VX(Tilemap, Table*, MapData)
	BINDING_PROPERTY_VX(Tilemap, Table*, FlashData)
	
	BINDING_PROPERTY_VXONLY(Tilemap, Table*, Flags)
	
	Viewport* mkxpTilemapGetViewport(void* ptr) {
		if(shState->rgssVersion > 1)
			return reinterpret_cast<TilemapVX*>(ptr)->getViewport();
		else
			return reinterpret_cast<Tilemap*>(ptr)->getViewport();
	}
	void mkxpTilemapSetViewport(void* ptr, Viewport* t) {
		if(shState->rgssVersion > 1)
			reinterpret_cast<TilemapVX*>(ptr)->setViewport(t);
	}
	
	BINDING_PROPERTY_VX(Tilemap, int, Visible)
	BINDING_PROPERTY_VX_DET(Tilemap, int, Ox, OX)
	BINDING_PROPERTY_VX_DET(Tilemap, int, Oy, OY)
	
	BINDING_METHOD(Tilemap, void, Update)(void* p) {
		if(shState->rgssVersion > 1) {
			reinterpret_cast<TilemapVX*>(p)->update();
		} else {
			reinterpret_cast<Tilemap*>(p)->update();
		}
	}
	
	BINDING_METHOD(Tilemap, Bitmap*, GetBitmap)(void* p, int i) {
		if(shState->rgssVersion > 1) {
			return reinterpret_cast<TilemapVX*>(p)->getBitmapArray().get(i);
		} else {
			return reinterpret_cast<Tilemap*>(p)->getAutotiles().get(i);
		}
	}
	BINDING_METHOD(Tilemap, void, SetBitmap)(void* p, int i, Bitmap* b) {
		if(shState->rgssVersion > 1) {
			reinterpret_cast<TilemapVX*>(p)->getBitmapArray().set(i, b);
		} else {
			reinterpret_cast<Tilemap*>(p)->getAutotiles().set(i, b);
		}
	}
}
