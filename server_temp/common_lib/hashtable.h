#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <pthread.h>

#define MAX_KEY_SIZE 100
#define MAX_ITEM_NUM 10

typedef struct HashItem
{
	char        	key[MAX_KEY_SIZE];     		
	unsigned short 	key_len;        		
	void 			*data;			
	struct HashItem	*next;      	
}HashItemDef;

class HashTable
{
private:
	inline unsigned int DoHash(const void *key, const int key_len);

	void AddFreeItem(HashItemDef *p);
	HashItemDef* GetFreeItem();
	pthread_mutex_t  	table_mutex;		
	unsigned int 		table_size;				 
	HashItemDef  		**hash_items; 		
	HashItemDef			*free_items;
	int					item_count;

public: 
	HashTable();				/* default constructor */
	HashTable(int hash_size);   /* constructor */
	~HashTable();            	/* disconstructor */

	int Add(const char *key, void *data);
	int Add(const void *key, const int key_len, void *data);
	int Remove(const char *key);
	int Remove(const void *key, const int key_len);
	void* Find(const char *key);
	void* Find(const void *key, const int key_len);
};

#endif
