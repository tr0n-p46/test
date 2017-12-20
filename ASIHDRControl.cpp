/******************************************************************************************************************************
*Filename 	      : ASIHDRControl.cpp										      *
*Version	      : 1.0.0										  	              *
*Libraries            : OpenCV SDK 3.0.0 and ASICamera SDK 0.6.0504                                  			      *        *Author               : Souvik Das                                 						              *	          
*Date                 : 05/07/2017											      *		
*Dependency           : opencv_world300.so, opencv_ts300.so and libASICamera.so                                               *          
*Compile instructions : Include Dependency include directories and libraries including libASICamera.so in CmakeLists.txt,     *
*			sudo cmake and sudo make									      *
*Run                  : ./ASIHDRControl											      *		  
*Platform             : Linux x86_x64			                                                                      *
*******************************************************************************************************************************/

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <time.h>
#include <unistd.h>

#include <opencv/cv.hpp>
#include <opencv2/core.hpp>

#include <ASICamera.h>

#define exitF exit(EXIT_FAILURE)
#define exitS exit(EXIT_SUCCESS)

#define CHANNELS	       	    3			//Image channels. RGB has 3 channels				
#define CV_IMAGE_DEPTH   IPL_DEPTH_8U			//OpenCV image depth declaration.
#define BINNING			    1			//Bin
#define IMAGE_TYPE          IMG_RGB24			//ASI image capture format. RGB here. 8 * 3 = 24
#define DEFAULT_CAMERA  	    0			//Default selected camera
#define CROP_RADIUS 		  850			//Roughly chosen circular crop radius
#define WIDTH_BIAS 	 	   50			//Circular crop width bias	
#define HEIGHT_BIAS     	  -50			//Circular crop height bias
#define DELAY            	    5 			//Delay between HDR captures
#define MAX_CONTROL       	    7			//Maximum controls of ASI series camera supported by current SDK version

using namespace std;
using namespace cv;

char* controls[] = {"Gain", "Exposure", "Gamma", "WB_R", "WB_B", "Brightness", "USB Traffic"};
char* bayer[] = {"RG","BG","GR","GB"};

int width;
int height;

void initialize() {
	cout << "Welcome to ASIHDRControl!\n";
	int numberOfConnectedCameras = getNumberOfConnectedCameras();
	if(numberOfConnectedCameras <= 0) {
		cout << "No ASI cameras found. Please check connections to power and USB.\nPress any key to exit...";
		getchar();
		exitF;	
	}
	cout << "Connected ASI cameras : " << numberOfConnectedCameras << endl;
	cout << "Opening default selected camera : " << getCameraModel(DEFAULT_CAMERA) << endl;
	bool success = openCamera(DEFAULT_CAMERA);
	if(!success) {
		cout << "Failed opening default camera!\nPress any key to exit...";
		getchar();
		exitF;	
	}
	cout << "Success!\nInitialization in progress...\n";
	success = initCamera();
	if(!success) {
		cout << "Failed initialization!\nPress any key to exit...";
		getchar();
		exitF;	
	}
	printf("Resolution : %d x %d\n", getMaxWidth(), getMaxHeight());
	if(isColorCam()) {
		printf("Color pattern : %s\n", bayer[getColorBayer()]);
	}
	else {
		printf("Monocolor\n");
	}
	printf("Sensor temperature: %02f\n", getSensorTemp());
	//bresult = false;
	for(int i = 0; i < MAX_CONTROL; i++)
	{
		if(isAvailable((Control_TYPE)i));
			/*printf("%s support: Yes | Min : %d | Max : %d | Value : %d\n", controls[i], 
									  getMin((Control_TYPE)i), 
									  getMax((Control_TYPE)i),
									  getValue((Control_TYPE)i, &bresult));*/
		else
			printf("%s support: No\n", controls[i]);
	}
	width = getMaxWidth();
	height = getMaxHeight();
	setImageFormat(width, height, BINNING, IMAGE_TYPE);
}

