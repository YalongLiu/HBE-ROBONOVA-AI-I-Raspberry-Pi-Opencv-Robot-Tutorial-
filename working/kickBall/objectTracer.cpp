
#include <sstream>
#include <string>
#include <iostream>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <raspicam/raspicam_cv.h>
#include "uart_api.h"

using namespace cv;
using namespace std;

//initial min and max HSV filter values.
//these will be changed using trackbars
  int H_MIN;
  int H_MAX;
  int S_MIN;
  int S_MAX;
  int V_MIN;
  int V_MAX;
  int G_MAX;
//default capture width and height
const int FRAME_WIDTH = 320;
const int FRAME_HEIGHT = 240;
//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 50;
//minimum and maximum object area
const int MIN_OBJECT_AREA = 10 * 1;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH / 1.5;
//names that will appear at the top of each window
const string windowName = "Original Image";
const string windowName1 = "HSV Image";
const string windowName2 = "Thresholded Image";
const string windowName3 = "After Morphological Operations";
const string trackbarWindowName = "Trackbars";

bool calibrationMode=true;//used for showing debugging windows, trackbars etc.

bool mouseIsDragging;//used for showing a rectangle on screen as user clicks and drags mouse
bool mouseMove;
bool rectangleSelected;
cv::Point initialClickPoint, currentMousePoint; //keep track of initial point clicked and current position of mouse
cv::Rect rectangleROI; //this is the ROI that the user has selected
vector<int> H_ROI, S_ROI, V_ROI;// HSV values from the click/drag ROI region stored in separate vectors so that we can sort them easily
bool trackObjects = true;
bool useMorphOps = true;

//Matrix to store each frame of the webcam feed
Mat cameraFeed;
//matrix storage for HSV image
Mat HSV;
//matrix storage for binary threshold image
Mat threshold1;
//x and y values for the location of the object
int x = 0, y = 0;
unsigned char cx[1];
unsigned char cy[1];
double refArea = 0;
unsigned char areaCode[1];

//image_transport::Publisher image_pub_;
raspicam::RaspiCam_Cv Camera;


void on_trackbar(int, void*)
{//This function gets called whenever a
	// trackbar position is changed

	//for now, this does nothing.
}

class objectTracerClass{
public:
  objectTracerClass();
private:


  void createTrackbars();
  //void clickAndDrag_Rectangle(int event, int x, int y, int flags, void* param);
  void recordHSV_Values(cv::Mat frame, cv::Mat hsv_frame);
  string intToString(int number);
  void drawObject(int x, int y, Mat &frame);
  void morphOps(Mat &thresh);
  void pushSpeed(float leftS,float rightS);
  void Angle(void);
  unsigned char GoObject(int xp ,int yp);
  void Go(void);
  void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed);

public:
  void	imageProccess();
};

objectTracerClass::objectTracerClass()
{

	/*H_MIN = 174;
	S_MIN = 77;
	V_MIN = 94;
	H_MAX = 179;
	S_MAX = 205;
	V_MAX = 188;
*/
        H_MIN = 14;
        S_MIN = 124;
        V_MIN = 155;

        H_MAX = 17;
        S_MAX = 200;
        V_MAX = 205;
	calibrationMode = false;
}



