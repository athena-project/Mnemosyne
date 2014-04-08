#include <iostream>
#include <fstream>
#include <bitset>
#include <stdint.h>
#include <vector>

#include "RevisionHandler.h"
using namespace std;
using namespace Athena::Mnemosyne;
template< size_t N>
vector<bool> b2v( bitset< N > b ){
    vector<bool> vect;
    for(int i=0; i<N; i++)
        vect.push_back( b[i] );
    return vect;
}

int main(){
    uint16_t i1=2;
    bitset< 16 >b1(i1);

    uint16_t i2=2;
    bitset< 16 >b2(i2);
    vector<bool> s1 = b2v( b1 );
    vector<bool> s2 = b2v( b2 );

    vector<bool> t;
    s2.push_back(true);
   RevisionHandler* handler= new RevisionHandler();
    ofstream stream("/home/severus/test1", ios::app);
    handler->createdMutations(s1 , s2, stream, 0);
//    ifstream s1( "/home/severus/s1", ios::binary);
//    ifstream s2 ("/home/severus/s2", ios::binary);
//    cout<<handler->diff(s1,s2)<<endl;
   delete handler;

    return 0;
}
