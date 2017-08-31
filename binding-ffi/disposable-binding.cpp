#include "disposable.h"

extern "C" {
	void mkxpDisposableDispose(void* obj) {
		if(obj) {
			Disposable* disp = static_cast<Disposable*>(obj);
			disp->dispose();
		}
	}
	
	int mkxpDisposableDisposed(void* obj) {
		if(obj)
			return static_cast<Disposable*>(obj)->isDisposed();
		
		return true;
	}
}