void objectTracerClass::createTrackbars(){
	//create window for trackbars


	namedWindow(trackbarWindowName, 0);
	//create memory to store trackbar name on window
	/*char TrackbarName[50];
	sprintf(TrackbarName, "H_MIN", H_MIN);
	sprintf(TrackbarName, "H_MAX", H_MAX);
	sprintf(TrackbarName, "S_MIN", S_MIN);
	sprintf(TrackbarName, "S_MAX", S_MAX);
	sprintf(TrackbarName, "V_MIN", V_MIN);
	sprintf(TrackbarName, "V_MAX", V_MAX);*/
	//create trackbars and insert them into window
	//3 parameters are: the address of the variable that is changing when the trackbar is moved(eg.H_LOW),
	//the max value the trackbar can move (eg. H_HIGH), 
	//and the function that is called whenever the trackbar is moved(eg. on_trackbar)
	//                                  ---->    ---->     ---->      
	createTrackbar("H_MIN", trackbarWindowName, &H_MIN, 255,  on_trackbar);
	createTrackbar("H_MAX", trackbarWindowName, &H_MAX, 255,  on_trackbar);
	createTrackbar("S_MIN", trackbarWindowName, &S_MIN, 255,  on_trackbar);
	createTrackbar("S_MAX", trackbarWindowName, &S_MAX, 255,  on_trackbar);
	createTrackbar("V_MIN", trackbarWindowName, &V_MIN, 255,  on_trackbar);
	createTrackbar("V_MAX", trackbarWindowName, &V_MAX, 255,  on_trackbar);

	
}
void  clickAndDrag_Rectangle(int event, int x, int y, int flags, void* param){
	//only if calibration mode is true will we use the mouse to change HSV values
	if (calibrationMode == true){
		//get handle to video feed passed in as "param" and cast as Mat pointer
		Mat* videoFeed = (Mat*)param;

		if (event == CV_EVENT_LBUTTONDOWN && mouseIsDragging == false)
		{
			//keep track of initial point clicked
			initialClickPoint = cv::Point(x, y);
			//user has begun dragging the mouse
			mouseIsDragging = true;
		}
		/* user is dragging the mouse */
		if (event == CV_EVENT_MOUSEMOVE && mouseIsDragging == true)
		{
			//keep track of current mouse point
			currentMousePoint = cv::Point(x, y);
			//user has moved the mouse while clicking and dragging
			mouseMove = true;
		}
		/* user has released left button */
		if (event == CV_EVENT_LBUTTONUP && mouseIsDragging == true)
		{
			//set rectangle ROI to the rectangle that the user has selected
			rectangleROI = Rect(initialClickPoint, currentMousePoint);

			//reset boolean variables
			mouseIsDragging = false;
			mouseMove = false;
			rectangleSelected = true;
		}

		if (event == CV_EVENT_RBUTTONDOWN){
			//user has clicked right mouse button
			//Reset HSV Values
			H_MIN = 0;
			S_MIN = 0;
			V_MIN = 0;
			H_MAX = 255;
			S_MAX = 255;
			V_MAX = 255;

		}
		if (event == CV_EVENT_MBUTTONDOWN){

			//user has clicked middle mouse button
			//enter code here if needed.
		}
	}

}
void objectTracerClass::recordHSV_Values(cv::Mat frame, cv::Mat hsv_frame){
	
	//save HSV values for ROI that user selected to a vector
	if (mouseMove == false && rectangleSelected == true){
		
		//clear previous vector values
		if (H_ROI.size()>0) H_ROI.clear();
		if (S_ROI.size()>0) S_ROI.clear();
		if (V_ROI.size()>0 )V_ROI.clear();
		//if the rectangle has no width or height (user has only dragged a line) then we don't try to iterate over the width or height
		if (rectangleROI.width<1 || rectangleROI.height<1) cout << "Please drag a rectangle, not a line" << endl;
		else{
			for (int i = rectangleROI.x; i<rectangleROI.x + rectangleROI.width; i++){
				//iterate through both x and y direction and save HSV values at each and every point
				for (int j = rectangleROI.y; j<rectangleROI.y + rectangleROI.height; j++){
					//save HSV value at this point
					H_ROI.push_back((int)hsv_frame.at<cv::Vec3b>(j, i)[0]);
					S_ROI.push_back((int)hsv_frame.at<cv::Vec3b>(j, i)[1]);
					V_ROI.push_back((int)hsv_frame.at<cv::Vec3b>(j, i)[2]);
				}
			}
		}
		//reset rectangleSelected so user can select another region if necessary
		rectangleSelected = false;
		//set min and max HSV values from min and max elements of each array
					
		if (H_ROI.size()>0){
			//NOTE: min_element and max_element return iterators so we must dereference them with "*"
			H_MIN = *std::min_element(H_ROI.begin(), H_ROI.end());
			H_MAX = *std::max_element(H_ROI.begin(), H_ROI.end());
			cout << "MIN 'H' VALUE: " << H_MIN << endl;
			cout << "MAX 'H' VALUE: " << H_MAX << endl;
		}
		if (S_ROI.size()>0){
			S_MIN = *std::min_element(S_ROI.begin(), S_ROI.end());
			S_MAX = *std::max_element(S_ROI.begin(), S_ROI.end());
			cout << "MIN 'S' VALUE: " << S_MIN << endl;
			cout << "MAX 'S' VALUE: " << S_MAX << endl;
		}
		if (V_ROI.size()>0){
			V_MIN = *std::min_element(V_ROI.begin(), V_ROI.end());
			V_MAX = *std::max_element(V_ROI.begin(), V_ROI.end());
			cout << "MIN 'V' VALUE: " << V_MIN << endl;
			cout << "MAX 'V' VALUE: " << V_MAX << endl;
		}

	}

	if (mouseMove == true){
		//if the mouse is held down, we will draw the click and dragged rectangle to the screen
		rectangle(frame, initialClickPoint, cv::Point(currentMousePoint.x, currentMousePoint.y), cv::Scalar(0, 255, 0), 1, 8, 0);
	}


}
string objectTracerClass::intToString(int number){


	std::stringstream ss;
	ss << number;
	return ss.str();
}
void objectTracerClass::drawObject(int x, int y, Mat &frame){

	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!


	//'if' and 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window)

	circle(frame, Point(x, y), 20, Scalar(0, 255, 0), 2);
	if (y - 25>0)
		line(frame, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, 0), Scalar(0, 255, 0), 2);
	if (y + 25<FRAME_HEIGHT)
		line(frame, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(0, 255, 0), 2);
	if (x - 25>0)
		line(frame, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
	if (x + 25<FRAME_WIDTH)
		line(frame, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(FRAME_WIDTH, y), Scalar(0, 255, 0), 2);

	putText(frame, intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);

}
void objectTracerClass::morphOps(Mat &thresh){

	//create structuring element that will be used to "dilate" and "erode" image.
	//the element chosen here is a 3px by 3px rectangle

	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));
	//dilate with larger element so make sure object is nicely visible
	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);


	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);

}
void objectTracerClass::pushSpeed(float leftS,float rightS)
{
	
}
void objectTracerClass::Angle(void)
{
	int cx = x;
	
	cx = cx - 320;
	if(cx>160)
		pushSpeed(50,-50);
	else if(cx>110)
		pushSpeed(30,-30);
	else if(cx>60)
		pushSpeed(20,-20);
	else if(cx>20)
		pushSpeed(15,-15);		
	else if(cx>-20)
		pushSpeed(0,0);		
	else if(cx>-60)
		pushSpeed(-15,15);
	else if(cx>-110)
		pushSpeed(-20,20);		
	else
		pushSpeed(-30,30);				
}

