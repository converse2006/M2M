/***************************************************************************
*   Copyright (C) 2010 by elsamuko                                        *
*   elsamuko@web.de                                                       *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

// small program to detect faces in an input image, based on the example of the OpenCV wiki:
// http://opencv.willowgarage.com/wiki/FaceDetection
// compile with:
// g++ -O2 -Wall `pkg-config --cflags opencv` `pkg-config --libs opencv` -o opencv-facedetect opencv-facedetect.cpp

// OpenCV
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"

// C++
#include <iostream>
#include <string>
#include <utility>

// C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#define SCALE         1
#define CASCADE_FILE "haarcascade_frontalface_alt.xml"

using namespace std;

bool DEBUG;

CvSeq* detect( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade);
void draw(IplImage* img, CvSeq* faces);
void usage();

CvSeq* detect( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade) {
    cvClearMemStorage( storage );
    if ( cascade ){
        return cvHaarDetectObjects( img, cascade, storage,
                                     1.1, 3, CV_HAAR_DO_CANNY_PRUNING,
                                     cvSize(40, 40) );
    }
    return 0;
}

void draw(IplImage* img, CvSeq* faces) {
    CvPoint pt1, pt2;
    for (int i = 0; i < faces->total; i++ ){
        CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
        pt1.x =  r->x            * SCALE;
        pt2.x = (r->x+r->width)  * SCALE;
        pt1.y =  r->y            * SCALE;
        pt2.y = (r->y+r->height) * SCALE;
        cvRectangle( img, pt1, pt2, CV_RGB(255,0,0), 3, 8, 0 );
    }
}

void usage() {
    cout << "Usage: opencv-facedetect [options] imageIn [imageOut]" << endl;
    cout << "       -h or --help      This message" << endl;
    cout << "       -d or --debug     Debug output" << endl;
    cout << "       -c or --cascade   Cascade file, default: haarcascade_frontalface_alt.xml" << endl;
    cout << endl;
}

int main( int argc, char** argv ) {
    if ( argc > 1 ) {

        string imageIn;
        string imageOut;
        string cascade_file = CASCADE_FILE;

        IplImage* image = 0;
        bool in = true;
        DEBUG = false;

        // set options
        for ( int i = 1; i < argc; i++ ){
            const char *sw = argv[i];

            if ( !strcmp(sw, "-h") || !strcmp( argv[i], "--help") ) {
                usage();
                return 0;

            } else if ( !strcmp(sw, "-d") || !strcmp(sw, "--debug") ) {
                DEBUG = true;

            } else if ( !strcmp(sw, "-c") || !strcmp(sw, "--cascade") ) {
                if ( i + 1 >= argc ) {
                    usage();
                    return 1;
                }
                cascade_file = argv[++i];

            } else {
                if (in) {
                    imageIn += sw;
                    in = false;
                } else {
                    imageOut += sw;
                }
            }
        }

        if ( imageIn.size() != 0 ) {
            cout<<"imageIn:  "<<imageIn<<endl;
            cout<<"cascade:  "<<cascade_file<<endl;

            // load the HaarClassifierCascade
            CvHaarClassifierCascade* cascade = (CvHaarClassifierCascade*)cvLoad( cascade_file.c_str(), 0, 0, 0 );
            if ( !cascade ) {
                cout<<"Error: Could not load classifier cascade"<<endl;
                return -1;
            }

            // detect faces
            cout<<"Detect faces"<<endl;
            CvMemStorage* storage = cvCreateMemStorage(0);
            image = cvLoadImage( imageIn.c_str(), -1 );
            cout<<"Image ="<<image<<endl;
            if (image) {
                cout<<"Image exist"<<endl;
                CvSeq* faces = detect(image, storage, cascade);
                if (faces->total != 0) {
                    cout<<"faces found"<<faces->total<<" !!"<<endl;
                    draw (image, faces);
                    if(imageOut.size() == 0) {
                        cvSaveImage(("face_" + imageIn).c_str(), image );
                        cout<<"ImageOut: "<< ("face_" + imageIn) <<endl;
                    }else{
                        cvSaveImage(imageOut.c_str(), image );
                        cout<<"ImageOut: "<< imageOut <<endl;
                    }
                }else{
                    cout<<"No faces found."<<endl;
                }
                cvReleaseImage( &image );
            }
        } else {
            cout << "Error: Not enough arguments:" << endl;
            usage();
        }
    } else {
        usage();
    }
    cout <<"Program finish!!\n";
    return 0;
}
