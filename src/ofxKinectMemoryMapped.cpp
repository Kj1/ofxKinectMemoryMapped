#include "ofxKinectMemoryMapped.h"
//TODO input FPS
//TODO High performance update

//lookup tables

const unsigned char userLookupR [7] = { 0, 255, 0,   0,   255, 0,   255 };
const unsigned char userLookupG [7] = { 0, 0,   255, 0,   255, 255, 0 };
const unsigned char userLookupB [7] = { 0, 0,   0,   255, 0,   255, 255 };
const unsigned char userLookupA [7] = { 0, 255, 255, 255, 255, 255, 255};

ofxKinectMemoryMapped::ofxKinectMemoryMapped() {
	memoryReady = false;	
	width = 640;
	height = 480;
	frameNr = 0;
	fpsTimerFrameNum = 0;
	fpsTimer = 0;
	fps;
	newFrame = false;
	colorImage.allocate(width,height);
	worldImage.allocate(width,height);
	labelImage.allocate(width,height);
	depthImage.allocate(width,height);
}

vector<string>		ofxKinectMemoryMapped::getKinects() {
	vector<string> aviablable;
	for (int i = 0; i < 10; i++) {
		string kinectname = "kinect"+ofToString(i);
		std::wstring stemp = std::wstring(kinectname.begin(), kinectname.end());
		LPCWSTR sw = stemp.c_str();

		HANDLE hMapFile;
		LPCTSTR pBuf;
		hMapFile = OpenFileMapping(FILE_MAP_READ,FALSE,sw);               
		if (hMapFile == NULL){
			continue;
		}
		//try to read out
	  	char* pixelsRcv = (char *) MapViewOfFile(hMapFile,FILE_MAP_READ,0,0, 100 + 640*480*11);
		if (pixelsRcv == NULL){
			ofLog(OF_LOG_ERROR, "Unable to create map view: error " + ofToString(GetLastError()));
		} else {
			UnmapViewOfFile(pixelsRcv);
			aviablable.push_back(kinectname);
		}
		CloseHandle(hMapFile);
	}
	return aviablable;
}

bool				ofxKinectMemoryMapped::setup(string _memoryName){
	memoryName = _memoryName;
	std::wstring stemp = std::wstring(memoryName.begin(), memoryName.end());
	LPCWSTR memoryLocation = stemp.c_str();
	HANDLE hMapFile;
	LPCTSTR pBuf;
	hMapFile = OpenFileMapping(FILE_MAP_READ,FALSE,memoryLocation);               
	if (hMapFile == NULL){
		ofLog(OF_LOG_ERROR, "Unable to create map view: error " + ofToString(GetLastError()));
		memoryReady = false;
		CloseHandle(hMapFile);
		return false;
	}

	//try to read out
	char* pixelsRcv = (char *) MapViewOfFile(hMapFile,FILE_MAP_READ,0,0, 100 + 640*480*11);
	if (pixelsRcv == NULL){
		ofLog(OF_LOG_ERROR, "Unable to create map view: error " + ofToString(GetLastError()));
		memoryReady = false;		
		UnmapViewOfFile(pixelsRcv);
		CloseHandle(hMapFile);
		return false;
	} 
	
	UnmapViewOfFile(pixelsRcv);
	CloseHandle(hMapFile);

	//readout ok!
	depthPixelsReceived = new short[width*height * 1]; //grayscale
	colorPixelsReceived = new unsigned char[width*height * 3]; //RGB
	worldPixelsReceived = new short[width*height * 3]; //RGB
	labelPixelsReceived = new unsigned char[width*height * 1]; //RGBA

	worldPixelsReceivedSimple = new unsigned char[width*height * 3]; //RGB
	depthPixelsReceivedSimple = new unsigned char[width*height * 1]; //RGB
	labelPixelsReceivedSimple = new unsigned char[width*height * 4]; //RGBA
	memoryReady = true;
	fpsTimer = ofGetElapsedTimeMillis();
	return true;
}

bool				ofxKinectMemoryMapped::isReady(){
	return memoryReady;
}

bool ofxKinectMemoryMapped::isNewFrame() {
	bool b = newFrame;
	newFrame = false;
	return b;
}

int ofxKinectMemoryMapped::getFPS() {
	return fps;
}

