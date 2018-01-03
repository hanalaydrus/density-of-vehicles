/*
* Project:  Inverse Perspective Mapping
*
* File:     main.cpp
*
* Contents: Creation, initialisation and usage of IPM object
*           for the generation of Inverse Perspective Mappings of images or videos
*
* Author:   Marcos Nieto <marcos.nieto.doncel@gmail.com>
* Date:	 22/02/2014
* Homepage: http://marcosnietoblog.wordpress.com/
*/

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/videoio.hpp"

#include <grpc++/grpc++.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <ctime>
#include <memory>
#include <string>

#include "helloworld.grpc.pb.h"

#include "IPM.h"

using namespace cv;
using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

// configuration
int frameNumReal = 0;
vector<Mat> inputImgArr;
Mat inputImg, inputImgReal;

// 1. Window and Video Information
Mat src, src_gray;
Mat dst;

int width = 0;
int height = 0;
int fps = 0, fourcc = 0;
int frameNum = 0;

// 2. Bird Eye View
Mat bird_eye_view;

int x_ld, y_ld;
int x_rd, y_rd;
int x_rt, y_rt;
int x_lt, y_lt;

float real_width = 17.5;
float real_height = 35;

// 3. Canny Edge
Mat detected_edges;
int edgeThresh = 1;
int lowThreshold = 30;
int const max_lowThreshold = 50;
int ratio = 3;
int kernel_size = 3;

// 4. Morphological Closing
Mat morph_closing;
int interation = 7;

// 5. Flood Fill
Mat flood_fill;

// 6. Count Density
float density = 0;

// 7. Shi Tomasi
Mat shi_tomasi;

// 8. Lucas Kanade Tracker
Mat prevImg, nextImg;
vector<Point2f> prevPts, nextPts;
int prevFrameNum, nextFrameNum;
float speed;

// 9. Traffic State
String traffic_state;

void GetFrame()
{
	VideoCapture cap("http://127.0.0.1:5000/video_feed");

	if (!cap.isOpened()) {                                                 // if unable to open video file
		cout << "error reading video file" << endl << endl;      // show error message
		// _getch();                   // it may be necessary to change or remove this line if not using Windows
		// return(0);                                                              // and exit program
	}

	// Show video information
	width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
	height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
	fps = static_cast<int>(cap.get(CAP_PROP_FPS));
	fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));

	cout << "Input video: (" << width << "x" << height << ") at " << fps << ", fourcc = " << fourcc << endl;
	// Get current image
	for (;;) 
	{
		cap >> inputImgReal;
		if (inputImgReal.empty())
		{
			cout << "Input image empty" << endl;
			continue;
		}
		inputImgArr.push_back(inputImgReal);
		frameNumReal++;
	}
}

void BirdEyeView()
{
	x_ld = 120, y_ld = 550;
	x_rd = width, y_rd = 550;
	x_rt = (width / 2 + 234), y_rt = 140;
	x_lt = (width / 2 - 165), y_lt = 140;

	// The 4-points at the input image	
	vector<Point2f> origPoints;
	origPoints.push_back(Point2f(x_ld, y_ld)); //kiri bawah
	origPoints.push_back(Point2f(x_rd, y_rd)); //kanan bawah
	origPoints.push_back(Point2f(x_rt, y_rt)); //kanan atas
	origPoints.push_back(Point2f(x_lt, y_lt)); //kiri atas

	// The 4-points correspondences in the destination image
	vector<Point2f> dstPoints;
	dstPoints.push_back(Point2f(0, real_height*20));
	dstPoints.push_back(Point2f(real_width*20, real_height*20));
	dstPoints.push_back(Point2f(real_width*20, 0));
	dstPoints.push_back(Point2f(0, 0));

	// IPM object
	IPM ipm(Size(width, height), Size(real_width*20, real_height*20), origPoints, dstPoints);

	ipm.applyHomography(inputImg, bird_eye_view);
	ipm.drawPoints(origPoints, inputImg);
}

/**
* @function CannyEdge
* @brief Trackbar callback - Canny thresholds input with a ratio 1:3
*/

