#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>
#include "../incremental/IncrementalFile.h"
#include "../incremental/Revision.h"


BOOST_AUTO_TEST_CASE( my_test ){
	/// INIT
	WorkingRevision wr1( "data/incremental_1");
	WorkingRevision wr2( "data/incremental_2");
	WorkingRevision wr3( "data/incremental_3");
	WorkingRevision wr4( "data/incremental_4");
	WorkingRevision wr5( "data/incremental_5");
	WorkingRevision wr6( "data/incremental_6");
	
	IncrementalFile incrFile( "" );
	

	incrFile.newRevision( wr1 );
	incrFile.newRevision( wr2 );
	incrFile.newRevision( wr3 );
	incrFile.newRevision( wr4 );
	incrFile.newRevision( wr5 );
	incrFile.newRevision( wr6 );
	
	incrFile.save();
		
	///BASIC TESTS
	incrFile=IncrementalFile( "" );
	incrFile.init();
	
	BOOST_REQUIRE( incrFile.get(1) == wr1 );
	BOOST_REQUIRE( incrFile.get(2) == wr2 );
	BOOST_REQUIRE( incrFile.get(3) == wr3 );
	BOOST_REQUIRE( incrFile.get(4) == wr4 );
	BOOST_REQUIRE( incrFile.get(5) == wr5 );
	BOOST_REQUIRE( incrFile.get(6) == wr6 );
	
	incrFile=IncrementalFile( "" );
	incrFile.lazyInit();
	
	BOOST_REQUIRE( incrFile.get(1) == wr1 );
	BOOST_REQUIRE( incrFile.get(2) == wr2 );
	BOOST_REQUIRE( incrFile.get(3) == wr3 );
	BOOST_REQUIRE( incrFile.get(4) == wr4 );
	BOOST_REQUIRE( incrFile.get(5) == wr5 );
	BOOST_REQUIRE( incrFile.get(6) == wr6 );
	
	///RECURSION DEPTH TESTS
	incrFile=IncrementalFile( "" );
	incrFile.init();

	fstream descriptor = incrFile.getHandler()->getDescriptor();
	descriptor.seekg(0, descriptor.end);
	int last = descriptor.tellg();
	for(int i=0; i<100; i++){
		incrFile.newRevision( wr1 );
		incrFile.newRevision( wr2 );
		incrFile.newRevision( wr3 );
		incrFile.newRevision( wr4 );
		incrFile.newRevision( wr5 );
		incrFile.newRevision( wr6 );
		
		///Check descriptor size
		descriptor.seekg(0, descriptor.end);
		BOOST_REQUIRE( descriptor.tellg() == last + 6*REVISION_SIZE_TABLE);
		last = descriptor.tellg();
	}
	incrFile.save();
	
	incrFile=IncrementalFile( "" );
	incrFile.init();
	for(int i=1; i<101; i++){
		BOOST_REQUIRE( incrFile.get(i*6+1) == wr1 );
		BOOST_REQUIRE( incrFile.get(i*6+2) == wr2 );
		BOOST_REQUIRE( incrFile.get(i*6+3) == wr3 );
		BOOST_REQUIRE( incrFile.get(i*6+4) == wr4 );
		BOOST_REQUIRE( incrFile.get(i*6+5) == wr5 );
		BOOST_REQUIRE( incrFile.get(i*6+6) == wr6 );
	}
	
    // seven ways to detect and report the same error:
    // BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error

    // BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error

    // if( add( 2,2 ) != 4 )
      //BOOST_ERROR( "Ouch..." );            // #3 continues on error

    // if( add( 2,2 ) != 4 )
      // BOOST_FAIL( "Ouch..." );             // #4 throws on error

    // if( add( 2,2 ) != 4 ) throw "Ouch..."; // #5 throws on error

    // BOOST_CHECK_MESSAGE( add( 2,2 ) == 4,  // #6 continues on error
                         //"add(..) result: " << add( 2,2 ) );

    // BOOST_CHECK_EQUAL( add( 2,2 ), 4 );	  // #7 continues on error
}
