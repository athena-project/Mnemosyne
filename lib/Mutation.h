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

                void setIdBeginning( uint64_t i ){ idBeginning = i; }
                void setSize( uint64_t s ){ size = s; }

                /**
                 * @brief Apply insertion to data in using the stream as a source
                 * @param data          - current data
                 * @param stream        - data use as reference
                 */
                void applyInsert(vector<char>& data, ifstream& stream);

                /**
                 * @brief Delete a part of the vect
                 * @param data          - current data
                 */
                void applyDelete(vector<char>& data);

                /**
                 * @brief Update the vector in using the stream as a source
                 * @param data          - current data
                 * @param stream        - data use as reference
                 */
                void applyUpdate(vector<char>& data, ifstream& stream);

                /**
                 * @brief Apply the current mutation at the data from the stream
                 * @param data          - current data
                 * @param stream        - data use as reference
                 */
                void apply( vector<char>& data, ifstream& stream);

        };
    }
}

#endif // MUTATION_H_INCLUDED