void CannyEdge(int, void*)
{
	/// Reduce noise with a kernel 3x3
	blur(src_gray, detected_edges, Size(3, 3));

	/// Canny detector
	Canny(detected_edges, detected_edges, lowThreshold, max_lowThreshold, kernel_size);

	/// Using Canny's output as a mask, we display our result
	dst = Scalar::all(0);

	src.copyTo(dst, detected_edges);

	// namedWindow("Canny Edge", WINDOW_NORMAL);
	// imshow("Canny Edge", detected_edges);
}

void MorphologicalClosing() 
{
	Point anchor = Point(-1,-1);
	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), anchor);
	morphologyEx(detected_edges, morph_closing, MORPH_CLOSE, kernel, anchor, interation);

	// namedWindow("Morph Closing", WINDOW_NORMAL);
	// imshow("Morph Closing", morph_closing);
}

void FloodFill()
{
	Mat im_th;
	morph_closing.copyTo(flood_fill);
	morph_closing.copyTo(im_th);
	for (int i = 0; i < flood_fill.cols; i++) {
		if (flood_fill.at<char>(0, i) == 0) {
			floodFill(flood_fill, cv::Point(i, 0), 255, 0, 10, 10);
		}   
		if (flood_fill.at<char>(flood_fill.rows-1, i) == 0) {
			floodFill(flood_fill, cv::Point(i, flood_fill.rows-1), 255, 0, 10, 10);
		}
	}
	for (int i = 0; i < flood_fill.rows; i++) {
		if (flood_fill.at<char>(i, 0) == 0) {
			floodFill(flood_fill, cv::Point(0, i), 255, 0, 10, 10);
		}
		if (flood_fill.at<char>(i, flood_fill.cols-1) == 0) {
			floodFill(flood_fill, cv::Point(flood_fill.cols-1, i), 255, 0, 10, 10);
		}
	}

	Mat im_floodfill_inv;
    bitwise_not(flood_fill, im_floodfill_inv);
	flood_fill = (im_th | im_floodfill_inv);

	// namedWindow("Flood Fill", WINDOW_NORMAL);
	// imshow("Flood Fill", flood_fill);
}

void CalculateDensity()
{
	int count_white = countNonZero(flood_fill);
	density = count_white/(real_width*20*real_height*20);
	cout << endl <<"Density : " << density << endl;
}

void ShiTomasiCorner()
{
	double qualityThreshold = 0.02;
	double minDist = 30;
	int blockSize = 5;
	bool useHarrisDetector = false;
	double k = 0.07;
	Scalar color = Scalar(255, 0, 0);

	prevImg.copyTo(shi_tomasi);

	goodFeaturesToTrack(shi_tomasi, prevPts, 40, qualityThreshold, minDist, Mat(), blockSize, useHarrisDetector, k);

	// for(size_t i = 0; i < prevPts.size(); i++)
	// {
	// 	circle(shi_tomasi, prevPts[i], 8, color, 2, 8, 0);
	// }
	
	// namedWindow("Shi Tomasi", WINDOW_NORMAL);
	// imshow("Shi Tomasi", shi_tomasi);
}

void LucasKanade()
{
	vector<uchar> status;
	vector<float> err;
	double distance;
	float duration;
	float oneSpeed, totalSpeed;
	TermCriteria termcrit(TermCriteria::COUNT|TermCriteria::EPS,20,0.03);
	int count = 0;

	src_gray.copyTo(nextImg);
	if (!prevImg.empty() && !nextImg.empty() && !prevPts.empty()) 
	{
		calcOpticalFlowPyrLK(prevImg, nextImg, prevPts, nextPts, status, err, Size(11,22), 3, termcrit, 0, 0.001);
		
		// Count Average Speed
		duration = ((float)(1)/(float)fps);
		// cout << "Duration : " << nextFrameNum << "-"<< prevFrameNum << "/" << fps << endl;
		// cout << "Duration : " << duration << endl;

		for (size_t i = 0; i < nextPts.size(); i++)
		{
			if (status[i] == 1) {
				distance = ((nextPts[i].y-prevPts[i].y)/20);
				// cout << "Distance : " << distance << endl;
				oneSpeed = ((float)distance/duration);
				// cout << "OneSpeed : " << oneSpeed << endl;
				if (oneSpeed < 0){oneSpeed = 0;}
				totalSpeed += oneSpeed;
				count++;
			}
		}
		speed = (totalSpeed/(float)count);
		cout << "Speed : " << speed << endl;
		
		// RNG rng(12345);
		// for(size_t i = 0; i < prevPts.size(); i++)
		// {
		// 	Scalar color = Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255));
		// 	circle(src, prevPts[i], 8, color, 2, 8, 0);
		// 	circle(src, nextPts[i], 8, color, 2, 8, 0);
		// }
		// namedWindow("Lucas Kanade", WINDOW_NORMAL);
		// imshow("Lucas Kanade", src);
	}
}

