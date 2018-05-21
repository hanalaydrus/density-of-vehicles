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

#include <grpc++/grpc++.h>
#include <chrono>
#include <cstdlib>
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

#include "Model.h"
#include "Density.h"

using namespace std::chrono;
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

///////////////////////////////////////////////////////////////////

static const char alphanum[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int stringLength = sizeof(alphanum) - 1;

char genRandom()
{

    return alphanum[rand() % stringLength];
}

int conccurrent = 0;

string printTime(){
	milliseconds ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    return to_string(ms.count());
}

///////////////////////////////////////////////////////////////////

class GreeterServiceImpl final : public Greeter::Service {
	Status SayHello(ServerContext* context,
					const HelloRequest* request,
					ServerWriter<HelloReply>* writer) override {

		////////// Logging /////////////////////////////////////
        vector< vector<boost::variant<int, string>> > log;

        int camera_id = request->id() % 10;
        string Str = to_string(request->id());

        conccurrent++;

        ////////////////////////////////////////////////////////

		HelloReply r;
		Model model;
		string response;

        cout << "start process" << endl;

		for (int i = 0; i < 1000; ++i) {
			///////////// Logging ///////////////////
            vector<boost::variant<int, string>> logs;
            /////////////////////////////////////////
			response = model.getDensityData(camera_id);
			r.set_response(response);
			writer->Write(r);
            /////////////////////////////////////////
			logs.push_back(Str);
            logs.push_back(printTime());
            logs.push_back(conccurrent);

            log.push_back(logs);
			
			if (context->IsCancelled()){
				break;
			}
		}
		model.logging(log);
        cout << "Finish check log cc: " << conccurrent << endl;
        conccurrent--;
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
	Model model;
	vector< map<string, boost::variant<int, string, map<string, int>>>> cameras;
	cameras = model.getCameras();

	thread tRunService[cameras.size()];

	for (int i = 0; i < cameras.size(); ++i){
		tRunService[i] = thread ( 
			Density::runDensityService,
			boost::get<int>(cameras[i]["camera_id"]),
			boost::get<string>(cameras[i]["url"]),
			boost::get<int>(cameras[i]["real_width"]),
			boost::get<int>(cameras[i]["real_height"]),
			boost::get< map<string, int> >(cameras[i]["mask_points"]),
			boost::get<int>(cameras[i]["edge_thresh"]),
			boost::get<int>(cameras[i]["low_thresh"]),
			boost::get<int>(cameras[i]["max_thresh"]),
			boost::get<int>(cameras[i]["morph_iteration"])
		);
	}

	thread tRunServer(RunServer);

	for (int i = 0; i < cameras.size(); ++i){
		tRunService[i].join();
    }

	tRunServer.join();

	// map<string, int> data;
	// data["x_lb"] = 365;
	// data["x_lt"] = 300;
	// data["x_rb"] = 545;
	// data["x_rt"] = 703;
	// data["y_lb"] = 295;
	// data["y_lt"] = 540;
	// data["y_rb"] = 275;
	// data["y_rt"] = 490;

	// RunService("http://localhost:5002/video_feed", 8, 16, data, 1, 0, 100, 7);

	return 0;
}
