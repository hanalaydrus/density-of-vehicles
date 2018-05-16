#include "Density.h"

Density::Density(){

}

//////////////////////////////////////////////////////////////////////

Mat BirdEyeView(Mat input_image, map<string, int> mask_points, int width, int height,  int real_width, int real_height)
{
	Mat bird_eye_view, input_image_points;

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
	
	// Draw Mask Street
	input_image.copyTo(input_image_points);
	ipm.drawPoints(origPoints, input_image_points);
	
	// namedWindow("Street Mask", WINDOW_NORMAL);
	// imshow("Street Mask", input_image_points);

	// Apply Bird Eye View or Homography
	ipm.applyHomography(input_image, bird_eye_view);

	// namedWindow("Bird Eye View", WINDOW_NORMAL);
	// imshow("Bird Eye View", bird_eye_view);
	
	
	return bird_eye_view;

}

/**
* @function CannyEdge
* @brief Trackbar callback - Canny thresholds input with a ratio 1:3
*/

Mat CannyEdge(int, void*, Mat input_image, int low_threshold, int max_lowThreshold)
{
	Mat detected_edges;
	int ratio = 3;
	int kernel_size = 3;

	/// Reduce noise with a kernel 3x3
	blur(input_image, detected_edges, Size(3, 3));
 
	/// Canny detector
	cv::Canny(detected_edges, detected_edges, low_threshold, max_lowThreshold, kernel_size);

	// namedWindow("Canny Edge", WINDOW_NORMAL);
	// imshow("Canny Edge", detected_edges);

	return detected_edges;
}

Mat MorphologicalClosing(Mat input_image, int iteration) 
{
	Mat morph_closing;

	Point anchor = Point(-1,-1);
	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), anchor);
	morphologyEx(input_image, morph_closing, MORPH_CLOSE, kernel, anchor, iteration);

	// namedWindow("Morph Closing", WINDOW_NORMAL);
	// imshow("Morph Closing", morph_closing);

	return morph_closing;
}

Mat FloodFill(Mat input_image)
{
	Mat im_th;
	Mat flood_fill;

	input_image.copyTo(flood_fill);
	input_image.copyTo(im_th);
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

	return flood_fill;
}

float CalculateDensity(Mat input_image, int real_width, int real_height)
{
	float density = 0;
	int count_white = countNonZero(input_image);
	// std::cout << endl <<"count white : " << count_white << endl;
	density = (float) count_white / (float) (real_width*20*real_height*20);
	// std::cout << endl <<"Density : " << density << endl;
	return density;
}

vector<Point2f> ShiTomasiCorner(Mat input_image)
{
	Mat shi_tomasi;
	vector<Point2f> prevPts;

	double qualityThreshold = 0.02;
	double minDist = 30;
	int blockSize = 5;
	bool useHarrisDetector = false;
	double k = 0.07;
	Scalar color = Scalar(255, 0, 0);

	input_image.copyTo(shi_tomasi);

	goodFeaturesToTrack(shi_tomasi, prevPts, 40, qualityThreshold, minDist, Mat(), blockSize, useHarrisDetector, k);

	// for(size_t i = 0; i < prevPts.size(); i++)
	// {
	// 	circle(shi_tomasi, prevPts[i], 8, color, 2, 8, 0);
	// }
	
	// namedWindow("Shi Tomasi", WINDOW_NORMAL);
	// imshow("Shi Tomasi", shi_tomasi);

	return prevPts;
}

float LucasKanade(Mat input_img_gray, Mat prev_img_gray, vector<Point2f> prevPts, float fps)
{
	vector<Point2f> nextPts;
	vector<uchar> status;
	vector<float> err;
	double distance;
	double duration;
	double speed, oneSpeed, totalSpeed;
	TermCriteria termcrit(TermCriteria::COUNT|TermCriteria::EPS,20,0.03);
	int count = 0;

	if (!prev_img_gray.empty() && !input_img_gray.empty() && !prevPts.empty()) 
	{
		calcOpticalFlowPyrLK(prev_img_gray, input_img_gray, prevPts, nextPts, status, err, Size(11,22), 3, termcrit, 0, 0.001);
		
		// Count Average Speed
		duration = ((double)(1)/(double)fps);
		// cout << "Duration : " << nextFrameNum << "-"<< prevFrameNum << "/" << fps << endl;
		// cout << "Duration : " << duration << endl;
		// cout << "FPS : " << fps << endl;

		for (size_t i = 0; i < nextPts.size(); i++)
		{
			if (status[i] == 1) {
				distance = ((nextPts[i].y-prevPts[i].y)/20);
				// cout << "Distance : " << distance << endl;
				oneSpeed = ((double)distance/duration);
				// cout << "OneSpeed : " << oneSpeed << endl;
				if (oneSpeed >= 0){
					totalSpeed += oneSpeed;
					count++;
				}
			}
		}
		// std::cout << "Total Speed : " << totalSpeed << endl;
		// std::cout << "Count : " << count << endl;
		if (count <= 0){
			speed = 0;
		} else {
			speed = (totalSpeed/(double)count);
		}
		// std::cout << "Speed : " << speed << endl;
		
		RNG rng(12345);
		for(size_t i = 0; i < prevPts.size(); i++)
		{
			Scalar color = Scalar(rng.uniform(0,255), rng.uniform(0, 255), rng.uniform(0, 255));
			circle(prev_img_gray, prevPts[i], 8, color, 2, 8, 0);
			circle(input_img_gray, nextPts[i], 8, color, 2, 8, 0);
		}
		// namedWindow("Lucas Kanade Prev", WINDOW_NORMAL);
		// imshow("Lucas Kanade Prev", prev_img_gray);

		// namedWindow("Lucas Kanade", WINDOW_NORMAL);
		// imshow("Lucas Kanade", input_img_gray);

		return speed;
	}
}

