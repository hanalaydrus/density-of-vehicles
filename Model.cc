// Model.cpp

#include "Model.h"

Model::Model(){

}

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

vector< map<string, boost::variant<int, string, map<string, int> > > > Model::getCameras() {
	// Input : -
	// Output : Array of Map
    vector< map<string, boost::variant<int, string, map<string, int> > > > cameras;
    string response_string;

    CURL *curl;
	CURLcode res;

	curl = curl_easy_init();

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://camera-service:50052/camera");
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
		for (int i = 0; i < 6; i++) {
			map<string, boost::variant<int, string, map<string, int> > > data;

			string data_detail_url = j["data"][i]["url"];
            data["url"] = data_detail_url;

            int data_detail_camera_id = j["data"][i]["camera_id"];
            data["camera_id"] = data_detail_camera_id;

            cameras.push_back(data);
		}
	}

	while(true){
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
			res = stmt->executeQuery("SELECT * FROM `configuration`");
			int i = 0;
	        while (res->next()) {
	        /* Access column data by alias or column name */
	        	map<string, boost::variant<int, string, map<string, int> > > data;
	        	data = cameras[i];

	            data["real_width"] = res->getInt("real_street_width");
	            data["real_height"] = res->getInt("real_street_height");
	            data["edge_thresh"] = res->getInt("edge_threshold");
	            data["low_thresh"] = res->getInt("low_threshold");
	            data["max_thresh"] = res->getInt("max_threshold");
	            data["morph_iteration"] = res->getInt("morph_closing_iteration");

	            cameras.at(i) = data;
	            i++;
	        }
	        // asumsi config id selalu sama dengan mask point id
	        res = stmt->executeQuery("SELECT * FROM `street_mask_points`");
	        i = 0;
	        while (res->next()) {
	        /* Access column data by alias or column name */
	        	map<string, boost::variant<int, string, map<string, int> > > data;
	        	data = cameras[i];
	            map<string, int> data_mask_point;

	            data_mask_point["x_lb"] = res->getInt("x_lb");
	            data_mask_point["x_lt"] = res->getInt("x_lt");
	            data_mask_point["x_rb"] = res->getInt("x_rb");
	            data_mask_point["x_rt"] = res->getInt("x_rt");
	            data_mask_point["y_lb"] = res->getInt("y_lb");
	            data_mask_point["y_lt"] = res->getInt("y_lt");
	            data_mask_point["y_rb"] = res->getInt("y_rb");
	            data_mask_point["y_rt"] = res->getInt("y_rt");
	            data["mask_points"] = data_mask_point;

	            cameras.at(i) = data;
	            i++;
	        }

			delete res;
			delete stmt;
			delete con;
			break;
		  
		  } catch (sql::SQLException &e) {
			cout << "# ERR: SQLException in " << __FILE__;
			cout << "# ERR: " << e.what();
			cout << " (MySQL error code: " << e.getErrorCode();
			cout << ", SQLState: " << e.getSQLState() << " )" << endl;
		  }
	}
	  
	  return cameras;
}

void Model::storeDensityData(int camera_id, string density_state) {
    // Input : camera_id, density_state
	// Output : -
    bool isExist = false;
    int density_history_id;
    string current_density_state;
    while (true){
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
	            query << "UPDATE `density_history` SET `date_time` = CURRENT_TIMESTAMP, `density_state` = '" << density_state << "' WHERE `density_history_id` = " << density_history_id;
	            stmt->execute(query.str());
	        } else if (!isExist) {
	            stmt = con->createStatement();
	            ostringstream query;
	            query << "INSERT INTO `density_history` (`camera_id`, `density_state`) VALUES (" << camera_id << ", '" << density_state << "')";
	            stmt->execute(query.str());
	        }

			delete res;
			delete stmt;
			delete con;
		  	break;

		} catch (sql::SQLException &e) {
			cout << "# ERR: SQLException in " << __FILE__;
			cout << "# ERR: " << e.what();
			cout << " (MySQL error code: " << e.getErrorCode();
			cout << ", SQLState: " << e.getSQLState() << " )" << endl;
		}
	}
}

string Model::getDensityData(int camera_id) {
    // Input : camera_id
	// Output : density_state
	string response;

	while(true){
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
		  	break;

		} catch (sql::SQLException &e) {
	        response = "";
			cout << "# ERR: SQLException in " << __FILE__;
			cout << "# ERR: " << e.what();
			cout << " (MySQL error code: " << e.getErrorCode();
			cout << ", SQLState: " << e.getSQLState() << " )" << endl;
		}
	}

	 return response;
}