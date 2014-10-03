#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>

#include <boost/filesystem.hpp>

#include "Ressource.h"
#include "Revision.h"
using namespace std;



int main(){
    string str;
    ifstream s("/home/toor/Desktop/1.html.origin",ios::in);
    char c;
    while( s.get(c) )
        str += c;

    Ressource r;
    RessourceHandler rHandler;


    vector<uint64_t> ids;
    //ids.push_back(69);
    r.setChunkIds( ids );


///Creation ressource
    rHandler.newRevision(&r, str);


///lecture


//    r.setCurrentRevision(3);
//    ofstream test("/home/toor/Desktop/2.html");
//    string a=rHandler.buildRevision(r, 1);
//    test<<a;
    return 0;
}