void				ofxKinectMemoryMapped::update(){
	if (!memoryReady) return;

	long startTime = ofGetElapsedTimeMillis();
	HANDLE hMapFile;	
	std::wstring stemp = std::wstring(memoryName.begin(), memoryName.end());
	LPCWSTR memoryLocation = stemp.c_str();

	hMapFile = OpenFileMapping(FILE_MAP_READ,FALSE,memoryLocation);               
	if (hMapFile == NULL){
		ofLog(OF_LOG_ERROR, "Could not open file mapping object: error " + ofToString(GetLastError()));
		return;
	}

   	char* pixelsRcv = (char *) MapViewOfFile(hMapFile,FILE_MAP_READ,0,0, 100 + 640*480*11);
	if (pixelsRcv == NULL){
		ofLog(OF_LOG_ERROR, "Unable to create map view: error " + ofToString(GetLastError()));
		CloseHandle(hMapFile);
		return;
	}
	
	//vars
	int currentPixel = 0;
	int offset = 100;		
	int user = 0;
	int realDepth = 0;
	short shortTmp = 0;

	//string 
	offset = 0;
	char firstLine [50];// = new char [100];
	for (int j = 0; j < 100;j+=2) {	
		firstLine[j/2] = pixelsRcv[offset+j];
	}
	string fn = string(firstLine);
	string framenr = fn.substr(fn.find("FN")+2);
	long frameNrNew = atol(framenr.c_str());
	
	if (frameNr == frameNrNew) {
		//no update needed!
		//unmap & close		
		UnmapViewOfFile(pixelsRcv);
		CloseHandle(hMapFile);
		return;

	}
	frameNr = frameNrNew;
	

	//color pixels
	offset = 100;		
	memcpy(colorPixelsReceived,pixelsRcv+offset, 640*480*3);
			
	//depth & label pixels
	offset += 640*480*3;
	int jj = 0;
	for (int j = 0; j < 640*480;j++) {					
		shortTmp = *(short *)(pixelsRcv+offset+(j*2));				
        user = shortTmp & 0x07;
        realDepth = (shortTmp >> 3);
		if (realDepth > 8192) realDepth = 8192;
		depthPixelsReceived[j] = realDepth;
		labelPixelsReceived[j] = (unsigned char) user;		
		depthPixelsReceivedSimple[j] = realDepth/32; 		
		labelPixelsReceivedSimple[jj++] = userLookupR[user];  //r
		labelPixelsReceivedSimple[jj++] = userLookupG[user];  //g
		labelPixelsReceivedSimple[jj++] = userLookupB[user];	//b			
		labelPixelsReceivedSimple[jj++] = userLookupA[user];	//a

	}
	
	//world image floats
	offset += 640*480*2;
	currentPixel = 0;	
	memcpy(worldPixelsReceived,pixelsRcv+offset, 640*480*6);
	
	//simplify world image for visibality
	for (int j = 0; j < 640*480;j++) {	
		currentPixel = j*3;
		worldPixelsReceivedSimple[currentPixel] = (worldPixelsReceived[currentPixel++]+3000)/24;
		worldPixelsReceivedSimple[currentPixel] = (worldPixelsReceived[currentPixel++]+3000)/24;
		worldPixelsReceivedSimple[currentPixel] = (worldPixelsReceived[currentPixel])/32;
	}

	//unmap & close	& release	
	UnmapViewOfFile(pixelsRcv);
	CloseHandle(hMapFile);
	
	//parse to images
	colorImage.setFromPixels(colorPixelsReceived,width,height);
	depthImage.setFromPixels(depthPixelsReceivedSimple,width,height);
	worldImage.setFromPixels(worldPixelsReceivedSimple,width,height);
	labelImage.setFromPixels(labelPixelsReceivedSimple,width,height);
	newFrame = true;

	//fps
	fpsTimerFrameNum++;
	long now = ofGetElapsedTimeMillis();
	if (now-fpsTimer > 1000){
		fpsTimer = now;
		fps = fpsTimerFrameNum;
		fpsTimerFrameNum = 0;
	}
	cout << "update: " << ofToString(ofGetElapsedTimeMillis()-startTime) << endl;
}

ofxCvColorImage		ofxKinectMemoryMapped::getRGBImage(){
	return colorImage;
}

ofxCvGrayscaleImage	ofxKinectMemoryMapped::getDepthImage(){
	return depthImage;
}

ofxCvColorImage		ofxKinectMemoryMapped::getWorldImage(){
	return worldImage;
}

ofxCvColorImageAlpha	ofxKinectMemoryMapped::getLabelImage() {
	return labelImage;
}

