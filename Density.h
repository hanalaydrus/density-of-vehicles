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

class Density {
public:
	Density();
	static void Density::runDensityService(
		int camera_id,
		string url,
		int real_width,
		int real_height,
		map<string, int> mask_points,
		int edge_thresh,
		int low_thresh,
		int max_thresh,
		int morph_iteration);
};