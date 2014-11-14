#include "Manager.h"

Manager::Manager(){
	const char* host="localhost";
	const char* login="root";
	const char* password="rj7@kAv;8d7_e(E6:m4-w&";
	const char* dbname="mnemosyne";

	conn =mysqlpp::Connection(false);
	if(!conn.connect(dbname, host, login, password)){
		cerr << "DB connection failed: " << conn.error() << endl;
		throw "";
	}
}

Manager::~Manager(){
    conn.disconnect();
}
