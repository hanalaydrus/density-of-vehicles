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
#include <vector>
#include <map>
#include "boost/variant.hpp"
#include "densityContract.grpc.pb.h"

#include "IPM.h"
#include "Model.h"

using namespace cv;
using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using densityContract::HelloRequest;
using densityContract::HelloReply;
using densityContract::Greeter;

// configuration
int frameNumReal = 0;
vector< Mat > inputImgArr;
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

// 3. Canny Edge
Mat detected_edges;
int ratio = 3;
int kernel_size = 3;

// 4. Morphological Closing
Mat morph_closing;

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

//////////////////////////////////////////////////////////////////////

void GetFrame(string url)
{
	VideoCapture cap(url);

	if (cap.isOpened()){
		// Show video information
		width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
		height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
		fps = static_cast<int>(cap.get(CAP_PROP_FPS));
		fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));

		// std::cout << "Input video: (" << width << "x" << height << ") at " << fps << ", fourcc = " << fourcc << endl;
	}
	for (;;) 
	{
		// Get current image
		cap >> inputImgReal;
		if (inputImgReal.empty())
		{
			std::cout << "Input image empty" << endl;
			cap.open(url);
			if (cap.isOpened()){
				// Show video information
				width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
				height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
				fps = static_cast<int>(cap.get(CAP_PROP_FPS));
				fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));
	
				// std::cout << "Input video: (" << width << "x" << height << ") at " << fps << ", fourcc = " << fourcc << endl;
			}
			continue;
		}

		inputImgArr.push_back(inputImgReal);

		frameNumReal++;
	}
}

void BirdEyeView(map<string, int> mask_points, int real_width, int real_height)
{
	// x_lb = 120, y_lb = 550;
	// x_rb = width, y_rb = 550;
	// x_rt = (width / 2 + 234), y_rt = 140;
	// x_lt = (width / 2 - 165), y_lt = 140;

	// cout << "x_rb : " << x_rb << endl;
	// cout << "x_rt : " << x_rt << endl;
	// cout << "x_lt : " << x_lt << endl;

	// The 4-points at the input image	
	vector<Point2f> origPoints;
	origPoints.push_back(Point2f(mask_points["x_lb"], mask_points["y_lb"])); //kiri bawah
	origPoints.push_back(Point2f(mask_points["x_rb"], mask_points["y_rb"])); //kanan bawah
	origPoints.push_back(Point2f(mask_points["x_rt"], mask_points["y_rt"])); //kanan atas
	origPoints.push_back(Point2f(mask_points["x_lt"], mask_points["y_lt"])); //kiri atas

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

void CannyEdge(int, void*, int lowThreshold, int max_lowThreshold)
{
	/// Reduce noise with a kernel 3x3
	blur(src_gray, detected_edges, Size(3, 3));

	/// Canny detector
	cv::Canny(detected_edges, detected_edges, lowThreshold, max_lowThreshold, kernel_size);

	/// Using Canny's output as a mask, we display our result
	dst = Scalar::all(0);

	src.copyTo(dst, detected_edges);

	// namedWindow("Canny Edge", WINDOW_NORMAL);
	// imshow("Canny Edge", detected_edges);
}

void MorphologicalClosing(int interation) 
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
    cv::bitwise_not(flood_fill, im_floodfill_inv);
	flood_fill = (im_th | im_floodfill_inv);

	// namedWindow("Flood Fill", WINDOW_NORMAL);
	// imshow("Flood Fill", flood_fill);
}

