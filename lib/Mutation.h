#ifndef MUTATION_H_INCLUDED
#define MUTATION_H_INCLUDED

#include <stdint.h>
#include <vector>
#include <fstream>
#include <bitset>
using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Mutation{
            protected :
                uint8_t type;
                uint64_t idBeginning;
                uint64_t size;
            public :
                static const uint8_t INSERT=0b00;
                static const uint8_t DELETE=0b01;
                static const uint8_t UPDATE=0b10;

                Mutation( uint8_t t, uint64_t id, uint64_t s) : type(t), idBeginning(id), size(s){};

                uint64_t getIdBeginning(){ return idBeginning; }
                uint64_t getSize(){ return size; }

                void applyInsert(vector<bool>& data, ifstream& stream);
                void applyDelete(vector<bool>& data);
                void applyUpdate(vector<bool>& data, ifstream& stream);
                void applyInsert(ofstream& newStream, ifstream& stream);
                void applyDelete(ifstream& data);
                void applyUpdate(ofstream& newStream, ifstream& data, ifstream& stream);

                void apply( vector<bool>& data, ifstream& stream);
                void apply( ofstream& newStream, ifstream& data, ifstream& stream);

        };
    }
}

#endif // MUTATION_H_INCLUDED