string TrafficState(float density, float speed)
{	
	string traffic_state;

	if (density < 0.5) {
		// set traffic state
		traffic_state = "Lancar";
		// output to console
		// cout << "Lancar" << endl << endl;
	} else if ((density >= 0.5) && (speed > 0.5)){
		// set traffic state
		traffic_state = "Ramai Lancar";
		// output to console
		// cout << "Ramai Lancar" << endl << endl;
	} else {
		// set traffic state
		traffic_state = "Padat";
		// output to console
		// cout << "Padat" << endl << endl;
	}
	return traffic_state;
}

float CountFPS(VideoCapture cap){
	int num_frames = 120;
	time_t start, end;
	Mat frame;

	time(&start);
	for (int i = 0; i < num_frames; ++i){
		cap >> frame;
	}
	time(&end);
	
	float seconds = difftime(end, start);
	return num_frames/seconds;
}

void Density::runDensityService(
	int camera_id,
	string url,
	int real_width,
	int real_height,
	map<string, int> mask_points,
	int edge_thresh,
	int low_thresh,
	int max_thresh,
	int morph_iteration)
{
	int width = 0;
	int height = 0;
	float fps = 0;
	int fourcc = 0;

	Model model;
	Mat inputImg, inputImg_gray, prevImg;
	vector<Point2f> shi_tomasi_prevPts;
	float density, speed;
	string traffic_state;
	string prev_traffic_state = "";

	VideoCapture cap(url);
	if (cap.isOpened()){
		width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
		// std::cout << "width: " << width << endl;
		height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
		// std::cout << "height: " << height << endl;
		fps = CountFPS(cap);
		if (isinf(fps)) {
			fps = static_cast<int>(cap.get(CAP_PROP_FPS));
		}
		fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));
	}

	// Main loop
	for (; ; )
	{
		cap >> prevImg;
		cap >> inputImg;

		if (prevImg.empty() || inputImg.empty()){

			std::cout << "Input image empty" << endl;

			cap.open(url);
			if (cap.isOpened()){
				std::cout << "Opeen " << height << endl;
				width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
				height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
				fps = CountFPS(cap);
				if (isinf(fps)) {
					fps = static_cast<int>(cap.get(CAP_PROP_FPS));
				}
				fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));
			}

			continue;
		}
		// Show raw input image
		// namedWindow("Input", WINDOW_NORMAL);
		// cv::imshow("Input", inputImg);

		// Show raw input image
		// namedWindow("Prev Img", WINDOW_NORMAL);
		// cv::imshow("Prev Img", prevImg);
		
		// Bird Eye View First
		prevImg = BirdEyeView(prevImg, mask_points, width, height, real_width, real_height);
		/// Convert the image to grayscale
		cv::cvtColor(prevImg, prevImg, CV_BGR2GRAY);

		// Bird Eye View Second
		inputImg = BirdEyeView(inputImg, mask_points, width, height, real_width, real_height);
		/// Convert the image to grayscale
		cv::cvtColor(inputImg, inputImg, CV_BGR2GRAY);
		inputImg.copyTo(inputImg_gray);

		/// Create a window
		// namedWindow("Canny Edge", WINDOW_NORMAL);

		/// Create a Trackbar for user to enter threshold
		// createTrackbar("Min Threshold:", "Canny Edge", &lowThreshold, max_lowThreshold, CannyEdge);

		/// Run Canny
		inputImg = CannyEdge(0, 0, inputImg, low_thresh, max_thresh);

		/// Run Morph closing
		inputImg = MorphologicalClosing(inputImg, morph_iteration);

		/// Run Flood Fill
		inputImg = FloodFill(inputImg);

		// Calculate density
		density = CalculateDensity(inputImg, real_width, real_height);

		// Shi Tomasi Corner Detection
		shi_tomasi_prevPts = ShiTomasiCorner(prevImg);

		// Lucas Kanade
		speed = LucasKanade(inputImg_gray, prevImg, shi_tomasi_prevPts, fps);

		// Traffic State
		traffic_state = TrafficState(density, speed);

		if (traffic_state != prev_traffic_state) {
			//insert to mysql density history
			model.storeDensityData(camera_id, traffic_state);
			prev_traffic_state = traffic_state;

		}

		waitKey(1);
	}
}