#include "ofHands.h"
using namespace ofxCv;
using namespace cv;
cv::Mat background, foreground;
//--------------------------------------------------------------
void ofHands::setup(){
    mCamWidth = 320;
    mCamHeight = 240;
    
    cam.initGrabber(mCamWidth, mCamHeight);
    mEncoder.setup(mCamWidth, mCamHeight);
    mEncoder.setFrameDuration(0.1);
    mEncoder.setNumColors(255);
    bIsRecording = false;
    mTimer.setup(3000);
    ofAddListener(mTimer.TIMER_COMPLETE, this, &ofHands::timerEnded);
    mImg.allocate(mCamWidth, mCamHeight, OF_IMAGE_GRAYSCALE);
    mThredhold.allocate(mCamWidth, mCamHeight, OF_IMAGE_GRAYSCALE);
    mBackground.allocate(mCamWidth, mCamHeight, OF_IMAGE_GRAYSCALE);
    mOut.allocate(mCamWidth, mCamHeight, OF_IMAGE_COLOR);
    mOut2.allocate(mCamWidth, mCamHeight, OF_IMAGE_COLOR_ALPHA);
    mOut3.allocate(mCamWidth, mCamHeight, OF_IMAGE_COLOR_ALPHA);
    mCurrentPos = -1;
    mSize = 0;
    mViewPort = ofRectangle(0, 0, 1280, 720);
    mMaxPlayers = (mViewPort.width/mCamWidth)*(mViewPort.height/mCamHeight);
    mScreenSkew = ofPoint((ofGetScreenWidth()- mCamWidth*(mViewPort.width/mCamWidth))/2, (ofGetScreenHeight() - mCamHeight*(mViewPort.height/mCamHeight))/2);
    
    
    mContourFinder.setMinArea(mCamWidth*mCamHeight/4);
    mContourFinder.setMinAreaRadius(10);
	mContourFinder.setMaxAreaRadius(mCamWidth/2);
    mContourFinder.setMaxArea(mCamWidth*mCamHeight);
	mContourFinder.setThreshold(25);
	mContourFinder.getTracker().setPersistence(30);
    mContourFinder.setSimplify(true);
	mContourFinder.getTracker().setMaximumDistance(32);
    ofxCv::imitate(mImg, mBackground);
    ofxCv::imitate(mImg, mThredhold);
    
    loadWarp();
    
    for(int i = 0; i < mMaxPlayers; i++){
        mPlayerPos.push_back(ofPoint(mViewPort.width/2, mViewPort.height/2));
        mScale.push_back(1);
        mRotate.push_back(0);
    }
    
    setupGui();
    ofAddListener(ofxGifEncoder::OFX_GIF_SAVE_FINISHED, this, &ofHands::gifSaved);
    
    ofxXmlSettings saves;
    saves.load("backup.xml");
    saves.pushTag("file");
    for(int i = 0; i < saves.getNumTags("path"); i++){
        mFileBuffer.push_back(saves.getValue("path", "", i));
    }
    saves.popTag();
    bLearn = true;
    mMode = OF_BLENDMODE_ALPHA;
}

void ofHands::setupGui(){
    
    modes.push_back("ADD");
    modes.push_back("SCREEN");
    modes.push_back("ALPHA");
    
    
    ofxUISuperCanvas* foogui = new ofxUISuperCanvas("HANDS");
    foogui->addSpacer();
    foogui->addLabel("Press 'h' to Hide GUIs", OFX_UI_FONT_SMALL);
    foogui->addSpacer();
    foogui->addSlider("threshold", 0, 255, 20);
    foogui->addButton("Background", false);
    foogui->addButton("Record", false);
    foogui->addToggle("Show Warp", false);
    foogui->addDropDownList("Blend Mode", modes);
    foogui->addSpacer();
    foogui->addImage("Live", &mImg);
    foogui->addImage("Output", &mOut);
    foogui->addImage("Thres", &mThredhold);
    foogui->addImage("Background", &mBackground);
    foogui->addImage("Test", &mOut2);
    foogui->addToggle("Show", false);
    foogui->autoSizeToFitWidgets();
    foogui->setPosition(0, 0);
    ofAddListener(foogui->newGUIEvent,this,&ofHands::guiEvent);
    int x = foogui->getRect()->width;
    int y = 0;
    for(int i = 0; i < mMaxPlayers; i++){
        ofxUISuperCanvas* foogui2 = new ofxUISuperCanvas("player"+ofToString(i));
        foogui2->add2DPad(ofToString(i), ofPoint(0, mViewPort.width), ofPoint(0,mViewPort.height), ofPoint(mViewPort.width/2, mViewPort.height/2));
        foogui2->addSlider("alpha"+ofToString(i), 0, 255, 255);
        foogui2->addSlider("scale"+ofToString(i), 0.5, 10, 1);
        foogui2->addSlider("rotate"+ofToString(i), 0, 360, 1);
        mAlpha.push_back(255);
        foogui2->setPosition(x, y);
        foogui2->autoSizeToFitWidgets();
        ofAddListener(foogui2->newGUIEvent,this,&ofHands::guiEvent);
        gui.push_back(foogui2);
        
        y+=foogui2->getRect()->height;
        if(x > ofGetWidth()){
            x=0;
        }
        if(y+foogui2->getRect()->height > 720){
            x+=210;
            y=0;
        }
    }
    gui.push_back(foogui);
    
    for(int i = 0; i < gui.size(); i++){
        gui[i]->loadSettings("settings/gui-"+ofToString(i));
    }
}

