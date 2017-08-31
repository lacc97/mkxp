#include "binding-util.h"

#include "sharedstate.h"
#include "graphics.h"

#define BINDING_MODULE_PROPERTY(mod, intmod, propType, propName, libPropName) \
	propType mkxp##mod##Get##propName() { \
		return shState->intmod().get##libPropName(); \
	} \
	void mkxp##mod##Set##propName(propType val) {\
		shState->intmod().set##libPropName(val); \
	}

extern "C" {
	BINDING_MODULE_PROPERTY(Graphics, graphics, int, FrameRate, FrameRate)
	BINDING_MODULE_PROPERTY(Graphics, graphics, int, FrameCount, FrameCount)
	BINDING_MODULE_PROPERTY(Graphics, graphics, int, Brightness, Brightness)
	
	void mkxpGraphicsUpdate() {
		shState->graphics().update();
	}
	
	void mkxpGraphicsWait(int d) {
		shState->graphics().wait(d);
	}
	
	void mkxpGraphicsFadeout(int d) {
		shState->graphics().fadeout(d);
	}
	
	void mkxpGraphicsFadein(int d) {
		shState->graphics().fadein(d);
	}
	
	void mkxpGraphicsFreeze() {
		shState->graphics().freeze();
	}
	
	void mkxpGraphicsTransition(int duration, const char* filename, int vague) {
		shState->graphics().transition(duration, filename, vague);
	}
	
	Bitmap* mkxpGraphicsSnapToBitmap() {
		return shState->graphics().snapToBitmap();
	}
	
	void mkxpGraphicsFrameReset() {
		shState->graphics().frameReset();
	}
	
	int mkxpGraphicsWidth() {
//         std::cout << "Graphics Width = " << shState->graphics().width() << std::endl;
		return shState->graphics().width();
	}
	
	int mkxpGraphicsHeight() {
//         std::cout << "Graphics Height = " << shState->graphics().height() << std::endl;
		return shState->graphics().height();
	}
	
	void mkxpGraphicsResizeScreen(int w, int h) {
		shState->graphics().resizeScreen(w, h);
	}
	
	void mkxpGraphicsPlayMovie(const char* filename) {
		shState->graphics().playMovie(filename);
	}
}
