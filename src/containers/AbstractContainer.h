#ifndef ABSTRACTCONTAINER_H_INCLUDED
#define ABSTRACTCONTAINER_H_INCLUDED

#include <stdint.h>
#include <map>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>

#include <tinyxml2.h>



using namespace std;
namespace tx=tinyxml2;
namespace fs=boost::filesystem;


/**
 * Store a RessourceObject
 *
 */
class AbstractContainer{
	protected:
		uint64_t id;
		uint64_t parent_id;
		fs::path location; ///path where this container will be saved, format /dir1/dir2/current_container
		AbstractContainer* parent = NULL;

		tx::XMLDocument* xml = NULL; /// Describe the content of an abstract container
		tx::XMLElement* root = NULL;

		map< uint64_t, AbstractContainer* > children; ///link childrens ids to children, Used only for DirContainer and TorrentContainer
		map< uint64_t, AbstractContainer* > map_children; ///id of a child container (all container not only direct children)
	public:
		AbstractContainer(){
			setLocation();
			xml = new tx::XMLDocument();
			root = xml->NewElement("AbstractContainer");
			xml->LinkEndChild( root );

		}

		AbstractContainer(const char* name){
			setLocation();
			xml = new tx::XMLDocument();
			root = xml->NewElement("AbstractContainer");
			xml->LinkEndChild( root );

		}


		AbstractContainer(uint64_t i, uint64_t p) : id(i), parent_id(p){
            AbstractContainer();
		}

		virtual ~AbstractContainer(){
			if(parent = NULL)
				delete parent;

			delete xml;

			for(map< uint64_t, AbstractContainer* >::iterator it = children.begin(); it!=children.end(); it++)
				delete it->second;
			for(map< uint64_t, AbstractContainer* >::iterator it = map_children.begin(); it!=map_children.end(); it++)
				delete it->second;
		}


        /// Index functions
        void writeIndex();
        void buildIndex();
        void hydrate();

        ///

		uint64_t getId(){ return id; }
		uint64_t getParent_id(){ return parent_id; }
		fs::path getLocation(){ return location; }

        map< uint64_t, AbstractContainer* >& exposeMap_children(){return map_children; }


		void setId( uint64_t i ){ id=i; }
		void setId( const char* i){(stringstream(i)) >> id;}

		/**
		 * @param dir - parent directory, format /dir1/dir2/dir3
		 */
		void setLocation( string dir="" ){
            stringstream f;
            f<<id;
            location = fs::path(dir+"/"+f.str() );
        }


		/**
		 *
		 */
		void copyTo( string path );


		/**
		 * Build the container architecture on disk
		 * @warning If the container has been already written, the old data will be rewritten.
		 */
		virtual void  build();


};


#endif // ABSTRACTCONTAINER_H_INCLUDED