void ofHands::guiEvent(ofxUIEventArgs &e){
    string name = e.getName();
	int kind = e.getKind();
    
    if(kind == OFX_UI_WIDGET_2DPAD){
        ofStringReplace(name, "player", "");
        int index = ofToInt(name);
        cout<<name<<endl;
        ofxUI2DPad* foo = (ofxUI2DPad *)e.widget;
        mPlayerPos[index].set(foo->getScaledValue().x, foo->getScaledValue().y);
    }
    if(name == "Record"){
        ofxUIButton* b = e.getButton();
        if(b->getValue()){
            keyPressed('r');
        }
    }
    if(name == "Background"){
        ofxUIButton* b = e.getButton();
        if(b->getValue()){
            bLearn = true;
            
        }
    }
    if(name == "Show"){
        ofxUIToggle* b = e.getToggle();
        for(int i = 0; i < gui.size(); i++){
            gui[i]->setVisible(b->getValue());
        }
        
    }
    if(name == "Show Warp"){
        ofxUIToggle* b = e.getToggle();
        if(b->getValue()){
            mWarp.show();
            mWarp.enable();
        }else{
            mWarp.hide();
            mWarp.disable();
        }
        
    }
    if(kind == OFX_UI_WIDGET_SLIDER_H){
        if(name == "threshold"){
            ofxUISlider* s = e.getSlider();
            thres = s->getValue();
        }else{
            if(ofIsStringInString(name, "alpha")){
                ofStringReplace(name, "alpha", "");
                int index = ofToInt(name);
                mAlpha[index] = e.getSlider()->getValue();
            }else if(ofIsStringInString(name, "scale")){
                ofStringReplace(name, "scale", "");
                int index = ofToInt(name);
                mScale[index] = e.getSlider()->getValue();
            }else if(ofIsStringInString(name, "rotate")){
                ofStringReplace(name, "rotate", "");
                int index = ofToInt(name);
                mRotate[index] = e.getSlider()->getValue();
            }
        }
    }
    if(name == "Blend Mode"){
        ofxUIDropDownList* b = (ofxUIDropDownList*)e.widget;
        vector<string> foo = b->getSelectedNames();
        if(foo.size() > 0){
            if(foo[0] == "ADD"){
                mMode = OF_BLENDMODE_ADD;
            }else if(foo[0] == "SCREEN"){
                mMode = OF_BLENDMODE_SCREEN;
            }else if(foo[0] == "ALPHA"){
                mMode = OF_BLENDMODE_ALPHA;
            }
        }
    }
}


void ofHands::gifSaved(string & fileName){
    cout<<"gif saved now upload it"<<fileName<<endl;
    mEncoder.reset();
    bSaving = false;
}