int main()
{

	initialize();
	cout << "Press any key to continue : ";
	getchar();
	IplImage *pRgb = cvCreateImage(cvSize(getWidth(),getHeight()), CV_IMAGE_DEPTH, CHANNELS);

	int count = 0;
	bool flag = true;
	Mat response;
	char num[21];
	setValue(CONTROL_GAIN, getMin(CONTROL_GAIN), false);

	while(true) {
		sprintf(num, "%d", count);
		cout << "\nStarting real time cycle : #" << count << endl;
		int exp_seq[] = {100, 120, 140, 160, 180, 200, 220, 240, 260, 280};
		vector<Mat> images;
		vector<float> times;

		clock_t t1 = clock();
		
		for(int i = 0;i < 10;i++) {
			float exp = exp_seq[i];
			if(exp >= 1000000) {
				exp = exp / 1000000;
				printf("%d --> Taking exposure at %.1lfs\n", i, exp);			
			}
			else if(exp >= 1000) {
				exp = exp / 1000;
				printf("%d --> Taking exposure at %.1lfms\n", i, exp);				
			}
			else {
				printf("%d --> Taking exposure at %dus\n", i, (int)exp);			
			}			
			setValue(CONTROL_EXPOSURE, exp_seq[i], false);
	
			printf("Exposure started!\n");
			startExposure();
			EXPOSURE_STATUS status;
			while(1) {
				status = getExpStatus();
				if(status == EXP_SUCCESS) 
					break;
			}
			stopExposure();
			if(status == EXP_SUCCESS) {
				getImageAfterExp((unsigned char*)pRgb->imageData, pRgb->imageSize);
			}
		
			printf("Exposure complete!\n");
			
			printf("Got image data successfully!\n");

			printf("Stopped capture!\n");
			Mat imgg = cvarrToMat(pRgb);
		/****************************************************CIRCULAR CROP*************************************************************/
			int radius = CROP_RADIUS;
			Point center(width / 2 - WIDTH_BIAS, height / 2 - HEIGHT_BIAS);
			Mat mask = Mat::zeros( imgg.rows, imgg.cols, CV_8UC1 );
			circle( mask, center, radius, Scalar(255,255,255), -1, 8, 0 ); //-1 means filled
			Mat dst;
			imgg.copyTo( dst, mask );
			Mat roi = dst( Rect( center.x - radius, center.y - radius, radius * 2, radius * 2 ) );
			//imwrite("Cropped.tiff", roi);
		/********************************************************END*******************************************************************/
			images.push_back(roi);
			times.push_back((float)exp_seq[i] / 1000000);
		
			printf("Loop %d completed!\n\n", i);
		}
		printf("Loaded exposure sequence successfully!\n");
		if(!flag) {
			flag = true;
			Ptr<CalibrateDebevec> calibrate = createCalibrateDebevec();
			calibrate->process(images, response, times);
			imwrite("CRF.tiff", response);
			printf("CRF estimated!\n");
		}
		else {
			response = imread("CRF.tiff", -1);
			//response.convertTo(response, CV_32F);
		}
		
		Mat hdr;
    		Ptr<MergeDebevec> merge_debevec = createMergeDebevec();
    		merge_debevec->process(images, hdr, times, response);

		printf("HDR created!\n");	

		clock_t t2 = clock();
		float duration = (float)(t2 - t1);
		printf("HDR Cycle Duration : %lfs\n", duration / 1000);

		/*Luminance Map*/
		
		Point minRedP, minGreenP, minBlueP;
		Point maxRedP, maxGreenP, maxBlueP;
		double minRed, minBlue, minGreen;
		double maxRed, maxBlue, maxGreen;

		Mat channels[3];
		split(hdr, channels);
		minMaxLoc(channels[0], &minRed, &maxRed, &minRedP, &maxRedP);
		minMaxLoc(channels[1], &minGreen, &maxGreen, &minGreenP, &maxGreenP);
		minMaxLoc(channels[2], &minBlue, &maxBlue, &minBlueP, &maxBlueP);



		cout << "Minimum : " << "Red : (" << minRedP.x << "," << minRedP.y << ") | Green : (" << minGreenP.x << "," << minGreenP.y << ") | Blue : (" << minBlueP.x << "," << minBlueP.y << ")\n";

		cout << "Red : " << minRed << " | Blue : " << minBlue << " | Green : " << minGreen << endl;

		cout << "Maximum : " << "Red : (" << maxRedP.x << "," << maxRedP.y << ") | Green : (" << maxGreenP.x << "," << maxGreenP.y << ") | Blue : (" << maxBlueP.x << "," << maxBlueP.y << ")\n";

		cout << "Red : " << maxRed << " | Blue : " << maxBlue << " | Green : " << maxGreen << endl;
		/*END*/

		/*HDR min max positions of RGB*/
		int rad = 5;
		Mat gmi = imread("Lumen.tiff", -1);
		gmi.convertTo(gmi, CV_32F);
		circle( gmi, maxRedP, rad, Scalar(0,0,255), -1, 8, 0 );
		circle( gmi, maxGreenP, rad, Scalar(0,255,0), -1, 8, 0 );
		circle( gmi, maxBlueP, rad, Scalar(255,0,0), -1, 8, 0 );
		
		string ext_tiff = "_lumen.tiff";
		string name = num + ext_tiff; 
		imwrite("Lumen.tiff", gmi);
		/**/
		string ext_hdr = "_hdr.hdr";
		name = num + ext_hdr; 
		imwrite(name, hdr);
		cout << "HDR file " << name << " written successfully to disk.\nCycle completed!\n";
		cout << "\nDelay for " << DELAY << "s\n\n"; 
		usleep(DELAY * 1000000);
		++count;
	}
	closeCamera();	
	return 0;
}
