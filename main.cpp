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
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
// #include <conio.h>
#include <ctime>

#include "IPM.h"

using namespace cv;
using namespace std;

Mat src, src_gray;
Mat dst, detected_edges, morph_closing, flood_fill;

int edgeThresh = 1;
int lowThreshold;
int const max_lowThreshold = 50;
int ratio = 3;
int kernel_size = 3;


/**
* @function CannyThreshold
* @brief Trackbar callback - Canny thresholds input with a ratio 1:3
*/
void CannyThreshold(int, void*)
{
	/// Reduce noise with a kernel 3x3
	blur(src_gray, detected_edges, Size(3, 3));

	/// Canny detector
	Canny(detected_edges, detected_edges, 30, 50, kernel_size);

	/// Using Canny's output as a mask, we display our result
	dst = Scalar::all(0);

	src.copyTo(dst, detected_edges);

	// namedWindow("Detected Edges", WINDOW_NORMAL);
	imshow("Detected Edges", detected_edges);
}

void MorphologicalClosing() 
{
	Point anchor = Point(-1,-1);
	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), anchor);
	morphologyEx(detected_edges, morph_closing, MORPH_CLOSE, kernel, anchor, 7);

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
	Mat im_out = (im_th | im_floodfill_inv);

	namedWindow("Flood Fill", WINDOW_NORMAL);
	imshow("Flood Fill", im_out);
}

int main(int _argc, char** _argv)
{

	// Images
	Mat inputImg, inputImgGray;
	Mat outputImg;

	VideoCapture video;

	video.open("CarsDrivingUnderBridge.mp4");

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
	int width = 0, dest_width= 350, height = 0, dest_height = 700, fps = 0, fourcc = 0;
	width = static_cast<int>(video.get(CV_CAP_PROP_FRAME_WIDTH));
	height = static_cast<int>(video.get(CV_CAP_PROP_FRAME_HEIGHT));
	fps = static_cast<int>(video.get(CV_CAP_PROP_FPS));
	fourcc = static_cast<int>(video.get(CV_CAP_PROP_FOURCC));

	cout << "Input video: (" << width << "x" << height << ") at " << fps << ", fourcc = " << fourcc << endl;

	// The 4-points at the input image	
	vector<Point2f> origPoints;
	origPoints.push_back(Point2f(120, 550)); //kiri bawah
	origPoints.push_back(Point2f(width, 550)); //kanan bawah
	origPoints.push_back(Point2f(width / 2 + 234, 140)); //kanan atas
	origPoints.push_back(Point2f(width / 2 - 165, 140)); //kiri atas

	// The 4-points correspondences in the destination image
	vector<Point2f> dstPoints;
	dstPoints.push_back(Point2f(0, dest_height));
	dstPoints.push_back(Point2f(dest_width, dest_height));
	dstPoints.push_back(Point2f(dest_width, 0));
	dstPoints.push_back(Point2f(0, 0));

	// IPM object
	IPM ipm(Size(width, height), Size(350, 700), origPoints, dstPoints);


	// Main loop
	int frameNum = 0;
	for (; ; )
	{
		printf("FRAME #%6d ", frameNum);
		fflush(stdout);
		frameNum++;

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
		printf("%.2f (ms)\r", 1000 * elapsed_secs);
		ipm.drawPoints(origPoints, inputImg);

		//canny edge detector
		/// Load an image
		src = outputImg;

		/// Create a matrix of the same type and size as src (for dst)
		dst.create(src.size(), src.type());

		/// Convert the image to grayscale
		cvtColor(src, src_gray, CV_BGR2GRAY);

		/// Create a window
		namedWindow("Detected Edges", WINDOW_NORMAL);

		/// Create a Trackbar for user to enter threshold
		// createTrackbar("Min Threshold:", "Detected Edges", &lowThreshold, max_lowThreshold, CannyThreshold);

		/// Run Canny
		CannyThreshold(0,0);

		/// Run Morph closing
		MorphologicalClosing();

		/// Run Flood Fill
		FloodFill();

		// View
		namedWindow("Input", WINDOW_NORMAL);
		imshow("Input", src_gray);

		namedWindow("Output", WINDOW_NORMAL);
		imshow("Output", outputImg);
		waitKey(1);
	}

	return 0;
}
