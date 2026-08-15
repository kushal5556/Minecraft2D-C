#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// --- Macross ---
#define da_append(arr, x)\
	do{\
		if((arr).size >= (arr).capacity){\
			(arr).capacity = (arr).capacity == 0 ? 10 : (arr).capacity * 2;\
			(arr).items = realloc((arr).items, sizeof(*(arr).items)*(arr).capacity);\
			if((arr).items == NULL){perror("[ERROR]: Failed to Realloc\n"); exit(1);}\
		}\
		(arr).items[(arr).size++] = x;\
	} while(0)

/// --- Structs -----
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

typedef struct{
	float x,y;
}Vector2;

// --- function declaration -----
Vector2 randomGradient(int ix, int iy);
float dotGridGradient(int ix, int iy, float x, float y);
float interpolate(float a0, float a1, float w);
float perlin(float x, float y);

int main(){

	return 0;	
}

// ----- Fnction definition 
Vector2 randomGradient(int ix, int iy)
{
	//No precomputed gradients mean this works for any number of grid coordinates
	const unsigned w = 8 * sizeof(unsigned);
	const unsigned s = w / 2;
	unsigned a = ix, b = iy;
	a *= 3284157443;

	b ^= a << s | a >> w - s;
	b *= 1911520717;

	a ^= b << s | b >> w - s;
	a *= 2048419325;
	float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*pi]

	//Create the vector from the angle
	Vector2 v = {
		.x = sin(random),
		.y = cos(random),
	};
	return v;
}

//compute the dot product of the distance and gradient vectors
float dotGridGradient(int ix, int iy, float x, float y)
{
	//Get gradient from integer coordinates
	Vector2 gradient = randomGradient(ix, iy);

	//compute the distance vector
	float dx = x - (float)ix;
	float dy = y - (float)iy;

	//compute the dot-product
	return (dx * gradient.x + dy*gradient.y);
}

float interpolate(float a0, float a1, float w)
{
	return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
}

float perlin(float x, float y)
{
	//determine gid cell corner coordinates
	int x0 = (int)x;
	int y0 = (int)y;

	int x1 = x0 + 1;
	int y1 = y0 + 1;

	//compute interpolation weights
	float sx = x - (float)x0;
	float sy = y - (float)y0;

	//compute and interpolate top two corners
	float n0 = dotGridGradient(x0, y0, x, y);
	float n1 = dotGridGradient(x1, y0, x, y);
	float ix0 = interpolate(n0, n1, sx);

	//compute and interpolate bottom two corners
	n0 = dotGridGradient(x0, y1, x, y);
	n1 = dotGridGradient(x1, y1, x, y);	
	float ix1 = interpolate(n0, n1, sx);

	//final step: interpolate between the two previously interpolated values, now in y
	float value = interpolate(ix0, ix1, sy);

	return value;
}

