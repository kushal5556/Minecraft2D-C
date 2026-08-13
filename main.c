#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH  800
#define HEIGHT 600

// --- macros ----
#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)

// ---- Global Variable -------------
#define BLOCK_WIDTH 30
#define BLOCK_HEIGHT 30

#define  CHUNK_WIDTH 10
#define  CHUNK_HEIGHT 10

#define CHUNK_DISTANCE 1 //chunk one both sides (not including current)

#define PLAYER_REACH_DISTANCE 3 //blocks
#define RAY_STEP 10 
#define RAY_RADIUS 5.0f

#define GRAVITY 200.0f

#define WORLD_SIZE 10
//----- camera ----
Vector2 CAMERA;

//----------- structs -----------
typedef enum{
	AIR = 0,
	DIRT,
	STONE,
}BlockType;

typedef struct{
	BlockType type;
	bool isBreak; 
	bool isHover;
}Block;

typedef struct{
	Vector2 position;
	Block blocks[CHUNK_WIDTH*CHUNK_HEIGHT];
}Chunk;

typedef struct{
	Vector2 position;
	Vector2 velocity;
	Vector2 ray;
}Player;

typedef struct{
	int x;
	int y;
}DirectionXY_i;

// ---- Global variables ----------
static Block* targetBlock = NULL;
static Block* hoveredBlock = NULL;
static float breakProgress = 0.0f;
const float TIME_TO_BREAK = 0.8f; 
Vector2 hitPosition = {0};

// ----- function declaration -----------------
void init_world(Chunk chunks[], int size);
void draw_chunk(Chunk* chunk);

//---chunk edits ---
int chunk_coord(float posX);
int chunk_index(int chunk_coord);

int getLeftChunkIndex(int chunkIdx);
int getRightChunkIndex(int chunkIdx);

void breakBlock();

void findHoveredBlock(Chunk chunks[], Vector2 playerPos, Vector2 playerSize);

DirectionXY_i blockPlayerDirection(Chunk chunks[], Vector2 playerPosition, int blockChunkIdx);
void placeBlockAt(Chunk chunks[], int blockChunkIdx, int targetX, int targetY, BlockType type, Vector2 playerPos, Vector2 playerSize);
bool isBlockBreak(Chunk chunks[], int blockX, int blockY, int chunkIdx);

void add_chunk(Chunk chunks[], Vector2 pos);

// collision function
bool player_collided(Chunk chunks[], Vector2 position, Vector2 size);
bool AABB(Vector2 posA, Vector2 sizeA, Vector2 posB, Vector2 sizeB);
bool rect_circle_collision(Vector2 rectPos, Vector2 rectSize, Vector2 circlePos, float circleRadius);
bool point_rect_collision(Vector2 point, Vector2 rectPos, Vector2 rectSize);

// ------------ textures ------------------
Texture2D grassBlock;
Texture2D stoneBlock;
Texture2D steve;

