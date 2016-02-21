#define BOOST_TEST_MODULE MyTest
#define  BOOST_TEST_DYN_LINK MyTest
#include <boost/test/unit_test.hpp>
#include "hrw.cpp"

BOOST_AUTO_TEST_SUITE( Foo)

BOOST_AUTO_TEST_CASE( hrw_test ){
   	NodeMap m(2);
	Node n1(12);
	Node n2(126);
	m.add_node(&n1);
	m.add_node(&n2);

	vector<Node*> t = m.wallocate(57);
	BOOST_CHECK( t[0] == m.rallocate(57)); 

	BOOST_CHECK( t[0] != NULL );  
	BOOST_CHECK( t[1] != NULL );  
	BOOST_CHECK( t[0] != t[1] );  


    n1.set_state( LOST );
	n2.set_state( FULL );
	t = m.wallocate(57);
	
	BOOST_CHECK( t[0] == NULL );  
	BOOST_CHECK( t[1] == NULL );  

	BOOST_CHECK( n2.get_id() == m.rallocate(57)->get_id() ); 

    n1.set_state( ACTIVE );
	n2.set_state( ACTIVE );
}

BOOST_AUTO_TEST_SUITE_END()


