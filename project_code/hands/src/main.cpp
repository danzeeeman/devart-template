#include "ofHands.h"
#include "ofAppGLFWWindow.h"
#include "ofxWatchdog.h"
//--------------------------------------------------------------
int main(){
ofxWatchdog::watch(3000, true, true, true);
    ofAppGLFWWindow window;
    window.setDoubleBuffering(true);
    window.setNumSamples(4);
	//ofAppGlutWindow window; // create a window
	// set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
	ofSetupOpenGL(&window,1280, 720, OF_FULLSCREEN);
	ofRunApp(new ofHands()); // start the app
}
