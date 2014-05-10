#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED

#include <stdint.h>

using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Block{
            protected :
                uint64_t id;
            public :
                const uint32_t CHUNKS = 2000; //2000 chuncks by block

                Block();
                Block( uint64_t id){ this->id = id; }

                uint64_t getId(){ return id; }
                void setId(uint64_t i){ id=i; }
        };
    }
}

#endif // BLOCK_H_INCLUDED