//--------------------------------------------------------------
void ofHands::update(){
    cam.update();
	
	if (cam.isFrameNew()){
        if(bLearn){
            convertColor(cam, mBackground, CV_RGB2GRAY);
            mBackground.update();
            copy(cam, mOut);
            background = toCv(mBackground);
            bLearn = false;
        }
        
        convertColor(cam, mImg, CV_RGB2GRAY);
        mImg.update();
        cv::subtract(toCv(mImg), background, foreground);
        
        mImg.update();
        toOf(foreground, mImg);
        blur(mImg, 25);
        threshold(mImg, mThredhold, thres);
        mThredhold.update();
        convertColor(cam, mImg, CV_RGB2GRAY);
        mImg.update();
        if(bIsRecording){
            mContourFinder.findContours(mThredhold);
            ofPixels & pix = mImg.getPixelsRef();
            ofPixels & thresPix = mThredhold.getPixelsRef();
            
            for(int i = 0; i<mImg.width*mImg.height; i++){
                if(thresPix[i] != 255){
                    pix[i] = 0;
                }else{
                    pix[i] = pix[i];
                }
            }
            mImg.update();
            mThredhold.update();
            convertColor(mImg, mOut, CV_GRAY2RGB);
            mOut.update();
            int count = 0;
            int count2 = 0;
            ofPixels & pix2 = mOut2.getPixelsRef();
            thresPix = mThredhold.getPixelsRef();
            ofPixels & pix3 = mOut.getPixelsRef();
            for(unsigned int i = 0; i<mThredhold.width*mThredhold.height; i++){
                if(thresPix[i] != 255){
                    pix2[count] = 0;
                    pix2[count+1] = 0;
                    pix2[count+2] = 0;
                    pix2[count+3] = 0;
                }else{
                    pix2[count] = pix3[count2];
                    pix2[count+1] = pix3[count2+1];
                    pix2[count+2] = pix3[count2+2];
                    pix2[count+3] = 255;
                }
                count=count+4;
                count2=count2+3;
            }
            
            mOut2.update();
            
            mEncoder.addFrame(mOut.getPixels(), mOut.getWidth(), mOut.getHeight(), 24, 0.1);
            mRecorder.addFrame(mOut2);
        }
    }
    
    if(mFileBuffer.size() > 0){
        mCurrentPos++;
        if(mCurrentPos >= mPlayers.size())
            mCurrentPos = 0;
        
        ofxImageSequence * fooPlayer = new ofxImageSequence();
        if(ofIsStringInString(mFileBuffer.front(), "png-sequence/")){
            fooPlayer->loadSequence(mFileBuffer.front());
        }else{
             fooPlayer->loadSequence("png-sequence/"+mFileBuffer.front());
        }
        fooPlayer->setFrameRate(ofRandom(10, 24));
        mPlayers.push_front(fooPlayer);
        if(mPlayers.size() > mMaxPlayers){
            ofxImageSequence * player = mPlayers.back();
            mPlayers.pop_back();
            delete player;
        }
        string foo = mFileBuffer.front();
        mFileList.push_back(foo);
        if(mFileList.size()> 12){
            mFileList.pop_front();
        }
        mFileBuffer.pop_front();
    }
}

void ofHands::timerEnded(int & args){
    bIsRecording = false;
    bSaving = true;
    mFileBuffer.push_back(mCurrentFile);
    mEncoder.save("gifs/"+mCurrentFile+"_upload"+".gif");
    if(mRecorder.isThreadRunning() && mRecorder.q.size() == 0){
        mRecorder.stopThread();
    }
    
    
}
//--------------------------------------------------------------
void ofHands::draw(){
    ofBackground(0, 0, 0);
    
    
    //rofTranslate(mScreenSkew);
    ofPushMatrix();
    {
        glPushMatrix();
        glMultMatrixf( mMatrix.getPtr());
        {
            ofPushStyle();
            ofPushMatrix();
            ofEnableBlendMode(mMode);
            ofTranslate(mScreenSkew);
            for(int i = 0; i < mPlayers.size(); i++){
                ofPushStyle();
                ofSetColor(255, 255, 255, mAlpha[i]);
                ofPushMatrix();
                ofTranslate(mPlayerPos[i]);
                ofRotate(mRotate[i]);
                ofPushMatrix();
                ofScale(mScale[i], mScale[i]);
                mPlayers[i]->getFrameForTime(ofGetElapsedTimef())->draw(-mCamWidth/2, -mCamHeight/2);
                ofPopMatrix();
                ofPopStyle();
                ofPopMatrix();
            }
            ofPopMatrix();
            ofPopStyle();
        }
        glPopMatrix();
    }
    ofPopMatrix();
    

    mWarp.draw();
    ofPopMatrix();
}


void ofHands::updateWarp(){
    mMatrix = mWarp.getMatrix();
    saveWarp();
}

void ofHands::loadWarp(){
    
    if(ofFile::doesFileExist("settings/.targetpointsMatrix-0")){
        vector<ofPoint> sp;
        vector<ofPoint> tp;
        for(int j = 0 ; j < 4; j++){
            string file2 = "settings/.targetpointsMatrix-"+ofToString(j);
            string file3 = "settings/.sourcepointsMatrix-"+ofToString(j);
            if(ofFile::doesFileExist(file2)){
                ofPoint pts;
                ofFile pointsRead(file2);
                pointsRead>>pts;
                pointsRead.close();
                tp.push_back(pts);
            }
            if(ofFile::doesFileExist(file3)){
                ofPoint pts;
                ofFile pointsRead(file3);
                pointsRead>>pts;
                pointsRead.close();
                sp.push_back(pts);
            }
        }
        mWarp.setSourceRect(mViewPort);
        mWarp.setSourcePoints(sp);
        mWarp.setTargetPoints(tp);
        mWarp.setup();
        updateWarp();
        mWarp.disable();
        mWarp.hide();
    }else{
        ofLog(OF_LOG_NOTICE)<<"No Files"<<endl;
        mWarp.setSourceRect(mViewPort);
        mWarp.setTopLeftCornerPosition(ofPoint(mViewPort.x, mViewPort.y));
        mWarp.setBottomLeftCornerPosition(ofPoint(mViewPort.x, mViewPort.y+mViewPort.height));
        mWarp.setTopRightCornerPosition(ofPoint(mViewPort.x+mViewPort.width, mViewPort.y));
        mWarp.setBottomRightCornerPosition(ofPoint(mViewPort.x+mViewPort.width, mViewPort.y+mViewPort.height));
        mWarp.setup();
        updateWarp();
    }
}


