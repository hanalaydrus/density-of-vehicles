// Model.cpp

#include "Model.h"

Model::Model(){

}


map<string, boost::variant<int, string, map<string, int> > > Model::getCameraConfig(int id) {
	// Input : -
	// Output : Array of Map
    map<string, boost::variant<int, string, map<string, int> > > camera;
    int mask_points_id;
	try {
		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;
	  
		/* Create a connection */
		driver = get_driver_instance();
		con = driver->connect("tcp://127.0.0.1:3306", "root", "root");
		/* Connect to the MySQL  database */
        con->setSchema("camera");
        
        stmt = con->createStatement();
		ostringstream query;
        query << "SELECT * FROM `camera` WHERE `camera_id` = " << id;
		res = stmt->executeQuery(query.str());
		while (res->next()) {
          /* Access column data by alias or column name */
          camera["url"] = res->getString("url");
        }
        // configuration
        con->setSchema("density");
        
        stmt = con->createStatement();
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
            camera["morph_teration"] = res->getInt("morph_closing_iteration");
        }

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