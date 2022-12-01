#include "BFHT.h"


#include <cmath>
#include <cstdlib>
//#include <cstring>
#include <iostream>

#include "hash.h"

using namespace std;

#define HASH(key) (h(&key, sizeof(Key_t), hash_seed))
#define IDX(hash, capacity) (hash % capacity)


/*
Function: generate_seeds() 
        Generate one randomized seed for hash function
*/
void BFHT::generate_seeds(void)
{
    srand(time(NULL));
	
    hash_seed = rand();
    hash_seed = hash_seed << (rand() % 63);
}

extern "C" hash_api *create_hashtable(const hashtable_options_t &opt, unsigned sz, unsigned tnum)
{
  if (sz)
    sz = log2(sz);
  else
    sz = 23;
  std::cout << "size is " << sz<<std::endl;
  return new BFHT(sz >= 2 ? sz : 2);
}

BFHT::BFHT(void) {}

/*
	Function:BFHT();
		Initialize a hash table
*/
BFHT::BFHT(size_t _original_table_size)
		:original_table_size{_original_table_size},
		init_capacity{(uint64_t)pow(2,_original_table_size)}
{

		std::cout << "init_capacity is " << init_capacity << std::endl;
		locksize = 256;
		nlocks = init_capacity/locksize + 1;
		mutex = new std::shared_mutex[nlocks];
		
		generate_seeds();

		buckets = new Node[init_capacity];
	

		//for bloom filter
		  // How many elements roughly do we expect to insert?
  		parameters.projected_element_count = 200000000;

   // Maximum tolerable false positive probability? (0,1)
   		parameters.false_positive_probability = 0.0001; // 1 in 10000

   // Simple randomizer (optional)
   		parameters.random_seed = 0xA5A5A5A5;
		if (!parameters)
		{
			std::cout << "Error - Invalid set of bloom filter parameters!" << std::endl;
		}
		parameters.compute_optimal_parameters();

		filter = new bloom_filter(parameters);
		if(filter == NULL){
			std::cout<<"new bloom filter error..."<<std::endl;
		}

}

BFHT::~BFHT(void)
{
	delete[] mutex;
	vmem_free(vmp, buckets);

	deletePM();
	//for bloom filter
	delete filter;
}
		
/*
	Function:BFHT_insert();
		Insert a key-value entry into wisdom hash table;
	Returns:
		true represents succeed;
		false represents failed.
*/
bool BFHT::Insert(Key_t &key, Value_t value){
	uint64_t f_hash = HASH(key);
	uint64_t f_idx = IDX(f_hash, init_capacity);
	
	uint64_t i;
	Node* tb = &buckets[f_idx];//tb means tmp bucket pointer
	//1. judge whether the key is existed?
//	if(filter->contains(key)){ 
		while(tb != NULL)
		{
			for(i = 0; i < ASSOC_NUM; i++ )
			{
				std::unique_lock<std::shared_mutex> lock(mutex[f_idx / locksize]);
				if(tb->token[i] == 1 && tb->slot[i].key == key) {
					return false;
				}
			}
			if(tb->next != NULL)
				tb = tb->next;
			else
				break;
		}
//	}
	// 2. Head insert the key-value pair into BFHT if the pair is not existed
	tb = &buckets[f_idx];
	std::unique_lock<std::shared_mutex> lock(mutex[f_idx / locksize]);
	Node *next=NULL;
	for(i = 0; i < ASSOC_NUM; i++ )
	{
		if(tb->token[i] == 0) {
			tb->slot[i].value = value;
			mfence();
			tb->slot[i].key = key;
			tb->token[i] = 1;
			clflush((char *)&tb, sizeof(Node));

			return true;
		}
	}
	next = tb->next;
	if(next != NULL){
		for(i = 0; i < ASSOC_NUM; i++ )
		{
			if(next->token[i] == 0) {
				next->slot[i].value = value;
				mfence();
				next->slot[i].key = key;
				next->token[i] = 1;
				clflush((char *)&next, sizeof(Node));
				return true;
			}
		}		
	}
	//3. Allocate Node from persistent memory space when head bucket chains has no slot space for new Key-Value pair
	Node *tmp = new Node;

	if(tmp == NULL) {
		printf("new tmp bucket node failed!\n");
		return false;
	}

	tmp->slot[0].value = value;
	mfence();
	tmp->slot[0].key = key;
	tmp->token[0] = 1;
	tmp->next = next;
	clflush((char *)&tmp, sizeof(Node));
	mfence();
	tb->next = tmp;
	clflush((char *)&tb, sizeof(Node));
	mfence();

	//for bloom filter
	filter->insert(key);
	return true;	
	
}

