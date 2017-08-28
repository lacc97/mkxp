#include "binding-util.h"

#include "sharedstate.h"
#include "filesystem.h"
#include "util.h"
#include "exception.h"

#include <cstdlib>

extern "C" {
	int mkxpFileExists(const char* file) {
		return shState->fileSystem().exists(file);
	}
	
    void* mkxpIOInitialize(const char* filename) {
        SDL_RWops* ops = SDL_AllocRW();

        try {
            shState->fileSystem().openReadRaw(*ops, filename);
        } catch(const Exception& e) {
            SDL_FreeRW(ops);

            throw e;
        }
        
        return ops;
    }

    void* mkxpIORead(void* ptr, int* success, int length) {
        if(!ptr) {
            *success = 0;
            return NULL;
        }

        if(length == 0) {
			*success = 1;
            return NULL;
		}

        SDL_RWops* ops = reinterpret_cast<SDL_RWops*>(ptr);

        char* data = reinterpret_cast<char*>(malloc(length));

        *success = (SDL_RWread(ops, data, 1, length) == length);

        return data;
    }

    int mkxpIOLengthToEnd(void* ptr) {
        SDL_RWops* ops = reinterpret_cast<SDL_RWops*>(ptr);

        Sint64 cur = SDL_RWtell(ops);
        Sint64 end = SDL_RWseek(ops, 0, SEEK_END);
        int length = end - cur;
        SDL_RWseek(ops, cur, SEEK_SET);

        return length;
    }

    uint8_t mkxpIOReadByte(void* ptr, int* success) {
        if(!ptr) {
            *success = 0;
            return 0;
        }

        unsigned char byte;
        *success = (SDL_RWread(reinterpret_cast<SDL_RWops*>(ptr), &byte, 1, 1) == 1);
        return byte;
    }

    void mkxpIOClose(void* ptr) {
        if(ptr) {
            SDL_RWops* ops = reinterpret_cast<SDL_RWops*>(ptr);
            SDL_RWclose(ops);
            SDL_FreeRW(ops);
        }
    }
}
