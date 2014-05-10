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

                Mutation(){}
                Mutation( uint8_t t, uint64_t id, uint64_t s) : type(t), idBeginning(id), size(s){}

                uint64_t getIdBeginning(){ return idBeginning; }
                uint64_t getSize(){ return size; }

                /**
                 * Binary vect functions
                **/
                void applyInsert(vector<char>& data, ifstream& stream);
                void applyDelete(vector<char>& data);
                void applyUpdate(vector<char>& data, ifstream& stream);


                void apply( vector<char>& data, ifstream& stream);

        };
    }
}

#endif // MUTATION_H_INCLUDED
