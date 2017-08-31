#include "binding-util.h"

#include "sharedstate.h"
#include "input.h"

extern "C" {
    void mkxpInputUpdate() {
        return shState->input().update();
    }
    
    int mkxpInputPress(const char* b) {
        return shState->input().isPressed(buttonMap[b]);
    }
    
    int mkxpInputTrigger(const char* b) {
        return shState->input().isTriggered(buttonMap[b]);
    }
    
    int mkxpInputRepeat(const char* b) {
        return shState->input().isRepeated(buttonMap[b]);
    }
    
    int mkxpInputDir4() {
        return shState->input().dir4Value();
    }
    
    int mkxpInputDir8() {
        return shState->input().dir8Value();
    }
}