unsigned char objectTracerClass::GoObject(int xp ,int yp)
{

	if(xp<108){
		if(yp < 81){
			return (unsigned char)0x01;
		}else if(yp < 161)
		{
			return (unsigned char)0x04;
		}else if(yp < 241) {return (unsigned char)0x07;}
			
	}else if(xp < 215){
		if(yp < 81){
			return (unsigned char)0x02;
		}else if(yp < 161)
		{
			return (unsigned char)0x05;
		}else if(yp < 241) {return (unsigned char)0x08;}	

	}else if(xp < 321){
		if(yp < 81){
			return (unsigned char)0x03;
		}else if(yp < 161)
		{
			return (unsigned char)0x06;
		}else if(yp < 241) {return (unsigned char)0x09;}	
	}
	
	
/*
	int cy = y;
	
	cy = 240-cy;
	
	if(cy>160)
		pushSpeed(100,100);
	else if(cy>110)
		pushSpeed(70,70);
	else if(cy>60)
		pushSpeed(50,50);
	else 
		{
			pushSpeed(0,0);
			std_msgs::String str_msg;
			std::stringstream sd;
			sd << "getObject" ;
			str_msg.data = sd.str();
			usb_arm_pub.publish(str_msg);
			trackObjects = false;
			
			ROS_ERROR("--%d",sizeObj);
		}
*/
	
	
}

