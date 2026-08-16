#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH  800
#define HEIGHT 600

// --- macros ----
#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)

// ---- Global Constants-------------
#define BLOCK_WIDTH 30
#define BLOCK_HEIGHT 30

#define  CHUNK_WIDTH 20
#define  CHUNK_HEIGHT 100

#define CHUNK_DISTANCE 1 //chunk one both sides (not including current)

#define PLAYER_REACH_DISTANCE 3 //blocks
#define RAY_STEP 10 
#define RAY_RADIUS 5.0f

#define GRAVITY 300.0f

#define WORLD_SIZE 30

//----- camera ----
Vector2 CAMERA;

//----------- structs -----------
typedef enum{
	AIR = 0,
	GRASS,
	STONE,
	WATER,
	BEDROCK,
	DIAMOND_ORE,
	GOLD_ORE,
	EMERALD_ORE,
	IRON_ORE,
	LAVA,
	COBBLESTONE,
	OBSIDIAN
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
	Chunk **items;
	size_t size;
	size_t capacity;
}World;

typedef struct{
	Vector2 position;
	Vector2 velocity;
}Player;

typedef struct{
	int x;
	int y;
}DirectionXY_i;

// ---- Global variables ----------
static Block* targetBlock = NULL;
static Block* hoveredBlock = NULL;
static float breakProgress = 0.0f;
const float TIME_TO_BREAK = 0.6f; 
Vector2 hitPosition = {0};

static float worldSeedOffset = 0.0f;

// ----- function declaration -----------------
void InitializeWorldSeed();

void init_chunk(Chunk* chunk);
void init_world(Chunk chunks[], int size);
void draw_chunk(Chunk* chunk);

//---chunk edit---
int chunk_coord(float posX);
int chunk_index(int chunk_coord);

int getLeftChunkIndex(int chunkIdx);
int getRightChunkIndex(int chunkIdx);

// block edit
void breakBlock();
void placeBlock(Chunk chunks[], Vector2 playerPos, Vector2 playerSize, BlockType type);
void findHoveredBlock(Chunk chunks[], Vector2 playerPos, Vector2 playerSize);

DirectionXY_i blockPlayerDirection(Chunk chunks[], Vector2 playerPosition, int blockChunkIdx);
void placeBlockAt(Chunk chunks[], int blockChunkIdx, int targetX, int targetY, BlockType type, Vector2 playerPos, Vector2 playerSize);
bool isBlockBreak(Chunk chunks[], int blockX, int blockY, int chunkIdx);


// --- noise function ----
float noise(float X);
float pseudo_random_2d(int x, int y);
float smooth_noise(float x);
float terrain_noise(float X); 
bool is_cave(float worldX, float worldY);

void update_chunk(Chunk chunks[], int chunkIdx);

// world chunks edit
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
Texture2D bedRock;
Texture2D waterBlock;
Texture2D lavaBlock;
Texture2D diamondOre;
Texture2D goldOre;
Texture2D ironOre;
Texture2D emeraldOre;
Texture2D cobbleStone;
Texture2D Obsidian;

