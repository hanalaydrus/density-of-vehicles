// Model.cpp

#include "Model.h"

Model::Model(){

}

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

map<int, bool> getAllCameraId() {
    map<int,bool> response;
	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;
	  
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://db-density:3306", "root", "root");
		/* Connect to the MySQL test database */
		con->setSchema("density");
	  
		stmt = con->createStatement();
		ostringstream query;
		query << "SELECT `camera_id` FROM `camera`";
		res = stmt->executeQuery(query.str());
		while (res->next()) {
			response[res->getInt("camera_id")] = false;
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

void storeDensityData(int camera_id, string density_state) {
    // Input : camera_id, density_state
	// Output : -
    bool isExist = false;
    int density_history_id;
    string current_density_state;
	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;
	  
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://db-density:3306", "root", "root");
		/* Connect to the MySQL  database */
		con->setSchema("density");
	  
        stmt = con->createStatement();
        ostringstream query;
        query << "SELECT * FROM `density_history` WHERE DATE(`date_time`) = CURDATE() AND `camera_id` = " << camera_id;
		res = stmt->executeQuery(query.str());
        
        while (res->next()) {
          /* Access column data by alias or column name */
            isExist = true;
            density_history_id = res->getInt(1);
            current_density_state = res->getString("density_state");
        }

        if (isExist && (current_density_state != density_state)) {
            stmt = con->createStatement();
            ostringstream query;
            query << "UPDATE `density_history` SET `date_time` = CURRENT_TIMESTAMP, `density_state` = " << density_state << " WHERE `density_history_id` = " << density_history_id;
            stmt->execute(query.str());
        } else if (!isExist) {
            stmt = con->createStatement();
            ostringstream query;
            query << "INSERT INTO `density_history` (`camera_id`, `density_state`) VALUES (" << camera_id << ", " << density_state << ")";
            stmt->execute(query.str());
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
}

string getDensityData(int camera_id) {
    // Input : camera_id
	// Output : density_state
	string response;
	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;
	  
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://db-density:3306", "root", "root");
		/* Connect to the MySQL test database */
		con->setSchema("density");
	  
		stmt = con->createStatement();
		ostringstream query;
		query << "SELECT * FROM `density_history` WHERE `camera_id` = " << camera_id;
		res = stmt->executeQuery(query.str());
		while (res->next()) {
			response = res->getString("density_state");
		}
		delete res;
		delete stmt;
		delete con;
	  
	  } catch (sql::SQLException &e) {
        response = "";
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	  }

	  return response;
}

map<string, boost::variant<int, string, map<string, int> > > Model::getCameraConfig(int id) {
	// Input : -
	// Output : Array of Map
    map<string, boost::variant<int, string, map<string, int> > > camera;
    int mask_points_id;
    string response_string;

    CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	string url = "http://camera-service:50052/camera/";
	url = url + to_string(id);

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		/* example.com is redirected, so we tell libcurl to follow redirection */ 
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
		  fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
	auto j = json::parse(response_string);

	if (j["status"] == "success") {
		string data_detail = j["url"];
        camera["url"] = data_detail;
	}

	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;
	  
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://db-density:3306", "root", "root");
		/* Connect to the MySQL  database */
        // configuration
        con->setSchema("density");
        
        stmt = con->createStatement();
        ostringstream query;
        query << "SELECT * FROM `configuration` WHERE `camera_id` = " << id;
		res = stmt->executeQuery(query.str());
        while (res->next()) {
        /* Access column data by alias or column name */
            camera["real_width"] = res->getInt("real_street_width");
            camera["real_height"] = res->getInt("real_street_height");
            mask_points_id = res->getInt("mask_points_id");
            camera["edge_thresh"] = res->getInt("edge_threshold");
            camera["low_thresh"] = res->getInt("low_threshold");
            camera["max_thresh"] = res->getInt("max_threshold");
            camera["morph_iteration"] = res->getInt("morph_closing_iteration");
        }
        query.str("");
        query << "SELECT * FROM `street_mask_points` WHERE `mask_points_id` = " << mask_points_id;
        res = stmt->executeQuery(query.str());
        while (res->next()) {
        /* Access column data by alias or column name */
            map<string, int> data;
            data["x_lb"] = res->getInt("x_lb");
            data["x_lt"] = res->getInt("x_lt");
            data["x_rb"] = res->getInt("x_rb");
            data["x_rt"] = res->getInt("x_rt");
            data["y_lb"] = res->getInt("y_lb");
            data["y_lt"] = res->getInt("y_lt");
            data["y_rb"] = res->getInt("y_rb");
            data["y_rt"] = res->getInt("y_rt");
            camera["mask_points"] = data;
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
	  
	  return camera;
}