void objectTracerClass::Go(void)
{
	int ccx = x;
	int ccy =y;
	
	areaCode[0] = GoObject(ccx,ccy);
	ccx = (ccx *255)/320;
	
	cx[0]= (unsigned char)ccx;
	cy[0]= (unsigned char)ccy;

	
}

void objectTracerClass::trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed){

	Mat temp;
	threshold.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	//use moments method to find our filtered object

	int largestIndex = 0;
	bool objectFound = false;
	refArea = 0;
	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();
		//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
		if (numObjects<MAX_NUM_OBJECTS){
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {

				Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;

				//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we save a reference area each
				//iteration and compare it to the area in the next iteration.
				if (area>MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea){
					x = moment.m10 / area;
					y = moment.m01 / area;
					objectFound = true;
					refArea = area;
					//save index of largest contour to use with drawContours
					largestIndex = index;
				}
				

			}
			//let user know you found an object
			if (objectFound == true){
				putText(cameraFeed, "Tracking Object", Point(0, 50), 2, 1, Scalar(0, 255, 0), 2);
				//draw object location on screen
				drawObject(x, y, cameraFeed);
				//draw largest contour
				//drawContours(cameraFeed, contours, largestIndex, Scalar(0, 255, 255), 2);
				Go();
			}
		}
		else 
			putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 50), 1, 2, Scalar(0, 0, 255), 2);
	}else
				
			{
					x=0;y=0;
					cx[0]= 0;
					cy[0]= 0;
			}
}
void objectTracerClass::imageProccess()
{
	//some boolean variables for different functionality within this
	//program

	Camera.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
	Camera.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
	if(!Camera.open()){cerr<<"erro!!"<<endl; perror("camera open");exit(1);}
	//cameraFeed = cv_ptr->image;

	//video capture object to acquire webcam feed
	//VideoCapture capture;
	//open capture object at location zero (default location for webcam)
	//capture.open(0);
	//set height and width of capture frame
	//capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	//capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	//must create a window before setting mouse callback
	

	//start an infinite loop where webcam feed is copied to cameraFeed matrix
	//all of our operations will be performed within this loop
	while (1){
		//store image to matrix
		//capture.read(cameraFeed);
		Camera.grab();
		Camera.retrieve ( cameraFeed );
		//convert frame from BGR to HSV colorspace
		cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);
		//set HSV values from user selected region
		recordHSV_Values(cameraFeed, HSV);
		//filter HSV image between values and store filtered image to
		//threshold matrix
		inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold1);
		//perform morphological operations on thresholded image to eliminate noise
		//and emphasize the filtered object(s)
		if (useMorphOps)
			morphOps(threshold1);
		//pass in thresholded frame to our object tracking function
		//this function will return the x and y coordinates of the
		//filtered object
		if (trackObjects)
			trackFilteredObject(x, y, threshold1, cameraFeed);

		//show frames 
		if (calibrationMode == true){

			//create slider bars for HSV filtering
			createTrackbars();
			imshow(windowName1, HSV);
			imshow(windowName2, threshold1);
		}
		else{

			destroyWindow(windowName1);
			destroyWindow(windowName2);
			destroyWindow(trackbarWindowName);
		}
		imshow(windowName, cameraFeed);
		
		//delay 30ms so that screen can refresh.
		//image will not appear without this waitKey() command
		//also use waitKey command to capture keyboard input
		char key = waitKey(30);	
		if (key == 's') calibrationMode = !calibrationMode;//if user presses 'c', toggle calibration mode
		//printf("%x \n",key);
	}
	user_uart_close();
//	return ;
}

int main(int argc, char** argv){
	
	objectTracerClass *objectTC = new objectTracerClass();
	user_uart_open("ttyAMA0");	
	user_uart_config(115200);	
	namedWindow(windowName);
	//set mouse callback function to be active on "Webcam Feed" window
	//we pass the handle to our "frame" matrix so that we can draw a rectangle to it
	//as the user clicks and drags the mouse
	cv::setMouseCallback(windowName, clickAndDrag_Rectangle, &cameraFeed);
	//initiate mouse move and drag to false 
	mouseIsDragging = false;
	mouseMove = false;
	rectangleSelected = false;

	objectTC->imageProccess();
	return 0;
}

