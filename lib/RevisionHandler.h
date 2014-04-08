#ifndef REVISIONHANDLER_H_INCLUDED
#define REVISIONHANDLER_H_INCLUDED

#include <vector>
#include <string>
#include <stdint.h>
#include <cmath>
#include <fstream>

#include "Revision.h"
#include "Mutation.h"
using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class RevisionHandler{
            public :
                RevisionHandler();
                ~RevisionHandler();
                /**  Extraction function
                 *  @param it   -   current position of the cursor of table
                **/
                int getNumber( vector<bool>& table, vector<bool>::iterator& it  );
                uint16_t getOrigine( vector<bool>& table, vector<bool>::iterator& it );
                uint64_t getIdBeginning( vector<bool>& table, vector<bool>::iterator& it );
                uint64_t getSize( vector<bool>& table, vector<bool>::iterator& it );
                uint16_t getDiff( vector<bool>& table, vector<bool>::iterator& it );


                vector<int> extractChildren( vector< int >& origines, int parent );
                void buildChildren( vector<int>& origines, vector< Revision* > revisions, Revision* current);

                /**
                 * Build the structur(ie no data)
                 * @param table     - number(uint16_t) origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(float)...
                **/
                Revision* build( vector<bool>& table );





                void write( vector<bool>& data, unsigned int pos, unsigned length, ofstream& stream);
                void createdMutations( vector<bool>& origine, vector<bool>& data, ofstream& stream, uint64_t pos);
                /**
                 * Calcul the difference(%) between  origin and data, if we keep the result in RAM
                 * @param origin     -
                 * @param data     -
                **/
                float diff( vector<bool>& origine, vector<bool>& data );
                /**
                 * Calcul the difference(%) between  origin and data, if we keep the result on hard disk
                 * @param origin     -
                 * @param data     -
                **/
                float diff( ifstream& origin, ifstream& data );


                Mutation readMutation( ifstream& stream );
                void applyMutations( vector<bool>& data, Revision* rev, ifstream& stream, uint64_t fileSize, uint64_t relativePos);
                void recopy( ifstream& data, ofstream& newStream, uint64_t size);
                void applyMutations( ifstream& data, ofstream& newStream, Revision* rev, ifstream& stream, uint64_t fileSize, uint64_t relativePos);
        };
    }
}

#endif // REVISIONHANDLER_H_INCLUDED