void CalculateDensity(int real_width, int real_height)
{
	int count_white = countNonZero(flood_fill);
	density = count_white/(real_width*20*real_height*20);
	// std::cout << endl <<"Density : " << density << endl;
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
		// std::cout << "Speed : " << speed << endl;
		
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

void TrafficState(ServerWriter<HelloReply>* writer)
{	
	if (density < 0.5) {
		// output stream
		HelloReply r;
		r.set_response("Lancar");
		writer->Write(r);
		// output to console
		// cout << "Lancar" << endl << endl;
	} else if ((density >= 0.5) && (speed > 0.5)){
		// output stream
		HelloReply r;
		r.set_response("Ramai Lancar");
		writer->Write(r);

		// output to console
		// cout << "Ramai Lancar" << endl << endl;
	} else {
		// output stream
		HelloReply r;
		r.set_response("Padat");
		writer->Write(r);

		// output to console
		// cout << "Padat" << endl << endl;
	}
}

void RunService(
	ServerWriter<HelloReply>* writer,
	int real_width,
	int real_height,
	map<string, int> mask_points,
	int edge_thresh,
	int low_thresh,
	int max_thresh,
	int morph_iteration)
{
	// Main loop
	for (; ; )
	{
		if (inputImgReal.empty() && frameNum != 0){ break;}
		if (inputImgReal.empty()){ continue;}

		if (frameNumReal>1) {
			inputImgReal.copyTo(inputImg);
			inputImgArr[frameNumReal-2].copyTo(prevImg);
		} else if (frameNumReal <= 1) {
			inputImgReal.copyTo(inputImg);
			inputImgReal.copyTo(prevImg);
		}
		frameNum++;
		// std::cout << "FRAME : " << frameNum << " REAL : " << frameNumReal << endl;

		// Bird Eye View First
		bird_eye_view = prevImg;
		BirdEyeView(mask_points, real_width, real_height);
		prevImg = bird_eye_view;
		cv::cvtColor(prevImg, prevImg, CV_BGR2GRAY);

		// Bird Eye View Second
		bird_eye_view = inputImg;
		
		BirdEyeView(mask_points, real_width, real_height);

		//canny edge detector
		/// Load an image
		src = bird_eye_view;

		/// Create a matrix of the same type and size as src (for dst)
		dst.create(src.size(), src.type());

		/// Convert the image to grayscale
		cv::cvtColor(src, src_gray, CV_BGR2GRAY);

		/// Create a window
		// namedWindow("Canny Edge", WINDOW_NORMAL);

		/// Create a Trackbar for user to enter threshold
		// createTrackbar("Min Threshold:", "Canny Edge", &lowThreshold, max_lowThreshold, CannyEdge);

		/// Run Canny
		CannyEdge(0, 0, low_thresh, max_thresh);

		/// Run Morph closing
		MorphologicalClosing(morph_iteration);

		/// Run Flood Fill
		FloodFill();

		// Calculate density
		CalculateDensity(real_width, real_height);

		// Shi Tomasi Corner Detection
		ShiTomasiCorner();

		// Lucas Kanade
		LucasKanade();

		// Traffic State
		TrafficState(writer);

		// View
		namedWindow("Input", WINDOW_NORMAL);
		cv::imshow("Input", inputImg);

		// namedWindow("Output", WINDOW_NORMAL);
		// imshow("Output", outputImg);
		waitKey(1);
	}
}

class GreeterServiceImpl final : public Greeter::Service {
	Status SayHello(ServerContext* context,
					const HelloRequest* request,
					ServerWriter<HelloReply>* writer) override {

		Model model;
		map<string, boost::variant<int, string, map<string, int> > > cameraConfig;
		cameraConfig = model.getCameraConfig(request->id());
		
		thread tGetFrame (GetFrame,boost::get<string>(cameraConfig["url"]));

		thread tRunService( 
			RunService,
			writer,
			boost::get<int>(cameraConfig["real_width"]),
			boost::get<int>(cameraConfig["real_height"]),
			boost::get< map<string, int> >(cameraConfig["mask_points"]),
			boost::get<int>(cameraConfig["edge_thresh"]),
			boost::get<int>(cameraConfig["low_thresh"]),
			boost::get<int>(cameraConfig["max_thresh"]),
			boost::get<int>(cameraConfig["morph_iteration"])
		);

		tGetFrame.join();
		tRunService.join();

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
	RunServer();

	return 0;
}