bool BFHT::InsertOnly(Key_t &, Value_t) { return true; }
/*
	Function:BFHT_query();
		Query a key-value item from hash table
*/
Value_t BFHT::Get(Key_t &key){
	if(filter->contains(key)){
		uint64_t f_hash = HASH(key);
		uint64_t f_idx = IDX(f_hash, init_capacity);

		uint64_t i;
		Node * tb = &buckets[f_idx];
		while(tb != NULL)
		{
			std::shared_lock<std::shared_mutex> lock(mutex[f_idx / locksize]);
			for(i = 0; i < ASSOC_NUM; i++ )
			{
				if(tb->token[i] == 1 && tb->slot[i].key == key ) {
					return tb->slot[i].value;
				}
			}
			tb = tb->next;
		}
	}
	return NONE;
}

/*
	Function:BFHT_delete()
		Delete a key-value entry from wisdom hash table
	Returns:
		0 represents succeed!
		1 represents failed!	
*/
bool BFHT::Delete(Key_t &key){
	if(filter->contains(key)){
		uint64_t f_hash = HASH(key);
		uint64_t f_idx = IDX(f_hash, init_capacity);

		uint64_t i;
		Node * tb = &buckets[f_idx];
		while(tb != NULL)
		{
			for(i = 0; i < ASSOC_NUM; i++ )
			{
				std::unique_lock<std::shared_mutex> lock(mutex[f_idx / locksize]);
				if(tb->token[i] == 1 && tb->slot[i].key == key ) {
					tb->token[i] = 0;
					clflush((char *)&tb->token[i], sizeof(uint8_t));
					return true;
				}
			}
			tb = tb->next;
		}
	}
	return false;	
}

/*
	Function:BFHT_recovery();
		Recovery a BFHT when system failure occurs!
	Returns:
		0 represents succeed!
		1 represents failed!
*/
bool BFHT::Recovery(){
	uint64_t count = 0;
	uint64_t i,j;
	Node * tb;//bucket pointer
	for (j=0; j < init_capacity; j++) {
		tb = &buckets[j];// j means head bucket number 
		while(tb != NULL)
		{
			std::unique_lock<std::shared_mutex> lock(mutex[j / locksize]);	
			for(i = 0; i < ASSOC_NUM; i++ )
			{
                if(tb->token[i] == 1){
                    filter->insert(tb->slot[i].key);
                }
			}
//			clflush((char*)&tb, sizeof(Node));
			tb = tb->next;
		}
	}

	return true;
}

/*
	Function:BFHT_size();
		
	Returns:
		the number of valid KV items in the hash table
*/
#if 1
size_t BFHT::Size(){
	size_t count = 0;
	uint64_t i,j;
	Node * tb;
	for (j=0; j < init_capacity; j++) {
		tb = &buckets[j];
		while(tb != NULL)
		{
			for(i = 0; i < ASSOC_NUM; ++i)
			{
				if(tb->token[i] != 0 )
					count++;
			}
			tb = tb->next;
		}
	}
	return count;
}
#endif

/*
	Function:BFHT_utilization();
		
	Returns:
		the load factor and memory utilization of the hash table
*/
hash_Utilization BFHT::Utilization(void)
{
	size_t count = 0;
	uint64_t i,j;
	Node * tb;
	size_t total_bucket_num = 0;
	for (j=0; j < init_capacity; ++j) {
		tb = &buckets[j];
		while(tb != NULL)
		{
			++total_bucket_num;
			for(i = 0; i < ASSOC_NUM; ++i)
			{
				if(tb->token[i] != 0 )
					count++;
			}
			tb = tb->next;
		}
	}
	hash_Utilization h;
	if(init_capacity == 0){std::cout<<"init_capacity is "<<init_capacity<<std::endl;}
	h.load_factor = ((float)(count) / (float)(total_bucket_num * ASSOC_NUM) * 100);
	h.utilization = ((float)(count * 16) / (float)(total_bucket_num * sizeof(Node)) * 100);
	return h;
}

