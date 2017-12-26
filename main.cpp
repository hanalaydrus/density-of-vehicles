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
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
// #include <conio.h>
#include <ctime>

#include "IPM.h"

using namespace cv;
using namespace std;

// configuration

// 1. Window and Video Information
Mat src, src_gray;
Mat dst;
String video_input = "CarsDrivingUnderBridge.mp4";

int width = 0;
int height = 0;
int fps = 0, fourcc = 0;
int frameNum = 0;

// 2. Detection Zone
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
	imshow("Canny Edge", detected_edges);
}

void MorphologicalClosing() 
{
	Point anchor = Point(-1,-1);
	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), anchor);
	morphologyEx(detected_edges, morph_closing, MORPH_CLOSE, kernel, anchor, interation);

	namedWindow("Morph Closing", WINDOW_NORMAL);
	imshow("Morph Closing", morph_closing);
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

	namedWindow("Flood Fill", WINDOW_NORMAL);
	imshow("Flood Fill", flood_fill);
}

void CalculateDensity()
{
	int count_white = countNonZero(flood_fill);
	density = count_white/(real_width*20*real_height*20);
	cout << "Density : " << density << endl;
}

void ShiTomasiCorner()
{
	double qualityThreshold = 0.02;
	double minDist = 30;
	int blockSize = 5;
	bool useHarrisDetector = false;
	double k = 0.07;
	Scalar color = Scalar(255, 0, 0);

	src_gray.copyTo(shi_tomasi);

	prevFrameNum = frameNum;

	goodFeaturesToTrack(shi_tomasi, prevPts, 40, qualityThreshold, minDist, Mat(), blockSize, useHarrisDetector, k);

	cout << "prevpts size " << prevPts.size() << endl;

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
	int duration;
	float oneSpeed, totalSpeed;
	TermCriteria termcrit(TermCriteria::COUNT|TermCriteria::EPS,20,0.03);

	src_gray.copyTo(nextImg);
	if (!prevImg.empty() && !nextImg.empty() && !prevPts.empty()) 
	{
		calcOpticalFlowPyrLK(prevImg, nextImg, prevPts, nextPts, status, err, Size(11,22), 3, termcrit, 0, 0.001);
	}

	// for (size_t i = 0; i < err.size(); i++)
	// {
	// 	cout << "Error :" << err[i] << endl;
	// }
	// Count Average Speed
	nextFrameNum = frameNum;
	duration = (nextFrameNum - prevFrameNum)*fps;

	for (size_t i = 0; i < nextPts.size(); i++)
	{
		distance = norm(nextPts[i]-prevPts[i])/20;
		oneSpeed = distance/duration;
		totalSpeed += oneSpeed;
	}
	speed = totalSpeed/nextPts.size();
	cout << "Duration : " << duration << endl;
	cout << "Speed : " << speed << endl;
	
	// For next detection
	src_gray.copyTo(prevImg);
	prevFrameNum = frameNum;
	prevPts = nextPts;
}

int main(int _argc, char** _argv)
{

	// Images
	Mat inputImg, inputImgGray;
	Mat outputImg;

	VideoCapture video;

	video.open(video_input);

	if (!video.isOpened()) {                                                 // if unable to open video file
		cout << "error reading video file" << endl << endl;      // show error message
		// _getch();                   // it may be necessary to change or remove this line if not using Windows
		return(0);                                                              // and exit program
	}

	if (video.get(CV_CAP_PROP_FRAME_COUNT) < 2) {
		cout << "error: video file must have at least two frames";
		// _getch();                   // it may be necessary to change or remove this line if not using Windows
		return(0);
	}

	// Show video information
	width = static_cast<int>(video.get(CV_CAP_PROP_FRAME_WIDTH));
	height = static_cast<int>(video.get(CV_CAP_PROP_FRAME_HEIGHT));
	fps = static_cast<int>(video.get(CV_CAP_PROP_FPS));
	fourcc = static_cast<int>(video.get(CV_CAP_PROP_FOURCC));

	cout << "Input video: (" << width << "x" << height << ") at " << fps << ", fourcc = " << fourcc << endl;

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

	// Initial Detection
	

	// Main loop
	for (; ; )
	{
		frameNum++;
		printf("FRAME #%6d ", frameNum);
		fflush(stdout);
		
		// Get current image		
		video >> inputImg;
		if (inputImg.empty())
			break;

		// Color Conversion
		if (inputImg.channels() == 3)
			cvtColor(inputImg, inputImgGray, CV_BGR2GRAY);
		else
			inputImg.copyTo(inputImgGray);

		// Process
		clock_t begin = clock();
		ipm.applyHomography(inputImg, outputImg);
		clock_t end = clock();
		double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
		// printf("%.2f (ms)\r", 1000 * elapsed_secs);
		ipm.drawPoints(origPoints, inputImg);

		//canny edge detector
		/// Load an image
		src = outputImg;

		/// Create a matrix of the same type and size as src (for dst)
		dst.create(src.size(), src.type());

		/// Convert the image to grayscale
		cvtColor(src, src_gray, CV_BGR2GRAY);

		/// Create a window
		namedWindow("Canny Edge", WINDOW_NORMAL);

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
		if (frameNum == 1) {
			ShiTomasiCorner();
			src_gray.copyTo(prevImg);
		}

		// Lucas Kanade
		if (frameNum > 1) {
			LucasKanade();
		}

		// View
		namedWindow("Input", WINDOW_NORMAL);
		imshow("Input", inputImg);

		namedWindow("Output", WINDOW_NORMAL);
		imshow("Output", outputImg);
		waitKey(1);
	}

	return 0;
}
