#pragma once

#include "ofMain.h"
#include "ofxImageSequenceRecorder.h"
#include "ofxImageSequence.h"
#include "ofxSimpleTimer.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "ofxQuadWarp.h"
#include "ofxGifEncoder.h"
#include "ofxUI.h"

class ofHands : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void gifSaved(string & fileName);
    void exit();
    void loadWarp();
    void updateWarp();
    void saveWarp();
    void timerEnded(int & args);
    void guiEvent(ofxUIEventArgs &e);
    void setupGui();
    vector<ofxUISuperCanvas*> gui;
    
    ofxGifEncoder mEncoder;
    ofxCv::ContourFinder mContourFinder;
    ofxCv::RunningBackground mBackgnd;
    
    ofxSimpleTimer mTimer;
    ofxImageSequenceRecorder mRecorder;
    string mCurrentFile;
    bool bIsRecording;
    deque<string> mFileBuffer;
    deque<string>mFileList;
    ofVideoGrabber cam;
    ofImage mImg;
    ofImage mOut;
    ofImage mOut2, mOut3;
    ofImage mBackground;
    ofImage mThredhold;
    ofFbo foobar;
    float thres;
    deque<ofxImageSequence*> mPlayers;
    vector<ofPoint> mPlayerPos;
    vector<float> mScale;
    vector<float> mAlpha;
    vector<float> mRotate;
    deque<float> mPos;
    int mCurrentPos;
    int mSize;
    bool bSaving;
    ofBlendMode mMode;
    vector<string> modes;

    int mCamWidth;
    int mCamHeight;
    int mMaxPlayers;
    ofMatrix4x4 mMatrix;
    ofPoint mScreenSkew;
    ofRectangle mViewPort;
    ofxQuadWarp mWarp;
    bool mFullScreen;
    bool bLearn;
};
