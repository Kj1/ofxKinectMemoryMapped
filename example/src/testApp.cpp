#include "testApp.h"


void testApp::setup() 
{
	//OF basics
    ofSetFrameRate(60);
    ofBackground(100);
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetWindowTitle("Kinect Memory mapped demo");

	//get list of kinects
	vector<string> kinects = kinectMemoryMapped.getKinects();
	
	//list them
	for (int i = 0; i < kinects.size(); i++) {
		ofLog(OF_LOG_VERBOSE, "Kinect found: " + kinects[i]);
	}


	//just setup the first kinect
	if (kinects.size() > 0)
		kinectMemoryMapped.setup(kinects[0]);
	else 
		ofLog(OF_LOG_VERBOSE, "No kinects found");

}

void testApp::update() 
{	
	kinectMemoryMapped.update();
}

void testApp::draw() 
{    
    ofBackground(0);	
	ofSetColor(255);	
	ofSetWindowTitle("OF FR:" + ofToString(ofGetFrameRate()) + "; Kinect FPS: " + ofToString(kinectMemoryMapped.getFPS()));
	kinectMemoryMapped.getRGBImage().draw(0,0);
	kinectMemoryMapped.getDepthImage().draw(640,0);
	kinectMemoryMapped.getWorldImage().draw(0,480);
	kinectMemoryMapped.getLabelImage().draw(640,480);

}

void testApp::exit() 
{	
}

void testApp::keyPressed (int key) 
{
}

void testApp::mousePressed(int x, int y, int button)
{
}

void testApp::mouseDragged(int x, int y, int button)
{
}

void testApp::mouseReleased(int x, int y, int button)
{
}

void testApp::windowResized(int w, int h)
{
}