int main()
{
	// ------ initialize ----------
	InitWindow(WIDTH, HEIGHT, "Minecraft 2D");
	SetTargetFPS(60);
	grassBlock = LoadTexture("Textures/GrassBlock.png");
	stoneBlock = LoadTexture("Textures/StoneBlock.png");
	steve      = LoadTexture("Textures/steve.png");

	Chunk chunks[WORLD_SIZE];
	init_world(chunks, WORLD_SIZE);

	Player player = {
		.position = (Vector2){0, -50},
		.velocity = {0}
	};
	Vector2 playersize = {BLOCK_WIDTH-2, BLOCK_HEIGHT-2};

	// ----------- game loop -------------------
	while(!WindowShouldClose()){
		// ------------------ update -------------------
		float dt = GetFrameTime();
		float speed = 80;
		float damping = 0.99f;
		player.velocity.y += GRAVITY * dt;
		player.velocity.x *= damping;
		player.velocity.y *= damping;


		if(IsKeyPressed(KEY_SPACE)){
			player.velocity.y -= ((GRAVITY/2)+10)*speed * dt;
		}
		if(IsKeyDown(KEY_S)){
			player.velocity.y += speed * dt;
		}
		if(IsKeyDown(KEY_D)){
			player.velocity.x += speed * dt;
		}
		if(IsKeyDown(KEY_A)){
			player.velocity.x -= speed * dt;
		}

		// --- update player position -------------
		player.position.x += player.velocity.x * dt;
		if(player_collided(chunks, player.position, playersize)){	
			player.position.x -= player.velocity.x * dt;
			player.velocity.x = 0;
		}
		player.position.y += player.velocity.y * dt;
		if(player_collided(chunks, player.position, playersize)){	
			player.position.y -= player.velocity.y * dt;
			player.velocity.y = 0;
		}

		// --- update camera (centered around player) --------------
		CAMERA.x = player.position.x - (WIDTH/2);
		CAMERA.y = player.position.y - (HEIGHT/2);

		// ------------ update chunks ---------------
		//--- ray casting ---------
		findHoveredBlock(chunks, player.position, playersize);

		// ---- block breaking -----------
		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
			breakBlock();

		}else{
			targetBlock = NULL;
			breakProgress = 0.0f;
		}

		// ---- block placing -----------
		if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
			if(hoveredBlock != NULL){
				int blockChunkIdx = chunk_index(chunk_coord(hitPosition.x));

				//find direction 
				DirectionXY_i direction = blockPlayerDirection(chunks, player.position, blockChunkIdx);
				int targetX = direction.x;
				int targetY = direction.y;

				//place blocks
				//find the correct chunk
				if(targetX < 0){
					//left chunk
					int leftChunkIdx = getLeftChunkIndex(blockChunkIdx);

					if(leftChunkIdx  >= 0 && leftChunkIdx < WORLD_SIZE){
						targetX += CHUNK_WIDTH; 
						blockChunkIdx = leftChunkIdx;
					}
				}else if(targetX >= CHUNK_WIDTH){
					//right chunk
					int rightChunkIdx = getRightChunkIndex(blockChunkIdx);

					if(rightChunkIdx >= 0 && rightChunkIdx < WORLD_SIZE){
						targetX -= CHUNK_WIDTH;
						blockChunkIdx = rightChunkIdx;
					}
				}

	            placeBlockAt(chunks, blockChunkIdx, targetX, targetY, STONE, player.position, playersize);
			}
		}
	
		// ------------ clear and draw --------------------
		BeginDrawing();
		ClearBackground(BLACK);

		int playerChunkCoordX = chunk_coord(player.position.x);
		for(int i = -CHUNK_DISTANCE ; i <= CHUNK_DISTANCE; i++){
			int chunkIdx = chunk_index(playerChunkCoordX + i);

			if(chunkIdx < 0 || chunkIdx >= WORLD_SIZE) continue;
			draw_chunk(&chunks[chunkIdx]);

			float x = chunks[chunkIdx].position.x;
			float y = chunks[chunkIdx].position.y;
			if(chunk_index(chunk_coord(player.position.x)) == chunk_index(chunk_coord(x))){
				//DrawRectangleLinesEx((Rectangle){x-CAMERA.x,y-CAMERA.y,CHUNK_WIDTH*BLOCK_WIDTH, CHUNK_HEIGHT*BLOCK_HEIGHT}, 3, BLUE);
			}else{
				//DrawRectangleLinesEx((Rectangle){x-CAMERA.x,y-CAMERA.y,CHUNK_WIDTH*BLOCK_WIDTH, CHUNK_HEIGHT*BLOCK_HEIGHT}, 3, WHITE);
			}
		}

		//DrawRectangle(player.position.x-CAMERA.x, player.position.y - CAMERA.y, playersize.x, playersize.y, RED);
		DrawTexture(steve, player.position.x-CAMERA.x, player.position.y-CAMERA.y, WHITE);
		DrawFPS(5,5);
		EndDrawing();
	}

	// -------- close everything ------------------
	UnloadTexture(grassBlock);
	UnloadTexture(stoneBlock);
	UnloadTexture(steve);
	CloseWindow();
	return 0;
}
/// ------------------- function definition --------------------

