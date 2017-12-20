#include <asihdrcommon.h>
#include <ctime>
#include <stdio.h>
#include <time.h>

const string config = "/etc/config.yml";

string currentTime() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%H_%M_%S", &tstruct);
	return buf;
}

string currentDate() {

	time_t t = time(0);   
    	struct tm * now = localtime(&t);
	char buf[80];
	sprintf(buf, "%d_%d_%d", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900);
	return buf;
}

string getFileName() { 
	return currentDate() + "_" + currentTime();
}

Mat apply(Mat &image, Mat &mask) {
	Mat image_XYZ;
	cvtColor(image, image_XYZ, COLOR_BGR2XYZ);
	for(int i = 0;i < image.rows;i++) {
		for(int j = 0;j < image.cols;j++) {
			Vec3f intensity = image_XYZ.at<Vec3f>(i, j);
			double factor = mask.at<double>(i, j);
			if(factor == 0.0f) continue;
			intensity.val[0] /= factor;
			intensity.val[1] /= factor;
			intensity.val[2] /= factor;
			image_XYZ.at<Vec3f>(i, j) = intensity;
		}	
	}
	Mat result;
	cvtColor(image_XYZ, result, COLOR_XYZ2BGR);
	return result;
}

int main(int argc, char *argv[]) {
	FileStorage fs(config, FileStorage::READ);
	if(!fs.isOpened()) {
		cout << "Cannot open config file. Try using sudo." << endl;
		exitF;	
	}
	int n = getNumberOfConnectedCameras();
	if(n < 1) {
		cout << "No cameras found! Try reconnecting the USB to another port." << endl;
		exitF;	
	}
	int defaultCamera = 0;
	fs["Default Camera"] >> defaultCamera;
	
	bool success = openCamera(defaultCamera);
	if(!success) {
		cout << "Cannot open camera. Try using sudo or re-run config-gen to change default camera." << endl;
		exitF;	
	}
	success = initCamera();
	if(!success) {
		cout << "Cannot initialize camera. Try reconnecting the USB to another port." << endl;
		exitF;	
	}
	int width, height, binning;
	fs["Image Width"] >> width;
	fs["Image Height"] >> height;
	fs["Binning"] >> binning;
	
	success = setImageFormat(width, height, binning, IMG_RGB24);
	if(!success) {
		cout << "Image format error. Re-run config-gen and set appropriate parameters." << endl;
		exitF;	
	}
	int delay;
	fs["Interval"] >> delay;
	
	int gain;
	fs["Gain"] >> gain;
	IplImage *frame = cvCreateImage(cvSize(getWidth(),getHeight()), IPL_DEPTH_8U, 3);
	setValue(CONTROL_GAIN, gain, false);

	string exp_s;
	fs["Exposure Unit"] >> exp_s;
	int exp, m, d, wb, hb, r;
	fs["Starting Exposure"] >> exp;
	fs["Sequence"] >> m;
	fs["Exposure Difference"] >> d;
	fs["Width Bias"] >> wb;
	fs["Height Bias"] >> hb;
	fs["Crop Radius"] >> r;
	
	if(strcmp(exp_s.c_str(), "m") == 0) {
		exp *= 1000;	
	}
	else if(strcmp(exp_s.c_str(), "s") == 0) {
		exp *= 1000000;	
	}
	float aperture;
	int iso;
	fs["ISO"] >> iso;
	fs["Aperture"] >> aperture;

	float k;	
	fs["Scale Factor"] >> k;

	FileStorage ms("mask.yml", FileStorage::READ);
	Mat mask;
	ms["Mask"] >> mask; 
	ms.release();

	while(true) {
		int e = exp;
		string dir_name = getFileName();
		system(("mkdir LDR/" + dir_name).c_str());
		string exposure_seq = "LDR/" + dir_name + "/exposure_seq.txt";
		ofstream file(exposure_seq.c_str());
		for(int i = 0;i < m;i++) {
			setValue(CONTROL_EXPOSURE, e, false);
			startExposure();
			EXPOSURE_STATUS status = EXP_WORKING;
			while(status == EXP_WORKING) {
				status = getExpStatus();
			}
			if(status == EXP_SUCCESS) {
				getImageAfterExp((unsigned char*)frame->imageData, frame->imageSize);
			}
			stopExposure();
			e += d;
	
			Mat image = cvarrToMat(frame);
			Point center(getWidth() / 2 - wb, getHeight() / 2 - hb);
			Mat mask = Mat::zeros(image.rows, image.cols, CV_8UC1);
			circle(mask, center, r, Scalar(255,255,255), -1, 8, 0 );
			Mat dst;
			image.copyTo(dst, mask);
			Mat roi = dst(Rect(center.x - r, center.y - r, r * 2, r * 2 ) );
			char buf[80];
			sprintf(buf, "%s_%d%s", dir_name.c_str(), i, ext_tiff.c_str());
			string filename = "LDR/" + dir_name + "/" + string(buf);
			imwrite(filename, roi);	
			
			float inv_exp = 1000000.0f / e;
			file << filename << " " << inv_exp << " " << aperture << " " << iso << " " << "0" << endl;
		}
		file.close();
		
		/***/
		string hdr_name = "HDR/" + dir_name + ext_hdr;
		system(("pfsinhdrgen " + string(exposure_seq) + " | pfshdrcalibrate -v -f Response/pfs_rsp.txt | pfsout --radiance HDR/pfs.hdr").c_str());
	
		char num[21];
		sprintf(num, "%lf", k);
		system(("pfsin HDR/pfs.hdr | pfsabsolute " + string(num) + " 1 | pfsout HDR/pfs_scale.hdr").c_str());
		Mat hdr = imread("HDR/pfs_scale.hdr", -1);		
		Mat result = apply(hdr, mask);
		imwrite(hdr_name, result);
		usleep(delay * 1000000);
	}

	closeCamera();
	fs.release();
	exitS;
}
