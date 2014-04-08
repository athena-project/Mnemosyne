#ifndef REVISION_CPP_INCLUDED
#define REVISION_CPP_INCLUDED

#include <vector>
#include <string>
#include <stdint.h>

using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Revision{


            protected :
                int n;                  //N° de la révision, -1 si représente la racine
                uint64_t idBeginning;   //Id du premier caractère de la révision
                uint64_t size;          //Taille de la révision
                uint32_t diff;          //E[%*10000] de différence avec l'origine(abstract)

                Revision* parent;
                vector< Revision* > children;
                Revision* previous;     //révision n-1
                Revision* next;         //révision n+1
                Revision* last;         //Dernière révision ajoutée : nmax

            public :
                const int REVISION_SIZE = 160; // origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(uint16_t)

                Revision();
                Revision(int num) : n(num){};
                Revision(int num, uint64_t id, uint64_t s, uint32_t d) : n(num), idBeginning(id), size(s), diff(d){};
                ~Revision();

                int getN(){ return n; }
                uint64_t getIdBeginning(){ return idBeginning; }
                uint64_t getSize(){ return size; }
                uint32_t getDiff(){ return diff; }

                void setParent( Revision* rev ){ parent=rev; }
                void setPrevious( Revision* rev ){ previous=rev; }
                void setNext( Revision* rev ){ next=rev; }
                void setLast( Revision* rev ){ last=rev; }

                void addChild( Revision* child ){ children.push_back(child); }

                /*
                 * Build the current revision(data) in the argument, if we keep the result in RAM
                 * @param data     -
                 */
                void hydrate( vector<bool>& data );
                /*
                 * Build the current revision(data) in the argument, if we keep the result on hard disk
                 * @param stream     -
                 */
                void hydrate( ofstream& stream );

        };

    }
}

#endif // REVISION_CPP_INCLUDED
