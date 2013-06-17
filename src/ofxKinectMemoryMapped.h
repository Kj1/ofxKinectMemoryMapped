#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCvColorImageAlpha.h"

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#pragma comment(lib, "user32.lib")

class ofxKinectMemoryMapped {
public:
	//initialisation
	ofxKinectMemoryMapped();
	vector<string>		getKinects();
	bool				setup(string memoryName);
	bool				isReady();
	bool				isNewFrame();
	int					getFPS();

	//update methods read out the memory
	void				update();

	//image getters
	ofxCvColorImage			getRGBImage();
	ofxCvGrayscaleImage		getDepthImage();
	ofxCvColorImage			getWorldImage();
	ofxCvColorImageAlpha	getLabelImage();

private:
	int width;
	int height;
	long frameNr;
	bool newFrame;
	int fps;
	long fpsTimer;
	int fpsTimerFrameNum;

	//image char arrays
	short*						depthPixelsReceived;
	short*						worldPixelsReceived;
	unsigned char*				labelPixelsReceived;
	unsigned char*				colorPixelsReceived;
	unsigned char*				worldPixelsReceivedSimple;
	unsigned char*				depthPixelsReceivedSimple;	
	unsigned char*				labelPixelsReceivedSimple;	

	ofxCvColorImage				colorImage;
	ofxCvColorImage				worldImage;
	ofxCvColorImageAlpha		labelImage;
	ofxCvGrayscaleImage			depthImage;

	//memory mapped
	string memoryName; 
	bool memoryReady;
};