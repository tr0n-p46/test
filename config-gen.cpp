#include <asihdrcommon.h>

#define MAX_CONTROLS 18

const string config = "/etc/config.yml"; 

const string controls[] = {"Gain ", "Exposure ", "Gamma ", "WB_R ", "WB_B ", "Brightness ", "Bandwidth overload ", "Overclock ", "Temperature ",
				"Hardware bin ", "High speed ", "Cooler power perc ", "Target temp ", "Cooler on ", "Mono bin ", "Fan on ",
					"Pattern adjust ", "Anti dew heat "};

const string bayerPattern[] = {"RG", "BG", "GR", "GB"};

const String keys =
        "{help usage ?   |             | Generates a new config file or edits previous config file.        }"
	"{w              |3096         | Sets width of frame.                                              }"
	"{h              |2080         | Sets height of frame.                                             }"
	"{b              |1            | Binning.                                                          }"
	"{r              |980          | Crop radius.	                                                   }"
	"{wb             |50           | Width bias.	                                                   }"
	"{hb             |-50          | Height bias.                                                      }"
	"{s              |100          | ISO speed.                                                        }"
	"{f              |3.0          | Aperture size.	                                                   }"
	"{g              |2.2          | Gamma correction factor.                                          }"
	"{e              |179.2        | White efficiency required for using with radiance.                }"
	"{n              |5            | Interval of capture.                                              }"
	"{k              |5.2          | Scale factor.                                                     }"
	"{c              |OFF          | Produce falsecolor images. To turn on make ON	                   }"
	"{hdr            |./HDR        | HDR saving path.                                                  }"
	"{ldr            |./LDR        | LDR saving path.                                                  }"
	"{falsecolor     |./falsecolor | Falsecolor saving path.                                           }"
	"{response       |./response   | Camera response function saving path.                             }"
	"{latest         |./           | Path to store the last computed HDR.                              }"
	"{exp_s          |u            | s for seconds, m for milliseconds and u for microseconds.         }"
	"{exposure       |100          | Starting exposure.                                                }"
	"{m              |10           | Number of LDR images to take.                                     }"
	"{gain           |0            | Gain.                                                             }"
	"{d              |100          | Exposure difference. Default is 100us                             }"
        ;

void initCameraParams(FileStorage fs) {
	int n = getNumberOfConnectedCameras();	
	if(n == 0) {
		cout << "No cameras found." << endl;
		exitF;
	}
	
	int defaultCamera = 0;
	if(n > 1) {
		for(int i = 0;i < n;i++) {
			string cameraModel = string(getCameraModel(i));
			cout << i << ". " << cameraModel << endl;	
		}
		cout << "Select default camera : " << endl;
		do{
			cin >> defaultCamera;
			if(defaultCamera < 0 || defaultCamera >= n) {
				cout << "Index out of bounds. Enter camera index from within list above : ";		
			}
		}while(defaultCamera < 0 || defaultCamera >= n);
	}
	fs << "Default Camera " << defaultCamera;
	string cameraModel = getCameraModel(defaultCamera);
	fs << "Camera Model " << cameraModel;
	bool success = openCamera(defaultCamera);
	if(!success) {
		cout << "Cannot open camera. Try using sudo." << endl;
		exitF;	
	}
	success = initCamera();
	if(!success) {
		cout << "Cannot initialize camera. Try reconnecting the USB to another port." << endl;
		exitF;
	}
	if(isColorCam()) {
		fs << "Color " << bayerPattern[getColorBayer()];	
	}
	else {
		fs << "Color " << "Monochrome";	
	}
	fs << "Pixel Size " << getPixelSize();
	fs << "Max Width " << getMaxWidth();
	fs << "Max Height " << getMaxHeight();
	if(isUSB3Camera()) {
		fs << "USB " << "3.0";	
	}
	else {
		fs << "USB " << "2.0";	
	}
	fs << "Electrons per ADU " << getElectronsPerADU();
	if(isCoolerCam()) {
		fs << "CoolerCam " << "Yes";	
	}
	else {
		fs << "CoolerCam " << "No";	
	}	
	fs << "Controls " << "[";
	for(Control_TYPE i = (Control_TYPE)0;(Control_TYPE)i < MAX_CONTROLS;i = (Control_TYPE)(i + 1)) {
		if(isAvailable(i)) {
			fs << "{";
			fs << "Index " << i;
			fs << "Control Name " << controls[i];
			bool isAuto = isAutoSupported(i);
			if(isAuto) {
				fs << "Auto " << "Yes";			
			}		
			else {
				fs << "Auto " << "No";			
			}
			fs << "Max value " << getMax(i);
			fs << "Min value " << getMin(i);
			fs << "Current value " << getValue(i, &isAuto);
			fs << "}";
		}
	}
	fs << "]";
	closeCamera();
}