void draw_chunk(Chunk* chunk)
{
	for(int y = 0; y < CHUNK_HEIGHT; y++){
		for(int x = 0; x < CHUNK_WIDTH; x++){
			int idx = x + (y * CHUNK_WIDTH);
			if(chunk->blocks[idx].isBreak) continue;

			int blockX = chunk->position.x + (x * BLOCK_WIDTH);
			int blockY = chunk->position.y + (y * BLOCK_HEIGHT);

			switch (chunk->blocks[idx].type){
				case DIRT:
					DrawTexture(grassBlock, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case STONE:
					DrawTexture(stoneBlock, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				default:
					break;
			}
			//--highlight hovered block ----
			if(chunk->blocks[idx].isHover){
				DrawRectangleLines(blockX - CAMERA.x, blockY - CAMERA.y, BLOCK_WIDTH, BLOCK_HEIGHT, BLACK);	
				chunk->blocks[idx].isHover = false;
			}

			// If this specific block is currently being mined, draw a progress overlay
			if (&chunk->blocks[idx] == targetBlock && breakProgress > 0.0f) {
			    // Option A: Simple semi-transparent dark overlay showing progress
			    float alpha = breakProgress / TIME_TO_BREAK;
			   DrawRectangle(blockX - CAMERA.x, blockY - CAMERA.y, BLOCK_WIDTH, BLOCK_HEIGHT, (Color){0, 0, 0, (unsigned char)(alpha * 150)});
			    
			    // Option B: Draw a mini progress bar above the block
			   DrawRectangle(blockX - CAMERA.x, blockY - CAMERA.y - 8, BLOCK_WIDTH, 5, LIGHTGRAY);
			    DrawRectangle(blockX - CAMERA.x, blockY - CAMERA.y - 8, BLOCK_WIDTH * (breakProgress / TIME_TO_BREAK), 5, GREEN);
			}
		}
	}
}

void init_world(Chunk chunks[], int size)
{
	Vector2 pos = {0,0};
	for(int i = 0; i < size; i++){
		Vector2 chunk_position = (Vector2){pos.x + (i*(CHUNK_WIDTH * BLOCK_WIDTH)), pos.y};
		chunk_position.x -= (size/2)*(CHUNK_WIDTH*BLOCK_WIDTH);

		int chunkIdx = chunk_index(chunk_coord(chunk_position.x));

		chunks[chunkIdx].position = chunk_position;

		for(int y = 0; y < CHUNK_HEIGHT; y++){
			for(int x = 0; x < CHUNK_WIDTH; x++){
				int idx = x + (y * CHUNK_WIDTH);

				chunks[chunkIdx].blocks[idx] = (Block){
					.type = y < (CHUNK_HEIGHT/2)/2 ? DIRT : STONE,
					.isBreak = false,
					.isHover = false
				};

				chunks[chunkIdx].blocks[idx].type = GetRandomValue(0,2);
				if(chunks[chunkIdx].blocks[idx].type == AIR){
					chunks[chunkIdx].blocks[idx].isBreak = true;
				}
			}
		}
	}
}

int chunk_index(int chunk_coord)
{
	if(chunk_coord >= 0){
		return chunk_coord * 2;
	}else if(chunk_coord < 0){
		return (-chunk_coord * 2) - 1;
	}
	return -1;
}

int chunk_coord(float posX)
{
	return (int)floorf(posX / (CHUNK_WIDTH * BLOCK_WIDTH));	
}

bool player_collided(Chunk chunks[], Vector2 position, Vector2 size)
{	
	int playerChunkCoordX = chunk_coord(position.x);	
	for(int i = -1; i <= 1; i++){ //only check current , left and right chunk
		int chunkIdx = chunk_index(playerChunkCoordX + i);

		if(chunkIdx < 0 || chunkIdx >= WORLD_SIZE) continue;

		for(int y = 0; y < CHUNK_HEIGHT; y++){
			for(int x = 0; x < CHUNK_WIDTH; x++){
				int idx = x + (y * CHUNK_WIDTH);

				if(chunks[chunkIdx].blocks[idx].isBreak) continue;

				Vector2 bpos = {chunks[chunkIdx].position.x + (x * BLOCK_WIDTH), chunks[chunkIdx].position.y + (y * BLOCK_HEIGHT)};
				Vector2 bsize = {BLOCK_WIDTH, BLOCK_HEIGHT};

				if(AABB(position, size, bpos, bsize)){
					return true;
				}
			}
		}
	}
	return false;
}

bool AABB(Vector2 posA, Vector2 sizeA, Vector2 posB, Vector2 sizeB)
{
	if( posA.x < posB.x  + sizeB.x &&
		posA.x + sizeA.x > posB.x  &&
		posA.y < posB.y  + sizeB.y &&
		posA.y + sizeA.y > posB.y  ){
		return true;
	}
	return false;
}

bool rect_circle_collision(Vector2 rectPos, Vector2 rectSize, Vector2 circlePos, float circleRadius)
{
	float closest_x = MAX(rectPos.x, MIN(rectPos.x + rectSize.x, circlePos.x));
	float closest_y = MAX(rectPos.y, MIN(rectPos.y + rectSize.y, circlePos.y));

	float dx = circlePos.x - closest_x;
	float dy = circlePos.y - closest_y;

	float dist_sq = (dx*dx) + (dy*dy);
	if(dist_sq <= circleRadius*circleRadius){
		return true;
	}
	return false;
}

bool point_rect_collision(Vector2 point, Vector2 rectPos, Vector2 rectSize)
{
	if((point.x >= rectPos.x) && (point.x <= rectPos.x + rectSize.x) && 
	   (point.y >= rectPos.y) && (point.y <= rectPos.y + rectSize.y) ){
		return true;
	}
	return false;
}

int getLeftChunkIndex(int chunkIdx)
{
	if(chunkIdx % 2 == 0){
		if(chunkIdx == 0){
			return 1;
		}else{
			return chunkIdx - 2;
		}
	}else{
		return chunkIdx + 2;
	}
	return -1;
}

int getRightChunkIndex(int chunkIdx)
{
	if(chunkIdx % 2 == 0){
		return chunkIdx + 2;	
	}else{
		if(chunkIdx == 1){
			return 0;
		}else{
			return chunkIdx - 2;
		}
	}
	return -1;
}

void placeBlockAt(Chunk chunks[], int blockChunkIdx, int targetX, int targetY, BlockType type, Vector2 playerPos, Vector2 playerSize)
{
	if(targetX >= 0 && targetX < CHUNK_WIDTH && targetY >= 0 && targetY < CHUNK_HEIGHT){
		int targetIdx = targetX + (targetY * CHUNK_WIDTH);

		if(chunks[blockChunkIdx].blocks[targetIdx].isBreak){
			Vector2 bpos = {
				chunks[blockChunkIdx].position.x + (targetX * BLOCK_WIDTH),
				chunks[blockChunkIdx].position.y + (targetY * BLOCK_HEIGHT),
			};
			Vector2 bsize = {BLOCK_WIDTH, BLOCK_HEIGHT};

			if(!AABB(playerPos, playerSize, bpos, bsize)){
				chunks[blockChunkIdx].blocks[targetIdx].isBreak = false;
				chunks[blockChunkIdx].blocks[targetIdx].type = type;
			}
		}
	}	
}

bool isBlockBreak(Chunk chunks[], int blockX, int blockY, int chunkIdx)
{
	int targetChunk = chunkIdx;

	if(blockX < 0){
		blockX += CHUNK_WIDTH;
		targetChunk = getLeftChunkIndex(chunkIdx);
	}else if (blockX >= CHUNK_WIDTH){
		blockX -= CHUNK_WIDTH;
		targetChunk = getRightChunkIndex(chunkIdx);
	}

	if(targetChunk < 0 || targetChunk >= WORLD_SIZE) return false;

	int idx = blockX + (blockY * CHUNK_WIDTH);
	return chunks[targetChunk].blocks[idx].isBreak;
}

DirectionXY_i blockPlayerDirection(Chunk chunks[], Vector2 playerPosition, int blockChunkIdx)
{

	int blockX = (int)(hitPosition.x - chunks[blockChunkIdx].position.x) / BLOCK_WIDTH;
	int blockY = (int)(hitPosition.y - chunks[blockChunkIdx].position.y) / BLOCK_HEIGHT;

	int playerX = (int)(playerPosition.x - chunks[blockChunkIdx].position.x) / BLOCK_WIDTH;
	int playerY = (int)(playerPosition.y - chunks[blockChunkIdx].position.y) / BLOCK_WIDTH;

	int targetX = -1;
	int targetY = -1;

	if(blockX == playerX){
		//up or down
		if(blockY < playerY){
			targetX = blockX;
			targetY = blockY +1;
		}else{
			targetX = blockX;
			targetY = blockY -1;
		}
	}else if(blockX < playerX){
		//left 
		if(blockY <= playerY){
			//up or same height
			if(blockY < playerY-1){
			//more than 2 block up
				targetX = blockX;							
				targetY = blockY + 1; //if not down,down
				if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
					//right
					targetX = blockX + 1; 
					targetY = blockY;
				}
			}else{	
				//right
				if(blockX < playerX-1){
					targetX = blockX + 1;//if not right, right
					targetY = blockY;
					if(!isBlockBreak(chunks,targetX, targetY, blockChunkIdx)){
						//down
						targetX = blockX;
						targetY = blockY + 1;
					}
				}else{
					if(blockY < playerY){
						targetX = blockX;
						targetY = blockY + 1;//if not down, down
						if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
							//right
							targetX = blockX + 1;
							targetY = blockY;
						}
					}else{
						targetX = blockX + 1;
						targetY = blockY;
					}
				}
			}
		}else{
			//down
			if(blockY > playerY+1){
				//more than 2 block down
				targetX = blockX;
				targetY = blockY - 1;//if not up, up
				if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
					//right
					targetX = blockX + 1;
					targetY = blockY;
				}
			}else{
				if(blockX < playerX-1){
					//more than 2 block left
					//right
					targetX = blockX + 1;//if not right, right
					targetY = blockY;
					if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
						//up
						targetX = blockX;
						targetY = blockY - 1;
					}
				}else{
					targetX = blockX;
					targetY = blockY - 1; //if not up, up
					if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
						//right
						targetX = blockX + 1;
						targetY = blockY;
					}
				}	
			}
		}
	}else if(blockX > playerX){
		//right
		if(blockY <= playerY){
			//up or same level
			if(blockY < playerY-1){
				//more than 2 block up
				targetX = blockX;
				targetY = blockY + 1;//if not below, below
				if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
					//left
					targetX = blockX - 1;
					targetY = blockY;
				}
			}else{
				//left
				if(blockX > playerX+1){
					targetX = blockX - 1;//if not left, left
					targetY = blockY;
					if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
						targetX = blockX;
						targetY = blockY + 1;
						//down
					}
				}else{
					if(blockY < playerY){
						targetX = blockX;
						targetY = blockY + 1;//if not down, down
						if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
							//left
							targetX = blockX - 1;
							targetY = blockY;
						}
					}else{
						targetX = blockX - 1;
						targetY = blockY;
					}
				}
			}
		}else{
			//down
			if(blockY > playerY + 1){
				//more than 2 block, down
				targetX = blockX;
				targetY = blockY - 1;//if not up, up
				if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
					//left
					targetX = blockX - 1;
					targetY = blockY;	
				}
			}else{
				if(blockX > playerX+1){
					//more than 2 block right	
					//left
					targetX = blockX - 1;//if not left, left
					targetY = blockY;
					if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
						//up
						targetX = blockX;
						targetY = blockY - 1;
					}
				}else{
					targetX = blockX;
					targetY = blockY - 1;//if not up, up
					if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
						//left
						targetX = blockX - 1;
						targetY = blockY;
					}
				}
			}
		}
	}

	return (DirectionXY_i){targetX, targetY};
}

