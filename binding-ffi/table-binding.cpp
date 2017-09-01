#include "binding-util.h"

#include "table.h"

extern "C" {
	BINDING_CONSTRUCTOR(Table)(int xsize, int ysize, int zsize) {
		return new Table(xsize, ysize, zsize);
	}
	BINDING_COPY_CONSTRUCTOR(Table)
	BINDING_DESTRUCTOR(Table)
	
	BINDING_METHOD(Table, void, Resize)(Table* ptr, int x, int y, int z) {
		ptr->resize(x, y, z);
	}
	BINDING_METHOD(Table, int, Xsize)(Table* ptr) {
		return ptr->xSize();
	}
	BINDING_METHOD(Table, int, Ysize)(Table* ptr) {
		return ptr->ySize();
	}
	BINDING_METHOD(Table, int, Zsize)(Table* ptr) {
		return ptr->zSize();
	}
	BINDING_METHOD(Table, int, Get)(Table* ptr, int x, int y, int z) {
		return ptr->at(x, y, z);
	}
	BINDING_METHOD(Table, int, Set)(Table* ptr, int x, int y, int z, int val) {
		ptr->at(x, y, z) = val;
	}
	
	Table* mkxpTableDeserialize(char* ptr, int len) {
        return Table::deserialize(ptr, len);
    }
}
