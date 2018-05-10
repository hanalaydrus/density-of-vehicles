// Model.h

#ifndef MY_MODEL
#define MY_MODEL

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <boost/variant.hpp>
#include <curl/curl.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;
///////////////////////////////////////////////////////////////////////////////////////////////////
class Model {
public:
    // function prototypes ////////////////////////////////////////////////////////////////////////
    Model();

    vector< map<string, boost::variant<int, string, map<string, int>>> > getCameras();
    void storeDensityData(int camera_id, string density_state);
    string getDensityData(int camera_id);
};

#endif    // MY_MODEL
