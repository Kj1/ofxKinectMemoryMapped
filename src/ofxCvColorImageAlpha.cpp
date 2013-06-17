

#include "ofxCvGrayscaleImage.h"
#include "ofxCvColorImage.h"
#include "ofxCvFloatImage.h"
#include "ofxCvShortImage.h"
#include "ofxCvColorImageAlpha.h"




//--------------------------------------------------------------------------------
ofxCvColorImageAlpha::ofxCvColorImageAlpha() {
    init();
}

//--------------------------------------------------------------------------------
ofxCvColorImageAlpha::ofxCvColorImageAlpha( const ofxCvColorImageAlpha& _mom ) {
    init();
    if( _mom.bAllocated ) {
        // cast non-const,  to get read access to the mon::cvImage
        ofxCvColorImageAlpha& mom = const_cast<ofxCvColorImageAlpha&>(_mom);
        allocate( (int)mom.getWidth(), (int)mom.getHeight() );
        cvCopy( mom.getCvImage(), cvImage, 0 );
    } else {
        ofLog(OF_LOG_NOTICE, "in ofxCvColorImageAlpha copy constructor, mom not allocated");
    }
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::init() {
    ipldepth = IPL_DEPTH_8U;
    iplchannels = 4;
    gldepth = GL_UNSIGNED_BYTE;
    glchannels = GL_RGBA;
    cvGrayscaleImage = NULL;
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::clear() {
    if (bAllocated == true && cvGrayscaleImage != NULL){
        cvReleaseImage( &cvGrayscaleImage );
    }
    ofxCvImage::clear();    //call clear in base class
}




// Set Pixel Data

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::set( float value ){
    cvSet(cvImage, cvScalar(value, value, value));
    flagImageChanged();
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::set(int valueR, int valueG, int valueB){
    cvSet(cvImage, cvScalar(valueR, valueG, valueB));
    flagImageChanged();
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator -= ( float value ) {
	cvSubS( cvImage, cvScalar(value, value, value), cvImageTemp );
	swapTemp();
    flagImageChanged();
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator += ( float value ) {
	cvAddS( cvImage, cvScalar(value, value, value), cvImageTemp );
	swapTemp();
    flagImageChanged();
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::setFromPixels( const unsigned char* _pixels, int w, int h ) {
    // copy pixels ignoring any ROI
	
    if( w == width && h == height ) {
		
		if( cvImage->width*cvImage->nChannels == cvImage->widthStep ){
			memcpy( cvImage->imageData,  _pixels, w*h*4);
		}else{
			
 		 	for( int i=0; i < height; i++ ) {
				memcpy( cvImage->imageData + (i*cvImage->widthStep),
 	                   _pixels + (i*w*4),
					   width*4 );
			}
			
		}
		
        flagImageChanged();
    } else {
        ofLog(OF_LOG_ERROR, "in setFromPixels, size mismatch");
    }
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::setRoiFromPixels( const unsigned char* _pixels, int w, int h ) {
    ofRectangle roi = getROI();
    ofRectangle inputROI = ofRectangle( roi.x, roi.y, w, h);
    ofRectangle iRoi = getIntersectionROI( roi, inputROI );
	
    if( iRoi.width > 0 && iRoi.height > 0 ) {
        // copy pixels from _pixels, however many we have or will fit in cvImage
        for( int i=0; i < iRoi.height; i++ ) {
            memcpy( cvImage->imageData + ((i+(int)iRoi.y)*cvImage->widthStep) + (int)iRoi.x*4,
				   _pixels + (i*w*4),
				   (int)(iRoi.width*4) );
        }
        flagImageChanged();
    } else {
        ofLog(OF_LOG_ERROR, "in setRoiFromPixels, ROI mismatch");
    }
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::setFromGrayscalePlanarImages( ofxCvGrayscaleImage& red, ofxCvGrayscaleImage& green, ofxCvGrayscaleImage& blue){
	ofRectangle roi = getROI();
    ofRectangle redRoi = red.getROI();
    ofRectangle greenRoi = green.getROI();
    ofRectangle blueRoi = blue.getROI();
    if( redRoi.width == roi.width && redRoi.height == roi.height &&
	   greenRoi.width == roi.width && greenRoi.height == roi.height &&
	   blueRoi.width == roi.width && blueRoi.height == roi.height )
    {
		cvCvtPlaneToPix(red.getCvImage(), green.getCvImage(), blue.getCvImage(),NULL, cvImage);
		flagImageChanged();
	} else {
        ofLog(OF_LOG_ERROR, "in setFromGrayscalePlanarImages, ROI/size mismatch");
	}
}


//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator = ( unsigned char* _pixels ) {
    setFromPixels( _pixels, width, height );
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator = ( const ofxCvGrayscaleImage& _mom ) {
    // cast non-const,  no worries, we will reverse any chages
    ofxCvGrayscaleImage& mom = const_cast<ofxCvGrayscaleImage&>(_mom);
	if( matchingROI(getROI(), mom.getROI()) ) {
		cvCvtColor( mom.getCvImage(), cvImage, CV_GRAY2RGBA );
        flagImageChanged();
	} else {
        ofLog(OF_LOG_ERROR, "in =, ROI mismatch");
	}
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator = ( const ofxCvColorImage& _mom )
{

        ofxCvColorImage& mom = const_cast<ofxCvColorImage&>(_mom);
        if( matchingROI(getROI(), mom.getROI()) )
		{
            cvCopy( mom.getCvImage(), cvImage, 0 );
            flagImageChanged();
        } else {
            ofLog(OF_LOG_ERROR, "in =, ROI mismatch");
        }
}
//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator = ( const ofxCvColorImageAlpha& _mom ) {
    if(this != &_mom) {  //check for self-assignment
        // cast non-const,  no worries, we will reverse any chages
        ofxCvColorImageAlpha& mom = const_cast<ofxCvColorImageAlpha&>(_mom);
        if( matchingROI(getROI(), mom.getROI()) ) {
            cvCopy( mom.getCvImage(), cvImage, 0 );
            flagImageChanged();
        } else {
            ofLog(OF_LOG_ERROR, "in =, ROI mismatch");
        }
    } else {
        ofLog(OF_LOG_WARNING, "in =, you are assigning a ofxCvColorImage to itself");
    }
}
//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator = ( const ofxCvFloatImage& _mom ) {
    // cast non-const,  no worries, we will reverse any chages
    ofxCvFloatImage& mom = const_cast<ofxCvFloatImage&>(_mom);
	if( matchingROI(getROI(), mom.getROI()) ) {
        if( cvGrayscaleImage == NULL ) {
            cvGrayscaleImage = cvCreateImage( cvSize(width,height), IPL_DEPTH_8U, 1 );
        }
        ofRectangle roi = getROI();
        setImageROI(cvGrayscaleImage, roi);
        rangeMap( mom.getCvImage(), cvGrayscaleImage,
				 mom.getNativeScaleMin(), mom.getNativeScaleMax(), 0, 255.0f );
		cvCvtColor( cvGrayscaleImage, cvImage, CV_GRAY2RGBA );
        flagImageChanged();
	} else {
        ofLog(OF_LOG_ERROR, "in =, ROI mismatch");
	}
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator = ( const ofxCvShortImage& _mom ) {
    // cast non-const,  no worries, we will reverse any chages
    ofxCvShortImage& mom = const_cast<ofxCvShortImage&>(_mom);
    if( matchingROI(getROI(), mom.getROI()) ) {
        if( cvGrayscaleImage == NULL ) {
            cvGrayscaleImage = cvCreateImage( cvSize(width,height), IPL_DEPTH_8U, 1 );
        }
        ofRectangle roi = getROI();
        setImageROI(cvGrayscaleImage, roi);
        rangeMap( mom.getCvImage(), cvGrayscaleImage, 0, 65535.0f, 0, 255.0f );
		cvCvtColor( cvGrayscaleImage, cvImage, CV_GRAY2RGBA );
        flagImageChanged();
    } else {
        ofLog(OF_LOG_ERROR, "in =, ROI mismatch");
    }
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::operator = ( const IplImage* _mom ) {
    ofxCvImage::operator = (_mom);
}


// Get Pixel Data


//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::convertToGrayscalePlanarImages(ofxCvGrayscaleImage& red, ofxCvGrayscaleImage& green, ofxCvGrayscaleImage& blue){
    ofRectangle roi = getROI();
    ofRectangle redRoi = red.getROI();
    ofRectangle greenRoi = green.getROI();
    ofRectangle blueRoi = blue.getROI();
	if( redRoi.width == roi.width && redRoi.height == roi.height &&
	   greenRoi.width == roi.width && greenRoi.height == roi.height &&
	   blueRoi.width == roi.width && blueRoi.height == roi.height )
    {
        cvCvtPixToPlane(cvImage, red.getCvImage(), green.getCvImage(), blue.getCvImage(), NULL);
        red.flagImageChanged();
        green.flagImageChanged();
        blue.flagImageChanged();
	} else {
        ofLog(OF_LOG_ERROR, "in convertToGrayscalePlanarImages, ROI/size mismatch");
	}
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::convertToGrayscalePlanarImage (ofxCvGrayscaleImage& grayImage, int whichPlane){
	
	ofRectangle roi = getROI();
    ofRectangle grayRoi = grayImage.getROI();
	
	if( grayRoi.width == roi.width && grayRoi.height == roi.height ){
		
		switch (whichPlane){
				
			case 0:
				cvCvtPixToPlane(cvImage, grayImage.getCvImage(), NULL, NULL, NULL);
				grayImage.flagImageChanged();
				break;
			case 1:
				cvCvtPixToPlane(cvImage, NULL, grayImage.getCvImage(), NULL, NULL);
				grayImage.flagImageChanged();
				break;
			case 2:
				cvCvtPixToPlane(cvImage, NULL, NULL, grayImage.getCvImage(), NULL);
				grayImage.flagImageChanged();
				break;
		}
		
	} else {
		
		ofLog(OF_LOG_ERROR, "in convertToGrayscalePlanarImages, ROI/size mismatch");
		
	}
	
}




// Draw Image



// Image Filter Operations

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::contrastStretch() {
	ofLog(OF_LOG_WARNING, "in contrastStratch, not implemented for ofxCvColorImage");
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::convertToRange(float min, float max ){
    rangeMap( cvImage, 0,255, min,max);
    flagImageChanged();
}



// Image Transformation Operations

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::resize( int w, int h ) {
	
    // note, one image copy operation could be ommitted by
    // reusing the temporal image storage
	
    IplImage* temp = cvCreateImage( cvSize(w,h), IPL_DEPTH_8U, 4 );
    cvResize( cvImage, temp );
    clear();
    allocate( w, h );
    cvCopy( temp, cvImage );
    cvReleaseImage( &temp );
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::scaleIntoMe( ofxCvImage& mom, int interpolationMethod ){
    //for interpolation you can pass in:
    //CV_INTER_NN - nearest-neigbor interpolation,
    //CV_INTER_LINEAR - bilinear interpolation (used by default)
    //CV_INTER_AREA - resampling using pixel area relation. It is preferred method
    //                for image decimation that gives moire-free results. In case of
    //                zooming it is similar to CV_INTER_NN method.
    //CV_INTER_CUBIC - bicubic interpolation.
	
    if( mom.getCvImage()->nChannels == cvImage->nChannels &&
	   mom.getCvImage()->depth == cvImage->depth ) {
		
        if ((interpolationMethod != CV_INTER_NN) &&
            (interpolationMethod != CV_INTER_LINEAR) &&
            (interpolationMethod != CV_INTER_AREA) &&
            (interpolationMethod != CV_INTER_CUBIC) ){
            ofLog(OF_LOG_WARNING, "in scaleIntoMe, setting interpolationMethod to CV_INTER_NN");
    		interpolationMethod = CV_INTER_NN;
    	}
        cvResize( mom.getCvImage(), cvImage, interpolationMethod );
        flagImageChanged();
		
    } else {
        ofLog(OF_LOG_ERROR, "in scaleIntoMe, mom image type has to match");
    }
}

//--------------------------------------------------------------------------------
void ofxCvColorImageAlpha::convertRgbToHsv(){
    cvCvtColor( cvImage, cvImageTemp, CV_RGB2HSV);
    swapTemp();
    flagImageChanged();
}