int main()
{
	// ------ initialize ----------
	InitWindow(WIDTH, HEIGHT, "Minecraft 2D");
	SetTargetFPS(60);
	grassBlock = LoadTexture("Textures/GrassBlock.png");
	stoneBlock = LoadTexture("Textures/StoneBlock.png");
	steve      = LoadTexture("Textures/steve.png");
	bedRock    = LoadTexture("Textures/BedRock.png");
	waterBlock = LoadTexture("Textures/WaterBlock.png");
	lavaBlock  = LoadTexture("Textures/LavaBlock.png");
	diamondOre = LoadTexture("Textures/DiamondOre.png");
	ironOre    = LoadTexture("Textures/IronOre.png");
	emeraldOre = LoadTexture("Textures/EmeraldOre.png");
	goldOre    = LoadTexture("Textures/GoldOre.png");
	cobbleStone = LoadTexture("Textures/CobleStone.png");
	Obsidian = LoadTexture("Textures/Obsidian.png");

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
		float speed = 200;
		float damping = 0.99f;
		player.velocity.y += GRAVITY * dt;
		player.velocity.x *= damping;
		player.velocity.y *= damping;


		if(IsKeyPressed(KEY_SPACE)){
			player.velocity.y -= ((GRAVITY/2)+40)*100* dt;
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
		if(player_collided(chunks, player.position, playersize)){
			player.position.y -= speed * dt;
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
			placeBlock(chunks, player.position, playersize, WATER);
		}

		int playerChunkCoordX = chunk_coord(player.position.x);
		for(int i = -CHUNK_DISTANCE; i <= CHUNK_DISTANCE; i++){
			int chunkIdx = chunk_index(playerChunkCoordX + i);

			if(chunkIdx < 0 || chunkIdx >= WORLD_SIZE) continue;
			update_chunk(chunks, chunkIdx);
		}
	
		// ------------ clear and draw --------------------
		BeginDrawing();
		ClearBackground(BLACK);

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
		static float timer = 0.0f;
		timer += dt;
		char coord[200];
		if(timer >= 0.3f){
			sprintf(coord, "X: %f | Y: %f", player.position.x, player.position.y);
			timer = 0.0f;
		}
		DrawText(coord, 5, 35,15, WHITE);
		DrawFPS(5,5);
		EndDrawing();
	}

	// -------- close everything ------------------
	UnloadTexture(grassBlock);
	UnloadTexture(stoneBlock);
	UnloadTexture(steve);
	UnloadTexture(bedRock);
	UnloadTexture(waterBlock);
	UnloadTexture(lavaBlock);
	UnloadTexture(ironOre);
	UnloadTexture(goldOre);
	UnloadTexture(emeraldOre);
	UnloadTexture(diamondOre);
	UnloadTexture(cobbleStone);
	UnloadTexture(Obsidian);
	CloseWindow();
	return 0;
}
/// ------------------- function definition --------------------
void InitializeWorldSeed()
{
    worldSeedOffset = (float)GetRandomValue(-100000, 100000); 
}

