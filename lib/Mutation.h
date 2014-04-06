#ifndef MUTATION_H_INCLUDED
#define MUTATION_H_INCLUDED

#include <stdint.h>
#include <vector>
#include <fstream>
using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Mutation{
            protected :
                int type;
                uint64_t idBegining;
                uint64_t size;
            public :
                enum type{ INSERT, DELETE, UPDATE };

                Mutation( int t, uint64_t id, uint64_t s) : type(t), idBegining(id), size(s){};

                void applyInsert(vector<bool>& data);
                void applyDelete(vector<bool>& data);
                void applyUpdate(vector<bool>& data);
                void applyInsert(ofstream& data);
                void applyDelete(ofstream& data);
                void applyUpdate(ofstream& data);

                void apply( vector<bool>& data);
                void apply( ofstream& data);

        };
    }
}

#endif // MUTATION_H_INCLUDED
