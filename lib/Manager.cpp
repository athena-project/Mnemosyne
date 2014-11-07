#include "Manager.h"

Manager::Manager(){
	conn = SQLFactory::getSQLConnexion();
}

Manager::~Manager(){
    conn.disconnect();
    delete &conn;
}
