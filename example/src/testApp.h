#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxKinectMemoryMapped.h"

class testApp : public ofBaseApp 
{
    public:

        void setup();
        void update();
        void draw();
        void exit();

        void keyPressed(int key);
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void windowResized(int w, int h);

	private:	
		ofxKinectMemoryMapped kinectMemoryMapped;
};
