/**
 * 
 */

class UltraFastWindow{
	protected:
		int beg;
		char last;
		char* window=NULL;
		size_t size=0;
		
	public:
		UltraFastWindow(){}
		
		UltraFastWindow(size_t _size) : size(_size){
			window = new char[size];
			last = *window;
			beg=0;
		}
		
		~UltraFastWindow(){
			delete[] window;
		}
		
		char add(char c){
			last = *(window +  beg);
			beg = (beg+1) % size;
			*(window + beg )=c;
			
			return last;
		}
		
		void set(char* data){
			memcpy(window, data, size);
			beg = 0;
			last = *window;
		}	
};


/**
 * A double-linked list faster than std one
 * Warning : not safe if the context is not
 */
class FastListNode{
	public:
		FastListNode* next=NULL;
		FastListNode* previous=NULL;
		char data;
		
		FastListNode(char c) : data(c){}
		
		~FastListNode(){
			if( next != NULL)
				delete next;
		}	
};

class FastList{
	protected:
		FastListNode* first=NULL;
		FastListNode* last=NULL;
	
	public:
		FastList(){}
		
		~FastList(){
			if( first != NULL )
				delete first;
		}
		
		void clear(){
			delete first;
			first = last = NULL;
		}
		
		char front(){
			return first->data;
		}
		
		void pop_front(){
			char c = first->data;
			
			FastListNode* tmp = first->next;
			if( tmp !=NULL){
				first->next->previous = NULL;
				first->next = NULL;
				delete first;
				first = tmp;
			}else{
				first = last = NULL;
			}
		}
		
		void push_back(char c){
			if( last != NULL ){
				last->next = new FastListNode(c);
				last->next->previous = last;
				last = last->next;
			}else{
				first = last = new FastListNode(c);
			}
		}
};
