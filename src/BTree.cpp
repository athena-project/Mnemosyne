#include <stdint.h>
#include <vector>
#include <cstddef>
#include <map>

using namespace std;

///some kind of B+ Tree, pas de recherche d'element à la fin
template<typename TKey, typename TItem>
class BNode{
	protected:
		unsigned int d = 0;
		TKey key; ///minkey of children, never change
		BNode& parent;
		BNode& sibling = NULL;
		bool leaf = false;
		
		vector<TItem&> items;
		vector<BNode&> children; ///sorted by key
	
	public:		
		BNode(unsigned int _d) : d(_d){}
		BNode(unsigned int _d, vector<BNode&> _children, BNode& _parent, BNode& _sibling){
			d = _d;
			children = _children;
			parent = _parent;
			sibling = _sibling;
			
			if( children.size() > 0)
				key = children[0].get_key();
		}
		BNode(unsigned int _d, vector<TItem> _items, BNode& _parent, BNode& _sibling){
			d = _d;
			items = _items;
			parent = _parent;
			sibling = _sibling;
			
			if( items.size() > 0)
				key = items[0].get_key();
		}
		
		TKey get_key(){ return key; }
		
		BNode* get_parent(){ return parent; }
		
		bool operator < (const BNode& o) const{
			return (key < o.key);
		}
		
		vector<TItem&> get_items(){
			return items;
		}
		
		uint64_t size(){ return leaf ? items.size() : children.size(); }
		
		bool full(){ return size() >= 2*d; }
		bool half_full(){  return size() == d; }
		bool empty(){ return size() < d; }
		
		BNode& search(TKey n_key){
			if( leaf )
				return this;
			
			for(int k=0; k<children.size(); k++){
				if( n_key < children[k].get_key() )
					return children[k].search( n_key );
			}
			
			return children.last()->search( n_key);
		}
		
		void add_item(TKey n_key, TItem& item){
			items.push_back( item );
			sort(items.begin(), items.end());
		}
		
		
		void remove_item(TKey r_key){
			uint64_t a = 0;
			uint64_t b = items.size();
			uint64_t m = (a+b) / 2;
			while( a<b ){
				m = (a+b) / 2;
				if( items[m]< r_key ){ a = m; }else{ b = m; }
			}
			
			items.erase( items.beg() + a );
		}
		
		void remove_child(TKey r_key){
			uint64_t a = 0;
			uint64_t b = children.size();
			uint64_t m = (a+b) / 2;
			while( a<b ){
				m = (a+b) / 2;
				if( children[m]< r_key ){ a = m; }else{ b = m; }
			}
			
			children.erase( children.beg() + a );
		}
		
		void add_child(BNode& child){
			children.push_back( child );
			sort(children.begin(), children.end());
		}
		
		BNode split(){
			///on suppose trier la liste des enfants
			if( leaf ){
				typename vector<TItem&>::const_iterator mid = items.begin() + d;
				vector<TItem&> left_items(items.begin(), mid++);
				items = vector<TItem>(mid, left_items);

				return ( sibling = BNode( mid , items.end(), parent));
			}else{
				typename vector<BNode&>::const_iterator mid = children.begin() + d;
				vector<BNode&> left_children(children.begin(), mid++);
				children = vector<BNode&>( mid, left_children);
			
				return ( sibling = BNode(mid, children.end(), parent));
			}
		}
		
		BNode& insert(TKey n_key, TItem& item){///reference ou pas ?
			if( leaf )
				return this;
			
			for(int k=0; k<children.size(); k++){
				if( n_key < children[k].get_key() )
					return children[k].search( n_key );
			}
			
			return children.last()->search( n_key);
		}
};

template<typename TKey, typename TItem>
class BTree{
	protected:
		BNode<TKey, TItem>& root;
		uint64_t d = 0;
	
	public:
		BTree(unsigned int _d) : d(_d){
			root = BNode<TKey, TItem>(d);
		}
		
		~BTree(){
			delete root;
		}
		
		vector<TItem&> search(TKey n_key){
			BNode<TKey, TItem>& node = root.search( n_key );
			
			return node->get_items();
		}
	
		void insert(TKey n_key, TItem& item){///reference ou pas ?
			BNode<TKey, TItem>& node = root.search( n_key );
			node.add_item( item );

			while( node.full() ){	
				BNode<TKey, TItem>& n_node = node.split();
				BNode<TKey, TItem>& parent = node.get_parent();
				if( *node == *root ){ ///root split
					BNode<TKey, TItem>& n_root(d);
					n_root.add_child( root );
					n_root.add_child( n_node );
					root = n_root;
				}else
					parent.add_child( n_node );
					
				node = parent;
			}
			

		}
		
		void remove(TKey r_key){
			BNode<TKey, TItem>& node = root.search( r_key );
			node.remove_item( r_key );
			
			while( node.empty() && *node != *root ){	
				if( node.get_sibling().half_full() ){
					node.fusion();
					BNode<TKey, TItem>& parent = node.get_parent();
				}else{
					node.fusion();
				}
				
				
			}
		}
};

int main(){
	
}

