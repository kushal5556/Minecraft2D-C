#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define da_append(arr, x)\
	do{\
		if((arr).size >= (arr).capacity){\
			(arr).capacity = (arr).capacity == 0 ? 10 : (arr).capacity * 2;\
			(arr).items = realloc((arr).items, sizeof(*(arr).items)*(arr).capacity);\
			if((arr).items == NULL){perror("[ERROR]: Failed to Realloc\n"); exit(1);}\
		}\
		(arr).items[(arr).size++] = x;\
	} while(0)

typedef struct{
	int *items;
	int size;
	int capacity;
}DA;

typedef struct{
	int i;
}Chunk;

typedef struct{
	int key;
	Chunk *value;	
}KEY_VALUE;

typedef struct{
	KEY_VALUE **items;
	size_t size;
	size_t capacity;
}Hash_Table;

Hash_Table createHT(size_t capacity){
	Hash_Table ht;
	ht.items = malloc(sizeof(KEY_VALUE*) * capacity);
	ht.size = 0;
	ht.capacity = capacity;
	return ht;
}

int get_hash(float n, int diviser){
	int key = floor(n/diviser);
	if(key >= 0){
		return key * 2;
	}else if(key < 0){
		return (-key * 2) - 1;
	}
	return -1;
}

KEY_VALUE get_pair(int key, Chunk* chunk){
	KEY_VALUE pair;
	pair.value = malloc(sizeof(Chunk));
	pair.key = key;
	pair.value = chunk;
	return pair;	
}

void insert_ht(Hash_Table* hashtable, Chunk* chunk){
	int key = get_hash(chunk->i, 30*10);
	KEY_VALUE pair = get_pair(key, chunk);

	if(hashtable->size >= hashtable->capacity){
		hashtable->capacity = hashtable->capacity == 0 ? 100 : hashtable->capacity * 2;
		hashtable->items = realloc(hashtable->items, sizeof(*hashtable->items)*hashtable->capacity);
		if(hashtable->items == NULL){
			perror("[HASH_TABLE]: failed to Realloc\n");
			exit(1);
		}

	}
	hashtable->items[key % hashtable->capacity] = &pair;
	hashtable->size++;
}

void free_ht(Hash_Table *hashtable){
	if(hashtable == NULL) return;

	for(int i = 0; i < hashtable->size; i++){
		free(hashtable->items[i]->value);
	}

	free(hashtable->items);
	hashtable->items = NULL;
	hashtable->size = hashtable->capacity = 0;
}

int main(){
	Hash_Table ht = createHT(10);

	return 0;	
}

