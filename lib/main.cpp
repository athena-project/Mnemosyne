#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>

#include <boost/filesystem.hpp>

#include "Ressource.h"
#include "Revision.h"
using namespace std;
using namespace Athena::Mnemosyne;



int main(){
    string str;
    ifstream s("/home/toor/Desktop/1.html.origin",ios::in);
    char c;
    while( s.get(c) )
        str += c;

    Ressource r;
    RessourceHandler rHandler;
    RevisionHandler revHandler;
//
//        r.getRevision()->setIStream( "/home/toor/Desktop/1.stream");
//        r.getRevision()->setOStream( "/home/toor/Desktop/1.stream");
    vector<uint64_t> ids;
    ids.push_back(24);
    r.setChunkIds( ids );


///Creation ressource

//    rHandler.newRevision(&r, str);


///lecture

//
    ofstream test("/home/toor/Desktop/2.html");
    string a=rHandler.buildRevision(r, 1);
    test<<a;

    return 0;
}
