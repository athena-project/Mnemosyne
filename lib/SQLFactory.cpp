#include "SQLFactory.h"

mysqlpp::Connection getSQLConn(){
    const char* host="localhost";
	const char* login="root";
	const char* password="rj7@kAv;8d7_e(E6:m4-w&";
	const char* dbname="mnemosyne";

	mysqlpp::Connection conn(false);

	if(conn.connect(dbname, host, login, password))
		return conn;
	else{
		cerr << "DB connection failed: " << conn.error() << endl;
		throw "";
	}
}