int main(int argc, char *argv[]) {
	CommandLineParser parser(argc, argv, keys);
	parser.about("config-gen v1.0.0");
	if (parser.has("help"))
	{
    		parser.printMessage();
    		return 0;
	}
	FileStorage fs(config, FileStorage::WRITE);
	if(!fs.isOpened()) {
		cout << "Cannot open config file. Try using sudo." << endl;
		exitF;
	}
	initCameraParams(fs);		

	int w = parser.get<int>("w");
	if((w % 8) != 0) {
		cout << "Width must be a multiple of 8. Defaulting to 3096." << endl;
		w = 3096;	
	}
	fs << "Image Width " << w;
	int h = parser.get<int>("h");
	if((h % 2) != 0) {
		cout << "Height must be a multiple of 2. Defaulting to 1080." << endl;
		h = 1080;		
	}
	fs << "Image Height " << h;
	int b = parser.get<int>("b");
	if(b != 1 && !isBinSupported(b)) {
		cout << "Mentioned binning is unsupported by camera. Defaulting to 1." << endl;
		b = 1;
	}
	fs << "Binning " << b;

	int r = parser.get<int>("r");
	if(r < 50) {
		cout << "Out of bounds crop radius. Using default 980." << endl;
		r = 980;
	}
	fs << "Crop Radius " << r;
	int wb = parser.get<int>("wb");
	if((wb < -100 || wb > 100)) {
		cout << "Out of bounds width bias. Using default 50." << endl;
		wb = 50;	
	}
	fs << "Width Bias " << wb;
	int hb = parser.get<int>("hb");
	if((hb < -100 || hb > 100)) {
		cout << "Out of bounds height bias. Using default -50." << endl;
		hb = -50;	
	}
	fs << "Height Bias " << hb;
	int s = parser.get<int>("s");
	if(s < 80) {
			cout << "ISO cannot be less than 80. Using default 100." << endl;
			s = 100;
	}
	fs << "ISO " << s;
	float f = parser.get<float>("f");
	if(f <= 1) {
		cout << "Aperture cannot be less than 1.0. Using default 3.0." << endl;
	 	f = 3.0f;	
	}
	fs << "Aperture " << f;
	float g = parser.get<float>("g");
	if((g < 0.0f || g > 2.8f)) {
		cout << "Gamma correction factor must be between 0 and 2.8 excluding 0. Using default 2.2." << endl;		
		g = 2.2f;
	}
	fs << "Gamma " << g;
	float e = parser.get<float>("e");
	if(e < 0) {
		cout << "Invalid white efficiency factor. Using default 179.2f." << endl;
		e = 179.2f;	
	} 
	fs << "White Efficiency " << e;
	int n = parser.get<int>("n");
	if(n < 1) {
		cout << "Delay cannot be less than 1s. Using default 15s." << endl;
		n = 15;	
	}
	fs << "Interval " << n;
	float k = parser.get<float>("k");
	if(k < 0) {
		cout << "Invalid scale factor. Using default 5.2." << endl;
		k = 5.2f;
	}
	fs << "Scale Factor " << k;
	string c = parser.get<string>("c");
	if((strcmp(c.c_str(), "OFF") != 0 && strcmp(c.c_str(), "ON") != 0)) {
		cout << "Falsecolor toggle can be either OFF or ON. Using default ON." << endl;
		c = "OFF";	
	}
	fs << "Falsecolor " << c;
	string hdr = parser.get<string>("hdr");	
	fs << "HDR Path " << hdr;
	struct stat st;
	if((stat("HDR", &st) != 0)) {
		system("mkdir HDR");
		system("chmod 777 HDR"); 
	}
	else if(!S_ISDIR(st.st_mode)) {
		cout << "Cannot create HDR directory..." << endl;
		exitF;	
	}
	string ldr  = parser.get<string>("ldr");
	fs << "LDR Path " << ldr;
	
	if((stat("LDR", &st) != 0)) {
		system("mkdir LDR");
		system("chmod 777 LDR");
	}
	else if(!S_ISDIR(st.st_mode)) {
		cout << "Cannot create LDR directory..." << endl;
		exitF;	
	}
	string falsecolor = parser.get<string>("falsecolor");
	fs << "Falsecolor Path " << falsecolor;
	
	if((stat("Falsecolor", &st) != 0)) {
		system("mkdir Falsecolor");
		system("chmod 777 Falsecolor");	
	}
	else if(!S_ISDIR(st.st_mode)) {
		cout << "Cannot create Falsecolor directory..." << endl;
		exitF;	
	}
	string response  = parser.get<string>("response");
	fs << "Response Path " << response;
	
	if((stat("Response", &st) != 0)) {
		system("mkdir Response");
		system("chmod 777 Response");	
	}
	else if(!S_ISDIR(st.st_mode)) {
		cout << "Cannot create Response directory..." << endl;
		exitF;	
	}
	string latest = parser.get<string>("latest");
	fs << "Latest " << latest;
	string exp_s = parser.get<string>("exp_s");
	fs << "Exposure Unit " << exp_s;
	int exp = parser.get<int>("exposure");
	fs << "Starting Exposure " << exp;
	int m = parser.get<int>("m");
	fs << "Sequence " << m;
	int gain = parser.get<int>("gain");
	fs << "Gain " << gain;
	int d = parser.get<int>("d");
	fs << "Exposure Difference " << d;
	fs.release();
	exitS;
}
