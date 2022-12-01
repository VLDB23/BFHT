#ifndef BFHT_H_
#define BFHT_H_

#include <hash_api.h>
//#include <stdlib.h>
#include <stdint.h>
#include <vmem.h>

#include <cstring>
#include <mutex>
#include <shared_mutex>
//#include <stdio.h>
//#include <string.h>
//#include <time.h>
//#include <ctype.h>
//#include <math.h>
//#include "hash.h"
//#include "log.h"

#include "pair.h"
#include "persist.h"
#define ASSOC_NUM 3                       // The number of slots in a bucket

//for bloom filter
#include "bloom_filter.hpp"

struct Entry{                     // A slot storing a key-value item 
    Key_t key;                 // KEY_LEN and VALUE_LEN are defined in log.h
    Value_t value;
	Entry()
	{
		key = INVALID;
		value = NONE;
	}
	void *operator new[](size_t size)
	{
		return vmem_aligned_alloc(vmp, 64, size);
	}	

	void *operator new(size_t size){ return vmem_aligned_alloc(vmp, 64, size); }
	
};
//链式hash
struct Node               // A bucket
{
    uint8_t token[ASSOC_NUM];             // A token indicates whether its corresponding slot is empty, which can also be implemented using 1 bit
    Entry slot[ASSOC_NUM];
	struct Node* next;//C语言的next指针前比较带struct关键字
	char dummy[5];
	void *operator new[](size_t size){
		return vmem_aligned_alloc(vmp, 64, size);
	}
	void *operator new(size_t size) { return vmem_aligned_alloc(vmp, 64, size);}
};

class BFHT :public hash_api
{
private:
	uint64_t init_capacity; // the number of original buckets in hash table

	uint64_t original_table_size; // the original table size in hash table

	Node* buckets; // the buckets array in hash table

	uint64_t hash_seed; //randomized seeds for hashing functions

	uint8_t flag; // a true flag means normal shutdown, and a false flag means system failure occur

	std::shared_mutex *mutex;
	int nlocks;
	int locksize;
	void generate_seeds(void);

	//for bloom filter
	bloom_parameters parameters;
	bloom_filter *filter;

public:
	BFHT(void);
	BFHT(size_t);
	~BFHT(void);
	bool InsertOnly(Key_t &, Value_t);
	bool Insert(Key_t &, Value_t);
	bool Delete(Key_t &);
	bool Recovery();
	Value_t Get(Key_t &);
	hash_Utilization Utilization(void);
	size_t Capacity(void){
		return init_capacity * ASSOC_NUM;
	}
	size_t Size(void);

//	hash_api
	void vmem_print_api(){ vmem_print(); }
	std::string hash_name() {return "ConHash";}
	bool recovery (){
		Recovery();
		return true;
	}
	hash_Utilization utilization(){ return Utilization(); }
  bool find(const char *key, size_t key_sz, char *value_out, unsigned tid)
  {
    Key_t k = *reinterpret_cast<const Key_t *>(key);
    auto r = Get(k);

    return r;
  }

  bool insert(const char *key, size_t key_sz, const char *value,
              size_t value_sz, unsigned tid, unsigned t)
  {
    Key_t k = *reinterpret_cast<const Key_t *>(key);
    Insert(k, value);
    return true;
  }
  bool insertResize(const char *key, size_t key_sz, const char *value,
                    size_t value_sz)
  {
    Key_t k = *reinterpret_cast<const Key_t *>(key);
    return Insert(k, value);
  }
  bool update(const char *key, size_t key_sz, const char *value,
              size_t value_sz)
  {
    return true;
  }

  bool remove(const char *key, size_t key_sz, unsigned tid)
  {
    Key_t k = *reinterpret_cast<const Key_t *>(key);
    return Delete(k);
  }

  int scan(const char *key, size_t key_sz, int scan_sz, char *&values_out)
  {
    return scan_sz;
  }
  
};



#endif  //BFHT_H_