void ofHands::saveWarp(){
    
    string file = "settings/.warpMatrix";
    
    ofFile fileWriter(file, ofFile::WriteOnly);
    fileWriter<<mMatrix;
    fileWriter.close();
    
    ofPoint * sp = mWarp.getSourcePoints();
    ofPoint * tp = mWarp.getTargetPoints();
    for(int j = 0; j < 4; j++){
        string file2 = "settings/.targetpointsMatrix-"+ofToString(j);
        ofFile pointsWriter(file2, ofFile::WriteOnly);
        pointsWriter<<tp[j];
        pointsWriter.close();
        string file3 = "settings/.sourcepointsMatrix-"+ofToString(j);
        ofFile sourceWriter(file3, ofFile::WriteOnly);
        sourceWriter<<sp[j];
        sourceWriter.close();
    }
}


void ofHands::exit(){
    mRecorder.waitForThread();
    mEncoder.waitForThread();
    for(int i = 0; i < gui.size(); i++){
        gui[i]->saveSettings("gui-"+ofToString(i));
    }
    
    if(mFileList.size() == mMaxPlayers){
        ofxXmlSettings saves;
        saves.addTag("file");
        saves.pushTag("file");
        for(int i = 0; i < mFileList.size(); i++){
            
            saves.addValue("path", mFileList[i]);
            
        }
        saves.popTag();
        saves.save("backup.xml");
    }
}

//--------------------------------------------------------------
void ofHands::keyPressed(int key){
    if(key == 'r' && !bIsRecording && !bSaving){
        
        mCurrentFile = ofGetTimestampString();
        ofDirectory::createDirectory(ofToDataPath("png-sequence/"+mCurrentFile));
        mRecorder.setPrefix(ofToDataPath("png-sequence/"+mCurrentFile+"/frame_"));
        mRecorder.setFormat("png");
        mTimer.start(false);
        if(!mRecorder.isThreadRunning())
            mRecorder.startThread(false, false);
        
        
        
        bIsRecording = !bIsRecording;
    }
    if(key == 'h'){
        for(int i = 0; i < gui.size(); i++){
            gui[i]->toggleVisible();
        }
    }
    if(key == ' '){
        mFullScreen = !mFullScreen;
        ofSetFullscreen(mFullScreen);
        if(mFullScreen)
            mScreenSkew = ofPoint((ofGetScreenWidth() - (mCamWidth*(mViewPort.width/mCamWidth)))/2, (ofGetScreenHeight() - (mCamHeight*(mViewPort.height/mCamHeight)))/2);
        else
            mScreenSkew = ofPoint((mViewPort.width - (mCamWidth*mViewPort.width/mCamWidth))/2, (mViewPort.height - (mCamHeight*mViewPort.height/mCamHeight))/2);
    }
    if(key == 'c'){
        mWarp.reset();
        mWarp.show();
        mWarp.enable();
        
        mViewPort =ofRectangle(mScreenSkew.x, mScreenSkew.y, mCamWidth*(mViewPort.width/mCamWidth), mCamHeight*(mViewPort.height/mCamHeight));
        mWarp.setSourceRect(mViewPort);
        mWarp.setTopLeftCornerPosition(ofPoint(mViewPort.x, mViewPort.y));
        mWarp.setBottomLeftCornerPosition(ofPoint(mViewPort.x, mViewPort.y+mViewPort.height));
        mWarp.setTopRightCornerPosition(ofPoint(mViewPort.x+mViewPort.width, mViewPort.y));
        mWarp.setBottomRightCornerPosition(ofPoint(mViewPort.x+mViewPort.width, mViewPort.y+mViewPort.height));
    }
    if(key == '='){
        updateWarp();
    }
}

//--------------------------------------------------------------
void ofHands::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofHands::mouseMoved(int x, int y){
    
}

//--------------------------------------------------------------
void ofHands::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofHands::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofHands::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofHands::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofHands::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofHands::dragEvent(ofDragInfo dragInfo){
    
}