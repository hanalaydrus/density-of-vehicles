// Model.h

#ifndef MY_MODEL
#define MY_MODEL

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include "boost/variant.hpp"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
///////////////////////////////////////////////////////////////////////////////////////////////////
class Model {
public:
    // function prototypes ////////////////////////////////////////////////////////////////////////
    Model();
    
    map<string, boost::variant<int, string, map<string, int> > > getCameraConfig(int id);

};

#endif    // MY_MODEL