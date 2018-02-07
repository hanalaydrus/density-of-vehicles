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
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

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

mutex mtx;

// configuration
int frameNumReal = 0;
vector< vector<Mat> > inputImgArr;
vector<Mat> inputImgReal;
Mat inputImg;

// 1. Window and Video Information
Mat src, src_gray;
Mat dst;

int width = 0;
int height = 0;
int fps = 0, fourcc = 0;
int frameNum = 0;

// 2. Bird Eye View
Mat bird_eye_view;

int x_lb = 120, y_lb = 550;
int x_rb = 1280, y_rb = 550;
int x_rt = 874, y_rt = 140;
int x_lt = 475, y_lt = 140;

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

//////////////////////////////////////////////////////////////////////

vector<string> GetDensityByID(int camera_id) {
	// Input : camera_id
	// Output : time, density
	vector<string> response;
	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;
	  
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://127.0.0.1:3306", "root", "root");
		/* Connect to the MySQL test database */
		con->setSchema("density");
	  
		stmt = con->createStatement();
		ostringstream query;
		query << "SELECT * FROM `density_history` WHERE `camera_id` = " << camera_id << " ORDER BY `date_time` DESC LIMIT 1";
		res = stmt->executeQuery(query.str());
		while (res->next()) {
			response.push_back(res->getString("date_time"));
			response.push_back(res->getString("density_state"));
		}
		delete res;
		delete stmt;
		delete con;
	  
	  } catch (sql::SQLException &e) {
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	  }

	  return response;
}

void StoreDensityData(int camera_id, string density_state) {
	// Input : id, density_data
	// Output : -
	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
	  
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://127.0.0.1:3306", "root", "root");
		/* Connect to the MySQL test database */
		con->setSchema("density");
	  
		stmt = con->createStatement();
		ostringstream query;
		query << "INSERT INTO `density_history`(`camera_id`, `density_state`) VALUES ("<< camera_id <<",'Padat')";
		stmt->execute(query.str());
		
		delete stmt;
		delete con;
	  
	  } catch (sql::SQLException &e) {
		cout << "# ERR: SQLException in store density, " << __FILE__;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	  }
	
}

vector< map<string, boost::variant<int, string>> > GetCameras() {
	// Input : -
	// Output : Array of Map
	vector< map<string, boost::variant<int, string>> > cameras;
	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;
	  
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://127.0.0.1:3306", "root", "root");
		/* Connect to the MySQL test database */
		con->setSchema("camera");
	  
		stmt = con->createStatement();
		res = stmt->executeQuery("SELECT * FROM `camera`");
		while (res->next()) {
		  /* Access column data by alias or column name */
			map<string, boost::variant<int, string>> data;
			data["camera_id"] = res->getInt("camera_id");
			data["url"] = res->getString("url");
			
			cameras.push_back(data);
		}
		
		delete res;
		delete stmt;
		delete con;
	  
	  } catch (sql::SQLException &e) {
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	  }
	  
	  return cameras;
}

//////////////////////////////////////////////////////////////////////

void GetFrame(int camera_id, string url)
{
	VideoCapture cap(url);

	if (cap.isOpened()){
		// Show video information
		width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
		height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
		fps = static_cast<int>(cap.get(CAP_PROP_FPS));
		fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));

		cout << "Input video: (" << width << "x" << height << ") at " << fps << ", fourcc = " << fourcc << endl;
	}
	for (;;) 
	{
		// Get current image
		mtx.lock();
		cap >> inputImgReal[camera_id];
		mtx.unlock();
		if (inputImgReal[camera_id].empty())
		{
			cout << "Input image empty" << endl;
			cap.open(url);
			if (cap.isOpened()){
				// Show video information
				width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
				height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
				fps = static_cast<int>(cap.get(CAP_PROP_FPS));
				fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));
	
				cout << "Input video: (" << width << "x" << height << ") at " << fps << ", fourcc = " << fourcc << endl;
			}
			continue;
		}
		mtx.lock();
		inputImgArr[camera_id].push_back(inputImgReal[camera_id]);
		mtx.unlock();
		frameNumReal++;
	}
}

void BirdEyeView()
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
	origPoints.push_back(Point2f(x_lb, y_lb)); //kiri bawah
	origPoints.push_back(Point2f(x_rb, y_rb)); //kanan bawah
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

void TrafficState(int camera_id)
{	
	if (density < 0.5) {
		// change traffic state
		traffic_state = "Lancar";
		// store data
		StoreDensityData(camera_id, traffic_state);
		// output to console
		// cout << "Lancar" << endl << endl;
	} else if ((density >= 0.5) && (speed > 0.5)){
		// change traffic state
		traffic_state = "Ramai Lancar";
		// store data
		StoreDensityData(camera_id, traffic_state);
		// output to console
		// cout << "Ramai Lancar" << endl << endl;
	} else {
		// change traffic state
		traffic_state = "Padat";
		// store data
		StoreDensityData(camera_id, traffic_state);
		// output to console
		// cout << "Padat" << endl << endl;
	}
}

void Run(int camera_id){

	// Main loop
	cout << "run 1, camera " << camera_id << endl;
	for (; ; )
	{
		cout << "run 2, camera " << camera_id << endl;
		if (inputImgReal[camera_id].empty() && frameNum != 0){ break;}
		if (inputImgReal[camera_id].empty()){ continue;}

		if (frameNumReal>1) {
			inputImgReal[camera_id].copyTo(inputImg);
			inputImgArr[camera_id][frameNumReal-2].copyTo(prevImg);
		} else if (frameNumReal <= 1) {
			inputImgReal[camera_id].copyTo(inputImg);
			inputImgReal[camera_id].copyTo(prevImg);
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
		TrafficState(camera_id);

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
		// get request id
		// select date and density_state from database
		// send message date and density state
		HelloReply r;
		for (; ; )
		{
			vector<string> response = GetDensityByID(request->id());
			r.set_timestamp(response[0]);
			r.set_response(response[1]);
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
	vector< map<string, boost::variant<int, string>> > cameras = GetCameras();
	thread tGetFrame[cameras.size()];
	thread tRun[cameras.size()];
	// thread tRunServer[cameras.size()];

	for (int i = 0; i < cameras.size(); ++i){
		cout << "done 1, camera " << boost::get<int>(cameras[i]["camera_id"]) << endl;
		tGetFrame[i] = thread (GetFrame,boost::get<int>(cameras[i]["camera_id"]),boost::get<string>(cameras[i]["url"]));
		cout << "done 2, camera " << boost::get<int>(cameras[i]["camera_id"]) << endl;
		tRun[i] = thread (Run, boost::get<int>(cameras[i]["camera_id"]));
		cout << "done 3, camera " << boost::get<int>(cameras[i]["camera_id"]) << endl;
		// tRunServer[i] = thread (RunServer);
	}
	thread tRunServer (RunServer);
	cout << "done 4" << endl;
	for (int i = 0; i < cameras.size(); ++i){
		tGetFrame[i].join();
		tRun[i].join();
		// tRunServer[i].join();
	}
	tRunServer.join();

	return 0;
}