void breakBlock()
{
	if(hoveredBlock != NULL){

		if(targetBlock != hoveredBlock){
			targetBlock = hoveredBlock;
			breakProgress = 0.0f;
		}

		breakProgress += GetFrameTime();

		if(breakProgress >= TIME_TO_BREAK){
			targetBlock->isBreak = true;
			targetBlock = NULL;
			breakProgress = 0.0f;
		}
	}else{
		targetBlock = NULL;
		breakProgress = 0.0f;
	}
}

void findHoveredBlock(Chunk chunks[], Vector2 playerPos, Vector2 playerSize)
{
	Vector2 mouse = GetMousePosition();
	mouse.x += CAMERA.x;
	mouse.y += CAMERA.y;

	Vector2 playerRay = (Vector2){0,0};
	Vector2 playerCenter = (Vector2){playerPos.x + (playerSize.x/2), playerPos.y + (playerSize.y/2)};

	float dx =  mouse.x - playerCenter.x;
	float dy =  mouse.y - playerCenter.y;

	float dist = sqrt(dx * dx + dy*dy);
	if(dist == 0.0f) dist = 0.001f;

	float nx = dx/dist;
	float ny = dy/dist;

	hoveredBlock = NULL;
	for(int step = 0; step < PLAYER_REACH_DISTANCE*BLOCK_WIDTH; step += RAY_STEP){

		playerRay.x = playerCenter.x + (step * nx);
		playerRay.y = playerCenter.y + (step * ny);

		bool rayHitSomething = false;

		int rayChunkCoordX = chunk_coord(playerRay.x);
		int rayChunkIdx = chunk_index(rayChunkCoordX);

		if(rayChunkIdx < 0 || rayChunkIdx >= WORLD_SIZE) continue;

		int localX = (int) floorf((playerRay.x - chunks[rayChunkIdx].position.x) / BLOCK_WIDTH);
		int localY = (int) floorf((playerRay.y - chunks[rayChunkIdx].position.y) / BLOCK_HEIGHT);

		if(localX >= 0 && localX < CHUNK_WIDTH && localY >= 0 && localY < CHUNK_HEIGHT){
			int idx = localX + (localY * CHUNK_WIDTH);

			if(!chunks[rayChunkIdx].blocks[idx].isBreak){
				Vector2 bpos = {
					chunks[rayChunkIdx].position.x + (localX * BLOCK_WIDTH),
					chunks[rayChunkIdx].position.y + (localY * BLOCK_HEIGHT)
				};
				Vector2 bsize = {BLOCK_WIDTH, BLOCK_HEIGHT};

				Rectangle blockRect = {bpos.x, bpos.y, bsize.x, bsize.y};

				if(rect_circle_collision(bpos, bsize, playerRay, RAY_RADIUS)){
					chunks[rayChunkIdx].blocks[idx].isHover = true;
					rayHitSomething = true;

					hoveredBlock = &chunks[rayChunkIdx].blocks[idx];
					hitPosition = bpos;
					break;
				}
			}
		}
		if(rayHitSomething) break;
	}
}

void add_chunk(Chunk chunks[], Vector2 pos)
{
	chunks;	
}