void draw_chunk(Chunk* chunk)
{
	for(int y = 0; y < CHUNK_HEIGHT; y++){
		for(int x = 0; x < CHUNK_WIDTH; x++){
			int idx = x + (y * CHUNK_WIDTH);
			bool isBreak = chunk->blocks[idx].isBreak;
			BlockType type = chunk->blocks[idx].type;

			if(type != AIR && type != WATER && type != LAVA){
				if(isBreak) continue;
			}

			int blockX = chunk->position.x + (x * BLOCK_WIDTH);
			int blockY = chunk->position.y + (y * BLOCK_HEIGHT);

			switch (type){
				case GRASS:
					DrawTexture(grassBlock, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case STONE:
					DrawTexture(stoneBlock, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case AIR:
				//	DrawTexture(waterBlock, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case WATER:
					DrawTexture(waterBlock, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case BEDROCK:
					DrawTexture(bedRock, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case LAVA:
					DrawTexture(lavaBlock, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case DIAMOND_ORE:
					DrawTexture(diamondOre, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case IRON_ORE:
					DrawTexture(ironOre, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case GOLD_ORE:
					DrawTexture(goldOre, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case EMERALD_ORE:
					DrawTexture(emeraldOre, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case COBBLESTONE:
					DrawTexture(cobbleStone, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case OBSIDIAN:
					DrawTexture(Obsidian, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
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

void init_chunk(Chunk* chunk)
{
	float thirdHalf = (CHUNK_HEIGHT*BLOCK_HEIGHT)/3.0f;
    float waterLevel = 0.0f; // Water forms in valleys around Y = -20 to Y = 0
    for(int x = 0; x < CHUNK_WIDTH; x++){
        float worldX = chunk->position.x + (x * BLOCK_WIDTH);
        float surface = noise(worldX);
        int waterCave = GetRandomValue(-5,5);

        for(int y = 0; y < CHUNK_HEIGHT; y++){
            int idx = x + (y * CHUNK_WIDTH);
            float worldY = chunk->position.y + (y * BLOCK_HEIGHT);

            chunk->blocks[idx] = (Block){0};
            chunk->blocks[idx].isBreak = false;
            chunk->blocks[idx].isHover = false;

            // 1. Bedrock at the bottom of the chunk (maximum positive Y)
            if(y == CHUNK_HEIGHT - 1 || worldY >= (chunk->position.y + (CHUNK_HEIGHT * BLOCK_HEIGHT) - (BLOCK_HEIGHT * GetRandomValue(1,3)))) {
                chunk->blocks[idx].type = BEDROCK;
                continue;
            }

            // 2. Air or Water above the surface terrain height
            if(worldY < surface) {
                if(worldY >= waterLevel && surface > -40.0f) {
                    chunk->blocks[idx].type = WATER;
                    chunk->blocks[idx].isBreak = true;
                } else {
                    chunk->blocks[idx].type = AIR;
                    chunk->blocks[idx].isBreak = true;
                }
                continue;
            }

            // 3. Underground Caves & Lava check
            // Only carve caves if we are deep enough underground (e.g., 60 pixels below surface)
            bool carveCave = false;
            if(worldY > surface + 60.0f) {
                if(is_cave(worldX, worldY)) {
                    carveCave = true;
                }
            }

            if(carveCave) {
                // Deep underground Lava check at the very bottom of the caves
                if(worldY > 750.0f) {
                } else {
                }
                float depthUnderground = worldY - surface;
                float randVal = pseudo_random_2d((int)worldX, (int)worldY);
                
                if (depthUnderground > 500.0f && randVal > 0.80f) {
                    chunk->blocks[idx].type = LAVA;
                    chunk->blocks[idx].isBreak = true;
                } else if (depthUnderground > 250.0f && randVal > 0.60f) {
                    chunk->blocks[idx].type = WATER;
                    chunk->blocks[idx].isBreak = true;
                } else{
                    chunk->blocks[idx].type = AIR;
                    chunk->blocks[idx].isBreak = true;
                }
                continue;
            }

            // 4. Surface & Underground Blocks (Grass, Stone, Ores)
            if(worldY <= surface + (BLOCK_HEIGHT * 1)) {
                chunk->blocks[idx].type = GRASS;
            } else {
                BlockType blockType = STONE;

                // Ores spawn deeper underground (+Y coordinates)
                float depthUnderground = worldY - surface;
                float randVal = pseudo_random_2d((int)worldX, (int)worldY);
                
                if (depthUnderground > 30.0f && randVal > 0.99f) {
                    blockType = EMERALD_ORE;
                } else if (depthUnderground > 250.0f && randVal > 0.97f) {
                    blockType = DIAMOND_ORE;
                } else if (depthUnderground > 150.0f && randVal > 0.94f) {
                    blockType = GOLD_ORE;
                } else if (depthUnderground > 90.0f && randVal > 0.91f) {
                    blockType = IRON_ORE;
                }

                chunk->blocks[idx].type = blockType;
            }
        }
    }
}


void init_world(Chunk chunks[], int size)
{
	InitializeWorldSeed();
	Vector2 pos = {0,0};
	for(int i = 0; i < size; i++){
		Vector2 chunk_position = (Vector2){pos.x + (i*(CHUNK_WIDTH * BLOCK_WIDTH)), pos.y - (CHUNK_HEIGHT/2) * BLOCK_HEIGHT};
		chunk_position.x -= (size/2)*(CHUNK_WIDTH*BLOCK_WIDTH);

		int chunkIdx = chunk_index(chunk_coord(chunk_position.x));
		chunks[chunkIdx].position = chunk_position;

		init_chunk(&chunks[chunkIdx]);
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

			bool isBreak = false;
			if(type == WATER || type == LAVA || type == AIR){
				isBreak = true;
			}
			if(!AABB(playerPos, playerSize, bpos, bsize)){
				chunks[blockChunkIdx].blocks[targetIdx].isBreak = isBreak;
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

	float blockWorldX = hitPosition.x;
	float playerWorldX = playerPosition.x;	

	int playerX = (int)(playerPosition.x - chunks[blockChunkIdx].position.x) / BLOCK_WIDTH;
	int playerY = (int)(playerPosition.y - chunks[blockChunkIdx].position.y) / BLOCK_WIDTH;

	int targetX = -1;
	int targetY = -1;

	if(blockWorldX == playerWorldX){
		//up or down
		if(blockY < playerY){
			targetX = blockX;
			targetY = blockY +1;
		}else{
			targetX = blockX;
			targetY = blockY -1;
		}
	}else if(blockWorldX < playerWorldX){
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
	}else if(blockWorldX > playerWorldX){
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
					targetX = blockX - 1; //Issue here
					targetY = blockY;	
				}
			}else{
				if(blockX > playerX+1){
					// more than 2 block right	
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
					targetY = blockY - 1; //if not up, up
					if(!isBlockBreak(chunks, targetX, targetY, blockChunkIdx)){
						// left
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
	if(hoveredBlock != NULL && hoveredBlock->type != BEDROCK){

		if(targetBlock != hoveredBlock){
			targetBlock = hoveredBlock;
			breakProgress = 0.0f;
		}

		breakProgress += GetFrameTime();

		if(breakProgress >= TIME_TO_BREAK){
			targetBlock->isBreak = true;
			targetBlock->type = AIR;
			targetBlock = NULL;
			breakProgress = 0.0f;
		}
	}else{
		targetBlock = NULL;
		breakProgress = 0.0f;
	}
}

void placeBlock(Chunk chunks[], Vector2 playerPos, Vector2 playerSize, BlockType type)
{
	if(hoveredBlock != NULL){
		int blockChunkIdx = chunk_index(chunk_coord(hitPosition.x));

		//find direction 
		DirectionXY_i direction = blockPlayerDirection(chunks, playerPos, blockChunkIdx);
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

        placeBlockAt(chunks, blockChunkIdx, targetX, targetY, type , playerPos, playerSize);
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

// AI noise function
// Simple pseudo-random hash function for 2D coordinates (useful for caves and ores)
float pseudo_random_2d(int x, int y) 
{
    int n = x + y * 57 + (int)worldSeedOffset;
    n = (n << 13) ^ n;
    int nn = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return 1.0f - ((float)nn / 1073741824.0f);
}

// Simple smooth noise function using interpolated pseudo-random values
float smooth_noise(float x) 
{
    int intX = (int)floorf(x);
    float fracX = x - intX;
    
    float v1 = pseudo_random_2d(intX, 0);
    float v2 = pseudo_random_2d(intX + 1, 0);
    
    // Cosine interpolation for smooth transitions (hills/mountains)
    float ft = fracX * 3.1415927f;
    float f = (1.0f - cosf(ft)) * 0.5f;
    
    return v1 * (1.0f - f) + v2 * f;
}

// Multi-octave terrain noise for mountains and valleys
float terrain_noise(float X) 
{
    float total = 0.0f;
    float frequency = 0.0009f;
    float amplitude = 200.0f;
    float maxValue = 0.0f;  // Used for normalizing result
    
    int octaves = 4;
    for(int i = 0; i < octaves; i++) {
        total += smooth_noise((X + worldSeedOffset) * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

// Main height function combining terrain noise with amplitude for pixel heights
float noise(float X) 
{
    float baseHeight = 0.0f;
    float mountainAmplitude = (CHUNK_HEIGHT * BLOCK_HEIGHT)/2.0f; // Controls height of mountains and depth of valleys
    
    // Scale the noise and map it to world height pixels
    float n = terrain_noise(X);
    return baseHeight - (n * mountainAmplitude);
}

// 2D Cave noise function
// Connected, larger cave system using wave interference and smooth contours
bool is_cave(float worldX, float worldY) {
    // Only generate caves well below the surface layer
    // (assuming surface is around Y=0 or negative, and underground is positive Y)
    
    // Scale down frequency for larger, sweeping chambers and tunnels
    float freqX = 0.008f;
    float freqY = 0.008f;
    
    // Create primary winding tunnels
    float n1 = sinf(worldX * freqX + worldSeedOffset) * cosf(worldY * freqY + worldSeedOffset);
    
    // Create secondary branching noise to make them organic and varied
    float n2 = sinf((worldX + worldY) * 0.005f + (worldSeedOffset * 0.5f));
    
    // Combine them to form a continuous network
    float combined = n1 + (n2 * 0.5f);
    
    // A narrow band near zero creates continuous connected tunnels/tubes instead of scattered dots
    // Adjust the thickness (-0.18f to 0.18f) to make caves wider or thinner
    return (combined > -0.18f && combined < 0.18f);
}

//TODO: fix the instant/infinite block generation (check AABB)
void update_chunk(Chunk chunks[], int chunkIdx)
{
	Chunk* chunk = &chunks[chunkIdx];
	Vector2 pos = chunk->position;

	// if checked from top to bottom, when top's liquid goes step down, in next iteration it again gets checked and goes another step
	//checking from bottom to top bypasses the second buffer approach
	for(int y = CHUNK_HEIGHT-1; y >= 0; y--){
		float Y = pos.y + (y * BLOCK_HEIGHT);

		for(int x = 0; x < CHUNK_WIDTH; x++){
			float X = pos.x + (x * BLOCK_WIDTH);

			int idx = x + (y * CHUNK_WIDTH);
			BlockType type = chunk->blocks[idx].type;

			if(type == WATER || type == LAVA){
				bool Moved = false;

				//Down
				if(y < CHUNK_HEIGHT-1){
					int down = y + 1;
					int Didx = x + (down * CHUNK_WIDTH);
					BlockType Dtype = chunk->blocks[Didx].type;

					if(Dtype == AIR){
						chunk->blocks[Didx].type = type;
						Moved = true;
					}else if(Dtype == type){
						Moved = true;
					}else{
						if(type == WATER){
							if(Dtype == LAVA){
								chunk->blocks[Didx].type = OBSIDIAN;
								chunk->blocks[Didx].isBreak = false;
								Moved = true;
							}
						}
						if(type == LAVA){
							if(Dtype == WATER){
								chunk->blocks[Didx].type = STONE;	
								chunk->blocks[Didx].isBreak = false;
								Moved = true;
							}
						}
					}
				}

				//if not skip after a downward flow, it will fill the world instantly
				if(Moved) continue;

				bool go_right = false;	
				if (y < CHUNK_HEIGHT-1){
					int R = x + 1;
					int D = y + 1;

					if(R >= CHUNK_WIDTH){
						R -= CHUNK_WIDTH;	
						int DRidx = R + (D * CHUNK_WIDTH);
						int Ridx = R + (y * CHUNK_WIDTH);

						int rightChunkIdx = getRightChunkIndex(chunkIdx);
						if(rightChunkIdx >= 0  && rightChunkIdx < WORLD_SIZE){

							BlockType DRtype = chunks[rightChunkIdx].blocks[DRidx].type;
							BlockType Rtype = chunks[rightChunkIdx].blocks[Ridx].type;

							if(Rtype == AIR){
								if(DRtype == AIR){
									go_right = true;
								}
							}
						}
					}else{
						int DRidx = R + (D * CHUNK_WIDTH);
						int Ridx = R + (y * CHUNK_WIDTH);

						BlockType DRtype = chunk->blocks[DRidx].type;
						BlockType Rtype = chunk->blocks[Ridx].type;

						if(Rtype == AIR){
							if(DRtype == AIR){
								go_right = true;
							}
						}
					}
				}

				bool go_left = false;	
				if (y < CHUNK_HEIGHT-1){
					int L = x - 1;
					int D = y + 1;

					if(L < 0){
						L += CHUNK_WIDTH;
						int DLidx = L + (D * CHUNK_WIDTH);
						int Lidx = L + (y * CHUNK_WIDTH);

						int leftChunkIdx = getLeftChunkIndex(chunkIdx);
						if(leftChunkIdx >= 0 && leftChunkIdx < WORLD_SIZE){

							BlockType DLtype = chunks[leftChunkIdx].blocks[DLidx].type;	
							BlockType Ltype = chunks[leftChunkIdx].blocks[Lidx].type;	

							if(Ltype == AIR){
								if(DLtype == AIR){
									go_left = true;
								}
							}
						}
					}else{
						int DLidx = L + (D * CHUNK_WIDTH);
						int Lidx = L + (y * CHUNK_WIDTH);

						BlockType DLtype = chunk->blocks[DLidx].type;
						BlockType Ltype = chunk->blocks[Lidx].type;

						if(Ltype == AIR){
							if(DLtype == AIR){
								go_left = true;
							}
						}
					}
				}

				if(true == go_right  && false == go_left){
					//Right
					int right = x + 1;
					if(right >= CHUNK_WIDTH){
						right -= CHUNK_WIDTH;
						int Ridx = right + (y * CHUNK_WIDTH);

						int rightChunkIdx = getRightChunkIndex(chunkIdx);
						if(rightChunkIdx >= 0 && rightChunkIdx < WORLD_SIZE){

							BlockType Rtype = chunks[rightChunkIdx].blocks[Ridx].type;

							if(Rtype == AIR){
								chunks[rightChunkIdx].blocks[Ridx].type = type;
							}else{
								if(type == WATER){
									if(Rtype == LAVA){
										chunks[rightChunkIdx].blocks[Ridx].type = COBBLESTONE;
										chunks[rightChunkIdx].blocks[Ridx].isBreak = false;
									}
								}
								if(type == LAVA){
									if(Rtype == WATER){
										chunks[rightChunkIdx].blocks[Ridx].type = COBBLESTONE;
										chunks[rightChunkIdx].blocks[Ridx].isBreak = false;
									}
								}
							}
						}
					}else{
						int Ridx = right + (y * CHUNK_WIDTH);
						BlockType Rtype = chunk->blocks[Ridx].type;

						if(Rtype == AIR){
							chunk->blocks[Ridx].type = type;
						}else{
							if(type == WATER){
								if(Rtype == LAVA){
									chunk->blocks[Ridx].type = COBBLESTONE;
									chunk->blocks[Ridx].isBreak = false;
								}
							}
							if(type == LAVA){
								if(Rtype == WATER){
									chunk->blocks[Ridx].type = COBBLESTONE;
									chunk->blocks[Ridx].isBreak = false;
								}
							}
						}
					}
				}else if(true == go_left && false == go_right){
					//Left	
					int left = x - 1;
					if(left < 0){
						left += CHUNK_WIDTH;
						int Lidx = left + (y * CHUNK_WIDTH);

						int leftChunkIdx = getLeftChunkIndex(chunkIdx);
						if(leftChunkIdx >= 0 && leftChunkIdx < WORLD_SIZE){

							BlockType Ltype = chunks[leftChunkIdx].blocks[Lidx].type;

							if(Ltype == AIR){
								chunks[leftChunkIdx].blocks[Lidx].type = type;
							}else{
								if(type == WATER){
									if(Ltype == LAVA){
										chunks[leftChunkIdx].blocks[Lidx].type = COBBLESTONE;
										chunks[leftChunkIdx].blocks[Lidx].isBreak = false;
									}
								}
								if(type == LAVA){
									if(Ltype == WATER){
										chunks[leftChunkIdx].blocks[Lidx].type = COBBLESTONE;
										chunks[leftChunkIdx].blocks[Lidx].isBreak = false;
									}
								}
							}
						}
					}else{
						int Lidx = left + (y * CHUNK_WIDTH);
						BlockType Ltype = chunk->blocks[Lidx].type;

						if(Ltype == AIR){
							chunk->blocks[Lidx].type = type;
						}else{
							if(type == WATER){
								if(Ltype == LAVA){
									chunk->blocks[Lidx].type = COBBLESTONE;
									chunk->blocks[Lidx].isBreak = false;
								}
							}
							if(type == LAVA){
								if(Ltype == WATER){
									chunk->blocks[Lidx].type = COBBLESTONE;
									chunk->blocks[Lidx].isBreak = false;
								}
							}
						}
					}
				}else{
					//Right
					int right = x + 1;
					if(right >= CHUNK_WIDTH){
						right -= CHUNK_WIDTH;
						int Ridx = right + (y * CHUNK_WIDTH);

						int rightChunkIdx = getRightChunkIndex(chunkIdx);
						if(rightChunkIdx >= 0 && rightChunkIdx < WORLD_SIZE){

							BlockType Rtype = chunks[rightChunkIdx].blocks[Ridx].type;

							if(Rtype == AIR){
								chunks[rightChunkIdx].blocks[Ridx].type = type;
							}else{
								if(type == WATER){
									if(Rtype == LAVA){
										chunks[rightChunkIdx].blocks[Ridx].type = COBBLESTONE;
										chunks[rightChunkIdx].blocks[Ridx].isBreak = false;
									}
								}
								if(type == LAVA){
									if(Rtype == WATER){
										chunks[rightChunkIdx].blocks[Ridx].type = COBBLESTONE;
										chunks[rightChunkIdx].blocks[Ridx].isBreak = false;
									}
								}
							}
						}
					}else{
						int Ridx = right + (y * CHUNK_WIDTH);
						BlockType Rtype = chunk->blocks[Ridx].type;

						if(Rtype == AIR){
							chunk->blocks[Ridx].type = type;
						}else{
							if(type == WATER){
								if(Rtype == LAVA){
									chunk->blocks[Ridx].type = COBBLESTONE;
									chunk->blocks[Ridx].isBreak = false;
								}
							}
							if(type == LAVA){
								if(Rtype == WATER){
									chunk->blocks[Ridx].type = COBBLESTONE;
									chunk->blocks[Ridx].isBreak = false;
								}
							}
						}
					}

					//Left	
					int left = x - 1;
					if(left < 0){
						left += CHUNK_WIDTH;
						int Lidx = left + (y * CHUNK_WIDTH);

						int leftChunkIdx = getLeftChunkIndex(chunkIdx);
						if(leftChunkIdx >= 0 && leftChunkIdx < WORLD_SIZE){

							BlockType Ltype = chunks[leftChunkIdx].blocks[Lidx].type;

							if(Ltype == AIR){
								chunks[leftChunkIdx].blocks[Lidx].type = type;
							}else{
								if(type == WATER){
									if(Ltype == LAVA){
										chunks[leftChunkIdx].blocks[Lidx].type = COBBLESTONE;
										chunks[leftChunkIdx].blocks[Lidx].isBreak = false;
									}
								}
								if(type == LAVA){
									if(Ltype == WATER){
										chunks[leftChunkIdx].blocks[Lidx].type = COBBLESTONE;
										chunks[leftChunkIdx].blocks[Lidx].isBreak = false;
									}
								}
							}
						}
					}else{
						int Lidx = left + (y * CHUNK_WIDTH);
						BlockType Ltype = chunk->blocks[Lidx].type;

						if(Ltype == AIR){
							chunk->blocks[Lidx].type = type;
						}else{
							if(type == WATER){
								if(Ltype == LAVA){
									chunk->blocks[Lidx].type = COBBLESTONE;
									chunk->blocks[Lidx].isBreak = false;
								}
							}
							if(type == LAVA){
								if(Ltype == WATER){
									chunk->blocks[Lidx].type = COBBLESTONE;
									chunk->blocks[Lidx].isBreak = false;
								}
							}
						}
					}
				}
			}
		}
	}
}

void add_chunk(Chunk chunks[], Vector2 pos)
{
	chunks;	
}