void TrafficState()
{	
	if (density < 0.5) {
		// change traffic state
		traffic_state = "Lancar";
		// output to console
		cout << "Lancar" << endl << endl;
	} else if ((density >= 0.5) && (speed > 0.5)){
		// change traffic state
		traffic_state = "Ramai Lancar";
		// output to console
		cout << "Ramai Lancar" << endl << endl;
	} else {
		// change traffic state
		traffic_state = "Padat";
		// output to console
		cout << "Padat" << endl << endl;
	}
}

void Run(){

	// Main loop
	for (; ; )
	{
		if (inputImgReal.empty() && frameNum != 0){break;}
		if (inputImgReal.empty()){continue;}

		if (frameNumReal>1) {
			inputImgReal.copyTo(inputImg);
			inputImgArr[frameNumReal-2].copyTo(prevImg);
		} else if (frameNumReal <= 1) {
			inputImgReal.copyTo(inputImg);
			inputImgReal.copyTo(prevImg);
		}
		frameNum++;
		cout << "FRAME : " << frameNum << " REAL : " << frameNumReal << endl;

		// Bird Eye View First
		bird_eye_view = prevImg;
		BirdEyeView();
		prevImg = bird_eye_view;
		cvtColor(prevImg, prevImg, CV_BGR2GRAY);

		// Bird Eye View Second
		bird_eye_view = inputImg;
		
		BirdEyeView();

		//canny edge detector
		/// Load an image
		src = bird_eye_view;

		/// Create a matrix of the same type and size as src (for dst)
		dst.create(src.size(), src.type());

		/// Convert the image to grayscale
		cvtColor(src, src_gray, CV_BGR2GRAY);

		/// Create a window
		// namedWindow("Canny Edge", WINDOW_NORMAL);

		/// Create a Trackbar for user to enter threshold
		// createTrackbar("Min Threshold:", "Canny Edge", &lowThreshold, max_lowThreshold, CannyEdge);

		/// Run Canny
		CannyEdge(0,0);

		/// Run Morph closing
		MorphologicalClosing();

		/// Run Flood Fill
		FloodFill();

		// Calculate density
		CalculateDensity();

		// Shi Tomasi Corner Detection
		ShiTomasiCorner();

		// Lucas Kanade
		LucasKanade();

		// Traffic State
		TrafficState();

		// View
		namedWindow("Input", WINDOW_NORMAL);
		imshow("Input", inputImg);

		// namedWindow("Output", WINDOW_NORMAL);
		// imshow("Output", outputImg);
		waitKey(1);
	}
}

class GreeterServiceImpl final : public Greeter::Service {
	Status SayHello(ServerContext* context,
					const HelloRequest* request,
					ServerWriter<HelloReply>* writer) override {
		
		HelloReply r;
		for (; ; )
		{
			r.set_message(traffic_state);
			writer->Write(r);
		}
	  	return Status::OK;
	}
};

void RunServer() {
	std::string server_address("0.0.0.0:50050");
	GreeterServiceImpl service;
  
	ServerBuilder builder;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	// Finally assemble the server.
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;
  
	// Wait for the server to shutdown. Note that some other thread must be
	// responsible for shutting down the server for this call to ever return.
	server->Wait();
}

int main(int _argc, char** _argv)
{
	thread first (GetFrame);
	thread second (Run);
	thread third (RunServer);

	first.join();
	second.join();
	third.join();
	
	return 0;
}
