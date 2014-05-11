#include "SQLFactory.h"

namespace Athena{
    namespace Mnemosyne{

        mysqlpp::Connection getSQLConnexion(){
            const char* host="localhost";
            const char* login="root";
            const char* password="root";
            const char* dbname="mnemosyne";

            mysqlpp::Connection conn(false);
            if(conn.connect(dbname, host, login, password))
                return conn;
            else{
                cerr << "DB connection failed: " << conn.error() << endl;
                throw "";
            }
        }
    }
}
