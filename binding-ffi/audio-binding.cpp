#include "binding-util.h"

#include "sharedstate.h"
#include "audio.h"

extern "C" {
	void mkxpAudioSetupMidi() {
		shState->audio().setupMidi();
	}
	
	void mkxpAudioBgmPlay(const char* filename, int volume, int pitch, float pos) {
		shState->audio().bgmPlay(filename, volume, pitch, pos);
	}
	void mkxpAudioBgmStop() {
		shState->audio().bgmStop();
	}
	float mkxpAudioBgmPos() {
		return shState->audio().bgmPos();
	}
	void mkxpAudioBgmFade(int duration) {
		shState->audio().bgmFade(duration);
	}
	
	void mkxpAudioBgsPlay(const char* filename, int volume, int pitch, float pos) {
		shState->audio().bgsPlay(filename, volume, pitch, pos);
	}
	void mkxpAudioBgsStop() {
		shState->audio().bgsStop();
	}
	float mkxpAudioBgsPos() {
		return shState->audio().bgsPos();
	}
	void mkxpAudioBgsFade(int duration) {
		shState->audio().bgsFade(duration);
	}
	
	void mkxpAudioMePlay(const char* filename, int volume, int pitch) {
		shState->audio().mePlay(filename, volume, pitch);
	}
	void mkxpAudioMeStop() {
		shState->audio().meStop();
	}
	void mkxpAudioMeFade(int duration) {
		shState->audio().meFade(duration);
	}
	
	void mkxpAudioSePlay(const char* filename, int volume, int pitch) {
		shState->audio().sePlay(filename, volume, pitch);
	}
	void mkxpAudioSeStop() {
		shState->audio().seStop();
	}
}
