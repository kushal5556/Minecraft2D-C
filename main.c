#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int WINDOW_WIDTH  = 800;
static int WINDOW_HEIGHT = 600;

// --- macros ----
#define da_append(array, item)\
	do{\
	if((array).size >= (array).capacity){\
		(array).capacity = (array).capacity == 0 ? 10 : (array).capacity*2;\
		(array).items = realloc((array).items, sizeof(*(array).items)*(array).capacity);\
		if((array).items == NULL){perror("[DA APPEND]: Failed to Realloc\n"); exit(1);}\
	}\
	(array).items[(array).size++] = item;\
	}while(0)\

#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)
#define NONE -1

// ---- Global Constants -------------
#define BLOCK_WIDTH 30
#define BLOCK_HEIGHT 30

#define  CHUNK_WIDTH 20
#define  CHUNK_HEIGHT 64

#define CHUNK_DISTANCE 1 //chunk one both sides (not including current)

#define PLAYER_REACH_DISTANCE 5 //blocks
#define RAY_STEP 10 
#define RAY_RADIUS 5.0f

#define MAX_FLUID_LEVEL 7

#define GRAVITY 700.0f
#define MOVE_SPEED 150.0f
#define JUMP_FORCE 250.0f

#define INIT_WORLD_SIZE 5

#define INVENTORY_SLOT_COUNT 9

#define ITEM_DESPAWN_TIME 100.0f //seconds
#define ITEM_STACK_COUNT 10

//----- camera ----
Vector2 CAMERA;

//----------- structs -----------
typedef enum{
	LEFT, RIGHT, TOP, BOTTOM
}BlockFace;

typedef enum{
	CREATIVE = 0,
	SURVIVAL
}GameMode;

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
	OBSIDIAN,
	OAKLOG,
	OAKLEAF
}BlockType;

typedef struct{
	int chunkIdx;
	int x;
	int y;
}FluidParent;

typedef struct{
	BlockType type;
	bool isBreak; 
	int fluid_level;
	FluidParent fluid_parent;
}Block;

typedef struct{
	Vector2 position;
	Block blocks[CHUNK_WIDTH*CHUNK_HEIGHT];
	bool used;
	bool structureGenerated;
}Chunk;

typedef struct{
	Chunk *items;
	size_t size;
	size_t capacity;
}World;

typedef struct{
	BlockType type[9];
	size_t count[9];
}Inventory;

typedef struct{
	Inventory inventory;
	Vector2 position;
	Vector2 velocity;
	Vector2 size;
	bool isInGround;
}Player;

typedef struct{
	BlockType type;
	Vector2 position;
	Vector2 velocity;
	float despawnTime;
}Item;

typedef struct{
	Item *items;
	size_t size;
	size_t capacity;
}DA_Item;

// ---- Global variables ----------
static Block* targetBlock = NULL;
static Block* hoveredBlock = NULL;
static float breakProgress = 0.0f;
const float TIME_TO_BREAK = 0.85f; 
Vector2 hitPosition = {0};
Vector2 rayHitPosition = {0};

static float worldSeedOffset = 0.0f;

// ----- function declaration -----------------
void InitializeWorldSeed();

void init_chunk(Chunk* chunk);
void draw_chunk(Chunk* chunk);

int getIndex(int x, int y);

//---chunk edit---
int chunk_coord(float posX);
int chunk_index(int chunk_coord);
float idxToPosition(int chunkIdx);

int getLocalX(World world, float worldX);
int getLocalY(World world, float worldX, float worldY);

int getLeftChunkIndex(int chunkIdx);
int getRightChunkIndex(int chunkIdx);

// block edit
void breakBlock(DA_Item* item);
bool placeBlock(World *world, Vector2 playerPos, Vector2 playerSize, BlockType type);
void findHoveredBlock(World world, Vector2 playerPos, Vector2 playerSize);

BlockFace getBlockFaceFromRay(Vector2 rectPos, Vector2 rectSize, Vector2 playerCenter, Vector2 rayEnd);
bool placeBlockAt(World *world, int blockChunkIdx, int targetX, int targetY, BlockType type, Vector2 playerPos, Vector2 playerSize);
bool isBlockBreak(World world, int blockX, int blockY, int chunkIdx);

// --- noise function ----
float noise(float X);
float pseudo_random_2d(int x, int y);
float smooth_noise(float x);
float terrain_noise(float X); 
bool is_cave(float worldX, float worldY);

bool isSolid(World world, float worldX, float worldY);
bool isLiquid(BlockType type);
bool hasParent(FluidParent parent);
bool FindParent(Block block, World world);
void fluid_system(World* world, Chunk *chunk);

// world chunks edit
void initWorld(World* world);
void freeWorld(World* world);
void addChunk(World *world, int chunkCoordX);
void ensure_capacity(World *world, int targetIdx);

void draw_World(World *world, int playerChunkCoordX);
void generate_structure(World *world, Chunk *chunk);

float snapToBlockRight(float worldX);
float snapToBlockLeft(float worldX);
float snapToBlockTop(float worldY);
float snapToBlockBottom(float worldY);
void updatePlayer(Player* player, World world, float dt, GameMode gameMode);

Texture2D reSizeTexture(Texture2D texture, int width, int height);
void drawInventory(Player player, int slotIndex);
void getInventorySlotIndex(int* slotIndex);
void deleteItemInventory(Inventory* inventory, int slotIndex, BlockType type);

void addItem(DA_Item* item, BlockType type, Vector2 position);
void drawItems(DA_Item item);
void updateItem(DA_Item* item, World world, float dt);
bool ItemCollide(World world, Vector2 itemPos, Vector2 itemSize);
void pickItem(Player *player, DA_Item* item, int slotIndex);
bool addItemToInventory(Player* player, Item item, int slotIndex);

// collision function
bool AABB(Vector2 posA, Vector2 sizeA, Vector2 posB, Vector2 sizeB);
bool rect_circle_collision(Vector2 rectPos, Vector2 rectSize, Vector2 circlePos, float circleRadius);
bool point_rect_collision(Vector2 point, Vector2 rectPos, Vector2 rectSize);

void load_texture();
void unload_texture();

void load_sound();
void unload_sound();

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
Texture2D oakLog;
Texture2D oakLeaf;

// ----- Sounds ------
Sound stonePlace;
Sound stoneBreaking;
Sound itemPick;

int main()
{
	// ------ initialize ----------
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Minecraft 2D");
	InitAudioDevice();
	SetTargetFPS(60);

	load_texture();
	load_sound();

	InitializeWorldSeed();

	World world;
	initWorld(&world);

	DA_Item items = {0};
	GameMode gameMode = SURVIVAL;

	Player player = {
		.inventory = {0},
		.position = (Vector2){0, -50},
		.velocity = {0},
		.size = (Vector2){BLOCK_WIDTH-2, steve.height-2},
		.isInGround = false
	};

	BlockType  SelectedBlock = player.inventory.type[0];
	unsigned int slotIndex = 0;

	for(int i = 0; i < 9; i++){
		player.inventory.type[i] = AIR;
		player.inventory.count[i] = 0;
	}

	// ----------- game loop -------------------
	while(!WindowShouldClose()){
		// ------------------ update -------------------
		float dt = GetFrameTime();

		updatePlayer(&player, world, dt, gameMode);
		//check for new chunk
		addChunk(&world,chunk_coord(player.position.x));

		if(IsKeyPressed(KEY_TAB)){
			if(gameMode == SURVIVAL){
				gameMode = CREATIVE;
			}else{
				gameMode = SURVIVAL;
			}
		}

		getInventorySlotIndex(&slotIndex);
		SelectedBlock = player.inventory.type[slotIndex];

		// -- look for window resize ----
		if(IsWindowResized()){
			WINDOW_WIDTH = GetRenderWidth();
			WINDOW_HEIGHT = GetRenderHeight();
		}

		// --- update camera (centered around player) --------------
		CAMERA.x = player.position.x - (WINDOW_WIDTH/2);
		CAMERA.y = player.position.y - (WINDOW_HEIGHT/2);

		// ------------ update chunks ---------------
		//--- ray casting ---------
		findHoveredBlock(world, player.position, player.size);

		// ---- block breaking -----------
		breakBlock(&items);

		// ---- block placing -----------
		if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
			bool placed = placeBlock(&world, player.position, player.size, SelectedBlock);
			if(placed && gameMode == SURVIVAL){
				deleteItemInventory(&player.inventory,slotIndex, SelectedBlock);
			}
		}

		int playerChunkCoordX = chunk_coord(player.position.x);
		static float update_timer = 0.0f;
		update_timer += dt;
		if(update_timer >= 0.2f){
			update_timer = 0.0f;
			for(int i = -CHUNK_DISTANCE; i <= CHUNK_DISTANCE; i++){
				int chunkIdx = chunk_index(playerChunkCoordX + i);

				if(chunkIdx < 0 || chunkIdx >= world.capacity) continue;
				fluid_system(&world, &world.items[chunkIdx]);
				generate_structure(&world, &world.items[chunkIdx]);
			}
		}

		if(IsKeyPressed(KEY_Q)){
			deleteItemInventory(&player.inventory, slotIndex, SelectedBlock);
		}

		//---update items----
		updateItem(&items, world, dt);
		pickItem(&player, &items, slotIndex);

		// ------------ clear and draw --------------------
		BeginDrawing();
		ClearBackground(SKYBLUE);

		draw_World(&world, chunk_coord(player.position.x));
		drawItems(items);
		drawInventory(player, slotIndex);

		DrawTexture(steve, player.position.x-CAMERA.x, player.position.y-CAMERA.y, WHITE);
		static float timer = 0.0f;
		timer += dt;
		char coord[100];
		if(timer >= 0.3f){
			sprintf(coord, "X: %.2f | Y: %.2f", (player.position.x/BLOCK_WIDTH), (player.position.y/BLOCK_HEIGHT));
			timer = 0.0f;
		}
		DrawCircle(rayHitPosition.x - CAMERA.x, rayHitPosition.y - CAMERA.y, 2, WHITE);
		DrawText(coord, 5, 35,15, WHITE);
		DrawFPS(5,5);
		EndDrawing();
	}

	// -------- close everything ------------------
	unload_texture();
	unload_sound();

	freeWorld(&world);
	free(items.items);

	CloseAudioDevice();
	CloseWindow();
	return 0;
}
/// ------------------- function definition --------------------

float snapToBlockLeft(float worldX)
{
	return floorf(worldX / BLOCK_WIDTH) * BLOCK_WIDTH;
}

float snapToBlockRight(float worldX)
{
	return (floorf(worldX / BLOCK_WIDTH) + 1) * BLOCK_WIDTH;
}

float snapToBlockTop(float worldY)
{
	return floorf(worldY / BLOCK_HEIGHT) * BLOCK_HEIGHT;
}

float snapToBlockBottom(float worldY)
{
	return (floorf(worldY / BLOCK_HEIGHT) + 1) * BLOCK_HEIGHT;
}

void updatePlayer(Player* player, World world, float dt, GameMode gameMode)
{
	Vector2 playerSize= player->size;

	player->velocity.x = 0;
	if(SURVIVAL == gameMode){
		player->velocity.y += GRAVITY * dt;
	}else{
		player->velocity.y = 0;
	}

	if(IsKeyDown(KEY_D)){
		player->velocity.x = MOVE_SPEED;
	}
	if(IsKeyDown(KEY_A)){
		player->velocity.x = -MOVE_SPEED;
	}

	if(CREATIVE == gameMode){
		if(IsKeyDown(KEY_W)){
			player->velocity.y = -MOVE_SPEED;
		}
		if(IsKeyDown(KEY_S)){
			player->velocity.y = MOVE_SPEED;
		}
	}

	if(IsKeyPressed(KEY_SPACE) && player->isInGround){
		player->velocity.y = -JUMP_FORCE;
		player->isInGround = false;
	}

	// --- update player position -------------
	player->position.x += player->velocity.x * dt;
	Vector2 pos = player->position;

	if(player->velocity.x > 0){//going right
		if (isSolid(world, pos.x + playerSize.x, pos.y) ||
			isSolid(world, pos.x + playerSize.x, pos.y + playerSize.y - 1)){

			player->position.x = snapToBlockLeft(pos.x + playerSize.x) - playerSize.x;
			player->velocity.x = 0;
		}
	}else if(player->velocity.x < 0){//going left
		if (isSolid(world, pos.x, pos.y) ||
			isSolid(world, pos.x, pos.y + playerSize.y - 1)){

			player->position.x = snapToBlockRight(pos.x);
			player->velocity.x = 0;
		}
	}
	
	player->position.y += player->velocity.y * dt;
	pos = player->position;
	player->isInGround = false;

	if(player->velocity.y > 0){ //going down
		if (isSolid(world, pos.x, pos.y + playerSize.y) || 
			isSolid(world, pos.x + playerSize.x - 1, pos.y + playerSize.y)){

			player->position.y = snapToBlockTop(pos.y + playerSize.y) - playerSize.y;
			player->velocity.y = 0;
			player->isInGround = true;
		}
	}else if(player->velocity.y < 0){//jumping up
		if (isSolid(world, pos.x, pos.y) || 
			isSolid(world, pos.x + playerSize.x - 1, pos.y)){

			player->position.y = snapToBlockBottom(pos.y);
			player->velocity.y = 0;
		}
	}
}

void InitializeWorldSeed()
{
    worldSeedOffset = (float)GetRandomValue(-100000, 100000); 
}

void draw_chunk(Chunk* chunk)
{
	for(int y = 0; y < CHUNK_HEIGHT; y++){
		for(int x = 0; x < CHUNK_WIDTH; x++){
			int idx = getIndex(x,y);
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
				case OAKLOG:
					DrawTexture(oakLog, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				case OAKLEAF:
					DrawTexture(oakLeaf, blockX-CAMERA.x , blockY-CAMERA.y, WHITE);
					break;
				default:
					break;
			}
			//--highlight hovered block ----
			if(&chunk->blocks[idx] == hoveredBlock){
				DrawRectangleLines(blockX - CAMERA.x, blockY - CAMERA.y, BLOCK_WIDTH, BLOCK_HEIGHT, BLACK);	
			}

			// If block is currently being mined
			if (&chunk->blocks[idx] == targetBlock && breakProgress > 0.0f) {
			    //semi-transparent dark progress overlay 
			    float alpha = breakProgress / TIME_TO_BREAK;
			    DrawRectangle(blockX - CAMERA.x, blockY - CAMERA.y, BLOCK_WIDTH, BLOCK_HEIGHT, (Color){0, 0, 0, (unsigned char)(alpha * 150)});
			    
			    //progress bar above the block
			    DrawRectangle(blockX - CAMERA.x, blockY - CAMERA.y - 8, BLOCK_WIDTH, 5, LIGHTGRAY);
			    DrawRectangle(blockX - CAMERA.x, blockY - CAMERA.y - 8, BLOCK_WIDTH * (breakProgress / TIME_TO_BREAK), 5, GREEN);
			}
		}
	}
}

void init_chunk(Chunk* chunk)
{
	float thirdHalf = (CHUNK_HEIGHT*BLOCK_HEIGHT)/3.0f;
    float waterLevel = 0.0f; 
    for(int x = 0; x < CHUNK_WIDTH; x++){
        float worldX = chunk->position.x + (x * BLOCK_WIDTH);
        float surface = noise(worldX);
        int waterCave = GetRandomValue(-5,5);

        for(int y = 0; y < CHUNK_HEIGHT; y++){
            int idx = getIndex(x,y);
            float worldY = chunk->position.y + (y * BLOCK_HEIGHT);

            chunk->blocks[idx] = (Block){0};
            chunk->blocks[idx].isBreak = false;
            chunk->blocks[idx].fluid_level = NONE;
            chunk->blocks[idx].fluid_parent = (FluidParent){
            	.chunkIdx = NONE,
            	.x = NONE,
            	.y = NONE
            };

            // Bedrock at the bottom of the chunk 
            if(y == CHUNK_HEIGHT - 1 || worldY >= (chunk->position.y + (CHUNK_HEIGHT * BLOCK_HEIGHT) - (BLOCK_HEIGHT * GetRandomValue(1,3)))) {
                chunk->blocks[idx].type = BEDROCK;
                continue;
            }

            //Air or Water above the surface terrain height
            if(worldY < surface) {
                if(worldY >= waterLevel && surface > -40.0f) {
                    chunk->blocks[idx].type = WATER;
                    chunk->blocks[idx].isBreak = true;
                    chunk->blocks[idx].fluid_level = 0;
                } else {
                    chunk->blocks[idx].type = AIR;
                    chunk->blocks[idx].isBreak = true;
                }
                continue;
            }

            // --------Underground Caves & Lava check------
            // Only carve caves if we are deep enough underground (like 60 pixels below surface)
            bool carveCave = false;
            if(worldY > surface + 60.0f) {
                if(is_cave(worldX, worldY)) {
                    carveCave = true;
                }
            }

            if(carveCave) {
                float depthUnderground = worldY - surface;
                float randVal = pseudo_random_2d((int)worldX, (int)worldY);
                
                if (depthUnderground > 400.0f && randVal > 0.80f) {
                    chunk->blocks[idx].type = LAVA;
                    chunk->blocks[idx].isBreak = true;
                    chunk->blocks[idx].fluid_level = 0;
                } else if (depthUnderground > 200.0f && randVal > 0.60f) {
                    chunk->blocks[idx].type = WATER;
                    chunk->blocks[idx].isBreak = true;
                    chunk->blocks[idx].fluid_level = 0;
                } else{
                    chunk->blocks[idx].type = AIR;
                    chunk->blocks[idx].isBreak = true;
                }
                continue;
            }

            //  Surface & Underground Blocks (Grass, Stone, Ores)
            if(worldY <= surface + (BLOCK_HEIGHT * 1)) {
                chunk->blocks[idx].type = GRASS;
              

            } else {
                BlockType blockType = STONE;

                // Ores spawn underground 
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

float idxToPosition(int chunkIdx)
{
	float chunkPixelWidth = (float)(CHUNK_WIDTH * BLOCK_WIDTH);

	if(0 == chunkIdx % 2){
		int step = chunkIdx/2;

		return (float)(chunkPixelWidth*step);
	}else{
		int step = (chunkIdx + 1) / 2;
		return -((float)(step * chunkPixelWidth));
	}
}

int getIndex(int x, int y)
{
	return x + (y * CHUNK_WIDTH);
}

void load_texture()
{
	grassBlock  = LoadTexture("Textures/GrassBlock.png");
	stoneBlock  = LoadTexture("Textures/StoneBlock.png");
	steve       = LoadTexture("Textures/steve.png");
	bedRock     = LoadTexture("Textures/BedRock.png");
	waterBlock  = LoadTexture("Textures/WaterBlock.png");
	lavaBlock   = LoadTexture("Textures/LavaBlock.png");
	diamondOre  = LoadTexture("Textures/DiamondOre.png");
	ironOre     = LoadTexture("Textures/IronOre.png");
	emeraldOre  = LoadTexture("Textures/EmeraldOre.png");
	goldOre     = LoadTexture("Textures/GoldOre.png");
	cobbleStone = LoadTexture("Textures/CobleStone.png");
	Obsidian    = LoadTexture("Textures/Obsidian.png");
	oakLog      = LoadTexture("Textures/OakLog.png");
	oakLeaf     = LoadTexture("Textures/OakLeaf.png");

	steve.height += (BLOCK_HEIGHT/2);
}

void unload_texture()
{
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
	UnloadTexture(oakLog);
	UnloadTexture(oakLeaf);
}

void load_sound()
{
	stonePlace    = LoadSound("Sounds/stonePlace.mp3");
	stoneBreaking = LoadSound("Sounds/stoneBreaking.mp3");
	itemPick      = LoadSound("Sounds/itemPick.mp3");
}

void unload_sound()
{
	UnloadSound(stonePlace);
	UnloadSound(stoneBreaking);
	UnloadSound(itemPick);
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

bool placeBlockAt(World *world, int blockChunkIdx, int targetX, int targetY, BlockType type, Vector2 playerPos, Vector2 playerSize)
{
	if(type == AIR) return false;
	if(targetX >= 0 && targetX < CHUNK_WIDTH && targetY >= 0 && targetY < CHUNK_HEIGHT){
		int targetIdx = getIndex(targetX, targetY);

		if(world->items[blockChunkIdx].blocks[targetIdx].isBreak){
			Vector2 bpos = {
				world->items[blockChunkIdx].position.x + (targetX * BLOCK_WIDTH),
				world->items[blockChunkIdx].position.y + (targetY * BLOCK_HEIGHT),
			};
			Vector2 bsize = {BLOCK_WIDTH, BLOCK_HEIGHT};

			if(!AABB(playerPos, playerSize, bpos, bsize)){
				if(isLiquid(type)){
					BlockType targetBlock = world->items[blockChunkIdx].blocks[targetIdx].type;
					if(isLiquid(targetBlock) && targetBlock != type){
						if(WATER == targetBlock && LAVA == type){
							world->items[blockChunkIdx].blocks[targetIdx].type = STONE;
							world->items[blockChunkIdx].blocks[targetIdx].isBreak = false;
							world->items[blockChunkIdx].blocks[targetIdx].fluid_level = NONE;
							world->items[blockChunkIdx].blocks[targetIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
							return true;
						}else if(LAVA == targetBlock && WATER == type){
							world->items[blockChunkIdx].blocks[targetIdx].type = OBSIDIAN;
							world->items[blockChunkIdx].blocks[targetIdx].isBreak = false;
							world->items[blockChunkIdx].blocks[targetIdx].fluid_level = NONE;
							world->items[blockChunkIdx].blocks[targetIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
							return true;
						}
					}else{
						world->items[blockChunkIdx].blocks[targetIdx].type = type;
						world->items[blockChunkIdx].blocks[targetIdx].isBreak = true;
						world->items[blockChunkIdx].blocks[targetIdx].fluid_level = 0;
						world->items[blockChunkIdx].blocks[targetIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						return true;
					}
				}else{
					world->items[blockChunkIdx].blocks[targetIdx].isBreak = false;
					world->items[blockChunkIdx].blocks[targetIdx].type = type;
					world->items[blockChunkIdx].blocks[targetIdx].fluid_level = NONE;
					world->items[blockChunkIdx].blocks[targetIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};

					PlaySound(stonePlace);
					return true;
				}
			}
		}
	}	
	return false;
}

bool isBlockBreak(World world, int blockX, int blockY, int chunkIdx)
{
	int targetChunk = chunkIdx;

	if(blockX < 0){
		blockX += CHUNK_WIDTH;
		targetChunk = getLeftChunkIndex(chunkIdx);
	}else if (blockX >= CHUNK_WIDTH){
		blockX -= CHUNK_WIDTH;
		targetChunk = getRightChunkIndex(chunkIdx);
	}

	if(targetChunk < 0 || targetChunk >= world.capacity) return false;

	int idx = getIndex(blockX, blockY);
	return world.items[targetChunk].blocks[idx].isBreak;
}

BlockFace getBlockFaceFromRay(Vector2 rectPos, Vector2 rectSize, Vector2 playerCenter, Vector2 rayEnd)
{
    // Box boundaries
    float xMin = rectPos.x;
    float xMax = rectPos.x + rectSize.x;
    float yMin = rectPos.y;
    float yMax = rectPos.y + rectSize.y;

    // Ray direction vector from player to rayEnd
    float dirX = rayEnd.x - playerCenter.x;
    float dirY = rayEnd.y - playerCenter.y;

    // Calculate intersection t-values for all 4 sides
    float tXMin = (dirX != 0) ? (xMin - playerCenter.x) / dirX : -1.0f;
    float tXMax = (dirX != 0) ? (xMax - playerCenter.x) / dirX : -1.0f;
    float tYMin = (dirY != 0) ? (yMin - playerCenter.y) / dirY : -1.0f;
    float tYMax = (dirY != 0) ? (yMax - playerCenter.y) / dirY : -1.0f;

    // Find the entry time (maximum of minimums for entry axes)
    // valid t values between 0.0 and 1.0
    float tEntry = -1.0f;
    BlockFace hitFace = -1;

    // Check X planes
    if (dirX < 0 && tXMax >= 0 && tXMax <= 1.0f){
        if (tXMax > tEntry) { 
        	tEntry = tXMax; 
        	hitFace = RIGHT; 
        }
    } else if (dirX > 0 && tXMin >= 0 && tXMin <= 1.0f){
        if (tXMin > tEntry){ 
        	tEntry = tXMin; 
        	hitFace = LEFT; 
        }
    }

    // Check Y planes
    if (dirY < 0 && tYMax >= 0 && tYMax <= 1.0f){
        if (tYMax > tEntry){ 
        	tEntry = tYMax; 
        	hitFace = BOTTOM; 
        }
    } else if (dirY > 0 && tYMin >= 0 && tYMin <= 1.0f){
        if (tYMin > tEntry){ 
        	tEntry = tYMin; 
        	hitFace = TOP; 
        }
    }

    return hitFace;
}

void breakBlock(DA_Item* item)
{
	if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
		if(hoveredBlock != NULL && hoveredBlock->type != BEDROCK){

			if(targetBlock != hoveredBlock){
				targetBlock = hoveredBlock;
				breakProgress = 0.0f;
			}

			breakProgress += GetFrameTime();
			if(!IsSoundPlaying(stoneBreaking)){
				PlaySound(stoneBreaking);
			}

			if(breakProgress >= TIME_TO_BREAK){
				addItem(item, targetBlock->type, hitPosition);

				targetBlock->isBreak = true;
				targetBlock->type = AIR;
				targetBlock = NULL;
				breakProgress = 0.0f;

			}
		}else{
			StopSound(stoneBreaking);

			targetBlock = NULL;
			breakProgress = 0.0f;
		}

	}else{
		StopSound(stoneBreaking);

		targetBlock = NULL;
		breakProgress = 0.0f;
	}
}

bool placeBlock(World *world, Vector2 playerPos, Vector2 playerSize, BlockType type)
{
	if(hoveredBlock != NULL){
		int blockChunkIdx = chunk_index(chunk_coord(hitPosition.x));

		Vector2 playerCenter = {
			playerPos.x + (playerSize.x/2),
			playerPos.y + (playerSize.y/2),
		};
		BlockFace face = getBlockFaceFromRay(hitPosition, (Vector2){BLOCK_WIDTH, BLOCK_HEIGHT}, playerCenter, rayHitPosition);

		int blockX = (int)(hitPosition.x - world->items[blockChunkIdx].position.x) / BLOCK_WIDTH;
		int blockY = (int)(hitPosition.y - world->items[blockChunkIdx].position.y) / BLOCK_HEIGHT;

		int targetX = blockX;
		int targetY = blockY;

		switch(face){
			case LEFT:
				targetX = blockX - 1;
				break;
			case RIGHT:
				targetX = blockX + 1;
				break;
			case TOP:
				targetY = blockY - 1;
				break;
			case BOTTOM:
				targetY = blockY + 1;
				break;
		}

		//place blocks
		//find the correct chunk
		if(targetX < 0){
			//left chunk
			int leftChunkIdx = getLeftChunkIndex(blockChunkIdx);

			if(leftChunkIdx  >= 0 && leftChunkIdx < world->capacity){
				targetX += CHUNK_WIDTH; 
				blockChunkIdx = leftChunkIdx;
			}
		}else if(targetX >= CHUNK_WIDTH){
			//right chunk
			int rightChunkIdx = getRightChunkIndex(blockChunkIdx);

			if(rightChunkIdx >= 0 && rightChunkIdx < world->capacity){
				targetX -= CHUNK_WIDTH;
				blockChunkIdx = rightChunkIdx;
			}
		}

        return placeBlockAt(world, blockChunkIdx, targetX, targetY, type , playerPos, playerSize);
	}
	return false;
}

void findHoveredBlock(World world, Vector2 playerPos, Vector2 playerSize)
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
	rayHitPosition = (Vector2){-99999, 99999};
	for(int step = 0; step < PLAYER_REACH_DISTANCE*BLOCK_WIDTH; step += RAY_STEP){

		playerRay.x = playerCenter.x + (step * nx);
		playerRay.y = playerCenter.y + (step * ny);

		bool rayHitSomething = false;

		int rayChunkCoordX = chunk_coord(playerRay.x);
		int rayChunkIdx = chunk_index(rayChunkCoordX);

		if(rayChunkIdx < 0 || rayChunkIdx >= world.capacity) continue;

		int localX = (int) floorf((playerRay.x - world.items[rayChunkIdx].position.x) / BLOCK_WIDTH);
		int localY = (int) floorf((playerRay.y - world.items[rayChunkIdx].position.y) / BLOCK_HEIGHT);

		if(localX >= 0 && localX < CHUNK_WIDTH && localY >= 0 && localY < CHUNK_HEIGHT){
			int idx = localX + (localY * CHUNK_WIDTH);

			if(!world.items[rayChunkIdx].blocks[idx].isBreak){
				Vector2 bpos = {
					world.items[rayChunkIdx].position.x + (localX * BLOCK_WIDTH),
					world.items[rayChunkIdx].position.y + (localY * BLOCK_HEIGHT)
				};
				Vector2 bsize = {BLOCK_WIDTH, BLOCK_HEIGHT};

				Rectangle blockRect = {bpos.x, bpos.y, bsize.x, bsize.y};

				if(rect_circle_collision(bpos, bsize, playerRay, RAY_RADIUS)){
					rayHitSomething = true;

					hoveredBlock = &world.items[rayChunkIdx].blocks[idx];
					hitPosition = bpos;
					rayHitPosition = playerRay;
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
bool is_cave(float worldX, float worldY) 
{
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

int getLocalX(World world, float worldX)
{
	int chunkIdx = chunk_index(chunk_coord(worldX));
	if(chunkIdx < 0 || chunkIdx >= world.capacity) return NONE;

	int localX = (int)(worldX - world.items[chunkIdx].position.x) / BLOCK_WIDTH;

	return localX;
}

int getLocalY(World world, float worldX, float worldY)
{
	int chunkIdx = chunk_index(chunk_coord(worldX));
	if(chunkIdx < 0 || chunkIdx >= world.capacity) return NONE;

	int localY = (int)(worldY - world.items[chunkIdx].position.y) / BLOCK_HEIGHT;

	return localY;
}

bool isSolid(World world, float worldX, float worldY)
{
	int chunkIdx = chunk_index(chunk_coord(worldX));

	if(chunkIdx < 0 || chunkIdx >= world.capacity) return false;

	int localX = (int)(worldX - world.items[chunkIdx].position.x) / BLOCK_WIDTH;
	int localY = (int)(worldY - world.items[chunkIdx].position.y) / BLOCK_HEIGHT;

	if(localX < 0 || localX >= CHUNK_WIDTH) return false;
	if(localY < 0 || localY >= CHUNK_HEIGHT) return false;

	Block target = world.items[chunkIdx].blocks[getIndex(localX, localY)];

	if(AIR == target.type || WATER == target.type || LAVA == target.type){
		return false;
	}

	return true;
}

bool isLiquid(BlockType type)
{
	if(WATER == type || LAVA == type){
		return true;
	}
	return false;
}

bool hasParent(FluidParent parent)
{
	if(parent.chunkIdx != NONE && parent.x != NONE & parent.y != NONE){
		return true;
	}
	return false;
}

bool isSource(Block block)
{
	if(!hasParent(block.fluid_parent) && block.fluid_level == 0){
		return true;	
	}
	return false;
}

bool FindParent(Block block, World world)
{
	FluidParent parent = block.fluid_parent;

	if(parent.chunkIdx < 0 || parent.chunkIdx >= world.capacity) return false;
	Block parentBlock = world.items[parent.chunkIdx].blocks[getIndex(parent.x, parent.y)];
		
	if(parentBlock.type != block.type){
		return false;
	}
	return true;
}

void fluid_system(World *world, Chunk *chunk)
{

	int size = CHUNK_WIDTH * CHUNK_HEIGHT;
	int chunkIdx = chunk_index(chunk_coord(chunk->position.x));
	Block block_buffer[size];

	for(int i = 0; i < size; i++){
		block_buffer[i].fluid_level = chunk->blocks[i].fluid_level;
		block_buffer[i].fluid_parent = chunk->blocks[i].fluid_parent;
		block_buffer[i].type = chunk->blocks[i].type;
		block_buffer[i].isBreak = chunk->blocks[i].isBreak;
	}

	for(int y = 0; y < CHUNK_HEIGHT; y++) {
		for(int x = 0; x < CHUNK_WIDTH; x++) {
			int idx = x + (y * CHUNK_WIDTH);

			Block block = chunk->blocks[idx];
			int level = block.fluid_level;	
			int fluid = block.type;
			FluidParent parent = block.fluid_parent;

			if(!isLiquid(fluid)) continue;

			if(!FindParent(block, *world) && !isSource(block)){
				//vanish
				block_buffer[idx].type = AIR;
				block_buffer[idx].fluid_level = NONE;
				block_buffer[idx].fluid_parent = (FluidParent){NONE, NONE, NONE};
				block_buffer[idx].isBreak = true;
				continue;
			}

			if(!hasParent(parent) && !isSource(block)){
				//vanish
				block_buffer[idx].type = AIR;
				block_buffer[idx].fluid_level = NONE;
				block_buffer[idx].fluid_parent = (FluidParent){NONE, NONE, NONE};
				block_buffer[idx].isBreak = true;
				continue;
			}


			//Down
			int down  = y + 1;
			if(down < CHUNK_HEIGHT){
				int downIdx = getIndex(x, down);
				Block down = chunk->blocks[downIdx];

				if(AIR == down.type){
					block_buffer[downIdx].type = fluid;
					block_buffer[downIdx].fluid_level  = level;
					block_buffer[downIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
					continue;
				}
				if(isLiquid(down.type)){
					if(WATER == fluid){
						if(LAVA == down.type){
							block_buffer[downIdx].type    = OBSIDIAN;
							block_buffer[downIdx].isBreak = false;
							block_buffer[downIdx].fluid_level  = NONE;
							block_buffer[downIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						}
					}else if(LAVA == fluid){
						if(WATER == down.type){
							block_buffer[downIdx].type    = STONE;
							block_buffer[downIdx].isBreak = false;
							block_buffer[downIdx].fluid_level  = NONE;
							block_buffer[downIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						}
					}
				continue;
				}
			}

			//Find flow direction
			bool FlowLeft = false;
			bool FlowRight = false;

			//down-left
			int left = x - 1;
			int leftChunkIdx = chunkIdx;

			if(left < 0){
				left += CHUNK_WIDTH;
				leftChunkIdx = getLeftChunkIndex(chunkIdx);
			}

			if(down < CHUNK_HEIGHT){
				if(leftChunkIdx >= 0 && leftChunkIdx < world->capacity){
					int downLeft = left + (down * CHUNK_WIDTH);
					int leftIdx =  left + (y * CHUNK_WIDTH);

					if(world->items[leftChunkIdx].blocks[downLeft].isBreak && world->items[leftChunkIdx].blocks[leftIdx].isBreak){
						if(isLiquid(world->items[leftChunkIdx].blocks[downLeft].type)){
							if(fluid == world->items[leftChunkIdx].blocks[downLeft].type){
								FlowLeft = true;
							}else{
								FlowLeft = true;
								level += 2; //to preven turning whole ocean floor into stone (lava->water)
							}
						}else{
							FlowLeft = true;
						}
					}
				}
			}

			//down-right
			int right = x + 1;
			int rightChunkIdx = chunkIdx;

			if(right >= CHUNK_WIDTH){
				right -= CHUNK_WIDTH;
				rightChunkIdx = getRightChunkIndex(chunkIdx);
			}

			if(down < CHUNK_HEIGHT){
				if(rightChunkIdx >= 0 && rightChunkIdx < world->capacity){
					int downRight = right + (down * CHUNK_WIDTH);
					int rightIdx = right + (y * CHUNK_WIDTH);

					if(world->items[rightChunkIdx].blocks[downRight].isBreak && world->items[rightChunkIdx].blocks[rightIdx].isBreak){
						if(isLiquid(world->items[rightChunkIdx].blocks[downRight].type)){
							if(world->items[rightChunkIdx].blocks[downRight].type == fluid){
								FlowRight = true;
							}else{
								FlowRight = true;
								level += 2;
							}
						}else{
							FlowRight = true;
						}
					}
				}
			}

			if(level >= MAX_FLUID_LEVEL) continue;

			//Flow left
			if(FlowLeft){
				int leftIdx      = left + (y * CHUNK_WIDTH);
				int downLeftIdx  = left + (down * CHUNK_WIDTH);

				Block blockLeft = world->items[leftChunkIdx].blocks[leftIdx];
				Block blockDownLeft = world->items[leftChunkIdx].blocks[downLeftIdx];

				if(AIR == blockLeft.type){
					if(leftChunkIdx == chunkIdx){
						block_buffer[leftIdx].type = fluid;
						block_buffer[leftIdx].fluid_level = level;
						block_buffer[leftIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
						block_buffer[leftIdx].isBreak = true;
					}else{
						world->items[leftChunkIdx].blocks[leftIdx].type = fluid;
						world->items[leftChunkIdx].blocks[leftIdx].fluid_level = level;
						world->items[leftChunkIdx].blocks[leftIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
						world->items[leftChunkIdx].blocks[leftIdx].isBreak = true;
					}
				}else if(isLiquid(blockLeft.type) && fluid != blockLeft.type){
					if(leftChunkIdx == chunkIdx){
						block_buffer[leftIdx].type = COBBLESTONE;
						block_buffer[leftIdx].fluid_level = NONE;
						block_buffer[leftIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						block_buffer[leftIdx].isBreak = false;
					}else{
						world->items[leftChunkIdx].blocks[leftIdx].type = COBBLESTONE;
						world->items[leftChunkIdx].blocks[leftIdx].fluid_level = NONE;
						world->items[leftChunkIdx].blocks[leftIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						world->items[leftChunkIdx].blocks[leftIdx].isBreak = false;
					}
				}
			}
			//Flow right
			if(FlowRight){
				int rightIdx      = right + (y * CHUNK_WIDTH);
				int downRightIdx  = right + (down * CHUNK_WIDTH);

				Block blockRight     = world->items[rightChunkIdx].blocks[rightIdx];
				Block blockDownRight = world->items[rightChunkIdx].blocks[downRightIdx];

				if(AIR == blockRight.type){
					if(rightChunkIdx == chunkIdx){
						block_buffer[rightIdx].type = fluid;
						block_buffer[rightIdx].fluid_level = level;
						block_buffer[rightIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
						block_buffer[rightIdx].isBreak = true;
					}else{
						world->items[rightChunkIdx].blocks[rightIdx].type = fluid;
						world->items[rightChunkIdx].blocks[rightIdx].fluid_level = level;
						world->items[rightChunkIdx].blocks[rightIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
						world->items[rightChunkIdx].blocks[rightIdx].isBreak = true;
					}
				}else if(isLiquid(blockRight.type) && fluid != blockRight.type){
					if(rightChunkIdx == chunkIdx){
						block_buffer[rightIdx].type = COBBLESTONE;
						block_buffer[rightIdx].fluid_level = NONE;
						block_buffer[rightIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						block_buffer[rightIdx].isBreak = false;
					}else{
						world->items[rightChunkIdx].blocks[rightIdx].type = COBBLESTONE;
						world->items[rightChunkIdx].blocks[rightIdx].fluid_level = NONE;
						world->items[rightChunkIdx].blocks[rightIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						world->items[rightChunkIdx].blocks[rightIdx].isBreak = false;
					}
				}
			}

			if(FlowLeft || FlowRight) continue;

			if(level >= MAX_FLUID_LEVEL) continue;

			//normal left
			if(leftChunkIdx  >= 0 && leftChunkIdx < world->capacity){

				int leftIdx = left + (y * CHUNK_WIDTH);
				Block leftBlock = world->items[leftChunkIdx].blocks[leftIdx];

				if(AIR == leftBlock.type){
					if(leftChunkIdx == chunkIdx){
						block_buffer[leftIdx].type = fluid;
						block_buffer[leftIdx].fluid_level = level + 1 > MAX_FLUID_LEVEL ?  MAX_FLUID_LEVEL : level + 1;
						block_buffer[leftIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
						block_buffer[leftIdx].isBreak = true;
					}else{
						world->items[leftChunkIdx].blocks[leftIdx].type = fluid;
						block_buffer[leftIdx].fluid_level = level + 1 > MAX_FLUID_LEVEL ?  MAX_FLUID_LEVEL : level + 1;
						world->items[leftChunkIdx].blocks[leftIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
						world->items[leftChunkIdx].blocks[leftIdx].isBreak = true;
					}	
				}else if(isLiquid(leftBlock.type) && leftBlock.type != fluid){
					if(leftChunkIdx == chunkIdx){
						block_buffer[leftIdx].type = fluid;
						block_buffer[leftIdx].fluid_level = NONE;
						block_buffer[leftIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						block_buffer[leftIdx].isBreak = false;
					}else{
						world->items[leftChunkIdx].blocks[leftIdx].type = fluid;
						world->items[leftChunkIdx].blocks[leftIdx].fluid_level = NONE;
						world->items[leftChunkIdx].blocks[leftIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						world->items[leftChunkIdx].blocks[leftIdx].isBreak = false;
					}	
				}
			}

			//normal right
			if(rightChunkIdx >= 0 && rightChunkIdx < world->capacity){

				int rightIdx = right + (y * CHUNK_WIDTH);
				Block rightBlock = world->items[rightChunkIdx].blocks[rightIdx];

				if(AIR == rightBlock.type){
					if(rightChunkIdx == chunkIdx){
						block_buffer[rightIdx].type = fluid;
						block_buffer[rightIdx].fluid_level = level + 1;
						block_buffer[rightIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
						block_buffer[rightIdx].isBreak = true;
					}else{
						world->items[rightChunkIdx].blocks[rightIdx].type = fluid;
						world->items[rightChunkIdx].blocks[rightIdx].fluid_level = level + 1;
						world->items[rightChunkIdx].blocks[rightIdx].fluid_parent = (FluidParent){chunkIdx, x, y};
						world->items[rightChunkIdx].blocks[rightIdx].isBreak = true;
					}	
				}else if(isLiquid(rightBlock.type) && rightBlock.type != fluid){
					if(rightChunkIdx == chunkIdx){
						block_buffer[rightIdx].type = COBBLESTONE;
						block_buffer[rightIdx].fluid_level = NONE;
						block_buffer[rightIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						block_buffer[rightIdx].isBreak = false;
					}else{
						world->items[rightChunkIdx].blocks[rightIdx].type = COBBLESTONE;
						world->items[rightChunkIdx].blocks[rightIdx].fluid_level = NONE;
						world->items[rightChunkIdx].blocks[rightIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						world->items[rightChunkIdx].blocks[rightIdx].isBreak = false;
					}	
				}
			}
		}
	}

	// update 
	for(int i = 0; i < size; i++){
		chunk->blocks[i].fluid_level  = block_buffer[i].fluid_level;
		chunk->blocks[i].fluid_parent = block_buffer[i].fluid_parent;
		chunk->blocks[i].type         = block_buffer[i].type;
		chunk->blocks[i].isBreak      = block_buffer[i].isBreak; 
	}
}

void ensure_capacity(World *world, int targetIdx) 
{
    if (targetIdx < 0) return;
    
    if (targetIdx >= world->capacity) {
        size_t oldCapacity = world->capacity;
        // Double until it fits 
        while (targetIdx >= world->capacity) {
            world->capacity *= 2;
        }
        
        world->items = realloc(world->items, sizeof(*world->items) * world->capacity);
        if (world->items == NULL) {
            perror("[WORLD ERROR]: Failed to Realloc\n");
            exit(1);
        }
        
        // Reset flags for the newly added memory block
        for (int i = oldCapacity; i < world->capacity; i++) {
            world->items[i].used = false;
        }
    }
}

void generate_structure(World *world, Chunk *chunk)
{
	if(chunk->structureGenerated) return;
	chunk->structureGenerated = true;

	int chunkIdx = chunk_index(chunk_coord(chunk->position.x));

	for(int x = 0; x < CHUNK_WIDTH; x++){
    	for(int y = 0; y < CHUNK_HEIGHT; y++){
    		int idx = getIndex(x, y);

    		int worldX = chunk->position.x + (x * BLOCK_WIDTH);
    		int worldY = chunk->position.y + (y * BLOCK_HEIGHT);

			float randVal = pseudo_random_2d(worldX, worldY);

    		if(GRASS == chunk->blocks[idx].type){
			    //trees on top of grass/surface
                if(randVal > 0.5f){
                	int treeHeight = GetRandomValue(3, 6); 
                	bool isEmpty = true;

                	for(int t = 1; t < treeHeight; t++){
	                	int treeIdx = getIndex(x, y-t);

                		Block target = chunk->blocks[treeIdx];
                		if(AIR != target.type && t < 3){
                			isEmpty = false;
                			break;
                		}
                	}

                	if(isEmpty){
		                for(int t = 1; t <= treeHeight; t++){
		                	if(y-t < 0 || y-t >= CHUNK_HEIGHT) continue;

		                	int treeIdx = getIndex(x, y-t);

		                	chunk->blocks[treeIdx].type = OAKLOG;
		                	chunk->blocks[treeIdx].isBreak = false;
		                	chunk->blocks[treeIdx].fluid_level = NONE;
		                	chunk->blocks[treeIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};

		                	if(t == treeHeight){
			                	chunk->blocks[treeIdx].type = OAKLEAF;

			                	if(y-t-1 >= 0 && y-t-1 < CHUNK_HEIGHT){
				                	int extra = getIndex(x, y-t-1);

				                	if(AIR == chunk->blocks[extra].type){
					                	chunk->blocks[extra].type = OAKLEAF;
					                	chunk->blocks[extra].isBreak = false;
					                	chunk->blocks[extra].fluid_level = NONE;
					                	chunk->blocks[extra].fluid_parent = (FluidParent){NONE, NONE, NONE};
					                }
				                }
		                	}

		                	if(t > (int)(treeHeight/2)){
		                		int left  = x - 1;
		                		int right = x + 1;


		                		int leftLeft   = x - 2;
		                		int rightRight = x + 2;


		                		if(left < 0){
		                			left += CHUNK_WIDTH;
		                			if(left >= 0 && left < CHUNK_WIDTH){
			                			int leftChunkIdx = getLeftChunkIndex(chunkIdx);

			                			if(leftChunkIdx >= 0 && leftChunkIdx < world->capacity){
				                			int leftIdx = getIndex(left, y-t);

				                			if(AIR == world->items[leftChunkIdx].blocks[leftIdx].type){
					                			world->items[leftChunkIdx].blocks[leftIdx].type    = OAKLEAF;
					                			world->items[leftChunkIdx].blocks[leftIdx].isBreak = false;
					                			world->items[leftChunkIdx].blocks[leftIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
					                			world->items[leftChunkIdx].blocks[leftIdx].fluid_level  = NONE;
					                		}
			                			}
			                		}
		                		}else{
			                		int leftIdx = getIndex(left, y-t);
		                			if(AIR == chunk->blocks[leftIdx].type){
					                	chunk->blocks[leftIdx].type = OAKLEAF;
					                	chunk->blocks[leftIdx].isBreak = false;
					                	chunk->blocks[leftIdx].fluid_level = NONE;
					                	chunk->blocks[leftIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
					                }
		                		}
		                		if(right >= CHUNK_WIDTH){
		                			right -= CHUNK_WIDTH;
		                			if(right >= 0 && right < CHUNK_WIDTH){
			                			int rightChunkIdx = getRightChunkIndex(chunkIdx);

			                			if(rightChunkIdx >= 0 && rightChunkIdx < world->capacity){
			                				int rightIdx = getIndex(right, y-t);

			                				if(AIR == world->items[rightChunkIdx].blocks[rightIdx].type){
					                			world->items[rightChunkIdx].blocks[rightIdx].type    = OAKLEAF;
					                			world->items[rightChunkIdx].blocks[rightIdx].isBreak = false;
					                			world->items[rightChunkIdx].blocks[rightIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
					                			world->items[rightChunkIdx].blocks[rightIdx].fluid_level  = NONE;
					                		}
				                		}
		                			}
		                		}else{
			                		int rightIdx = getIndex(right, y-t);

		                			if(AIR == chunk->blocks[rightIdx].type){
					                	chunk->blocks[rightIdx].type = OAKLEAF;
					                	chunk->blocks[rightIdx].isBreak = false;
					                	chunk->blocks[rightIdx].fluid_level = NONE;
					                	chunk->blocks[rightIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
			                		}
			                	}

			                	if(treeHeight > 4 && t < treeHeight){
				                	if(leftLeft >= 0){

				                		int leftLeftIdx = getIndex(leftLeft, y-t);
				                		if(AIR == chunk->blocks[leftLeftIdx].type){
						                	chunk->blocks[leftLeftIdx].type = OAKLEAF;
						                	chunk->blocks[leftLeftIdx].isBreak = false;
						                	chunk->blocks[leftLeftIdx].fluid_level = NONE;
						                	chunk->blocks[leftLeftIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
				                		}
				                	}else{
				                		leftLeft += CHUNK_WIDTH;

			                			if(leftLeft >= 0 && leftLeft < CHUNK_WIDTH){
				                			int leftChunkIdx = getLeftChunkIndex(chunkIdx);

				                			if(leftChunkIdx >= 0 && leftChunkIdx < world->capacity){
					                			int leftIdx = getIndex(leftLeft, y-t);

					                			if(AIR == world->items[leftChunkIdx].blocks[leftIdx].type){
						                			world->items[leftChunkIdx].blocks[leftIdx].type    = OAKLEAF;
						                			world->items[leftChunkIdx].blocks[leftIdx].isBreak = false;
						                			world->items[leftChunkIdx].blocks[leftIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						                			world->items[leftChunkIdx].blocks[leftIdx].fluid_level  = NONE;
						                		}
				                			}
				                		}
				                	}

				                	if(rightRight < CHUNK_WIDTH){

				                		int rightRightIdx = getIndex(rightRight, y-t);
				                		if(AIR == chunk->blocks[rightRightIdx].type){
						                	chunk->blocks[rightRightIdx].type = OAKLEAF;
						                	chunk->blocks[rightRightIdx].isBreak = false;
						                	chunk->blocks[rightRightIdx].fluid_level = NONE;
						                	chunk->blocks[rightRightIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						                }
				                	}else{
			                			rightRight -= CHUNK_WIDTH;
			                			if(rightRight >= 0 && rightRight < CHUNK_WIDTH){
				                			int rightChunkIdx = getRightChunkIndex(chunkIdx);

				                			if(rightChunkIdx >= 0 && rightChunkIdx < world->capacity){
				                				int rightIdx = getIndex(rightRight, y-t);

				                				if(AIR == world->items[rightChunkIdx].blocks[rightIdx].type){
						                			world->items[rightChunkIdx].blocks[rightIdx].type    = OAKLEAF;
						                			world->items[rightChunkIdx].blocks[rightIdx].isBreak = false;
						                			world->items[rightChunkIdx].blocks[rightIdx].fluid_parent = (FluidParent){NONE, NONE, NONE};
						                			world->items[rightChunkIdx].blocks[rightIdx].fluid_level  = NONE;
						                		}
					                		}
			                			}
			                		}
				                }
		                	}
		                }
		            }
	            }
    		}
    	}
    }
}

void initWorld(World* world)
{
	//pre-allocate the memory
	world->capacity = INIT_WORLD_SIZE*2;
	world->items = malloc(sizeof(*world->items)*world->capacity);

	if(world->items == NULL){
		perror("[WORLD INIT]: Failed to malloc\n");
		exit(1);
	}

	for(int i = 0; i < world->capacity; i++){
		world->items[i].used = false;	
	}

	for(int i = 0; i < INIT_WORLD_SIZE; i++){

		Vector2 chunk_position = {(i * BLOCK_WIDTH * CHUNK_WIDTH), -((CHUNK_HEIGHT * BLOCK_HEIGHT) / 2)};
		chunk_position.x -= ((int)(INIT_WORLD_SIZE/2)) * (BLOCK_WIDTH * CHUNK_WIDTH);

		int chunkIdx = chunk_index(chunk_coord(chunk_position.x));
		if(chunkIdx < 0 || chunkIdx >= INIT_WORLD_SIZE) continue;

		Chunk chunk;
		chunk.position = chunk_position;
		chunk.used = true;
		chunk.structureGenerated = false;

		init_chunk(&chunk);
		world->items[chunkIdx] = chunk;
		world->size++;
	}
}

void addChunk(World *world, int chunkCoordX)
{
	if(0 == world->size){
		initWorld(world);
		return;
	}
	int chunkIdx = chunk_index(chunkCoordX);

	if(chunkIdx < 0){return; } 

	ensure_capacity(world, chunkIdx);//if player somehow exceeds the capacity , just scale it

	if(!world->items[chunkIdx].used){
		float x = idxToPosition(chunkIdx);
		Chunk chunk;
		chunk.position = (Vector2){x, -((CHUNK_HEIGHT*BLOCK_HEIGHT)/2)};	
		chunk.used = true;
		chunk.structureGenerated = false;

		init_chunk(&chunk);

		world->items[chunkIdx] = chunk;
		world->size++;
	}

	for(int i = -(CHUNK_DISTANCE+1); i <= (CHUNK_DISTANCE+1); i++){
		if(i == 0) continue; //already checked

		chunkIdx = 	chunk_index(chunkCoordX + i);
		ensure_capacity(world, chunkIdx);

		if(chunkIdx >= 0 && chunkIdx < world->capacity){
			if(!world->items[chunkIdx].used){
				Chunk chunk;
				chunk.position = (Vector2){idxToPosition(chunkIdx), -((CHUNK_HEIGHT*BLOCK_HEIGHT)/2)};
				chunk.used = true;
				chunk.structureGenerated = false;

				init_chunk(&chunk);
				world->items[chunkIdx] = chunk;
				world->size++;
			}
		}
	}
}

void freeWorld(World *world)
{
	free(world->items);
}

void draw_World(World *world, int playerChunkCoordX)
{
	for(int i = -CHUNK_DISTANCE; i <= CHUNK_DISTANCE; i++){
		int chunkIdx = chunk_index(playerChunkCoordX + i);

		if(chunkIdx < 0 || chunkIdx >= world->capacity) continue;
		draw_chunk(&world->items[chunkIdx]);

		float x = world->items[chunkIdx].position.x;
		float y = world->items[chunkIdx].position.y;
		if(chunk_index(playerChunkCoordX) == chunk_index(chunk_coord(x))){
			// DrawRectangleLinesEx((Rectangle){x-CAMERA.x,y-CAMERA.y,CHUNK_WIDTH*BLOCK_WIDTH, CHUNK_HEIGHT*BLOCK_HEIGHT}, 3, BLUE);
		}else{
			// DrawRectangleLinesEx((Rectangle){x-CAMERA.x,y-CAMERA.y,CHUNK_WIDTH*BLOCK_WIDTH, CHUNK_HEIGHT*BLOCK_HEIGHT}, 3, WHITE);
		}
	}
}

Texture2D reSizeTexture(Texture2D texture, int width, int height)
{
	texture.width = width;
	texture.height = height;
	return texture;
}

void drawInventory(Player player, int slotIndex)
{
	float w = WINDOW_WIDTH/18;
	float h = w;
	float y = WINDOW_HEIGHT-h-20;
	for(int i = 0; i < INVENTORY_SLOT_COUNT; i++){
		BlockType type = player.inventory.type[i];
		float x = (4*w)+ (i * w);

		switch (type){
			case GRASS:
				DrawTexture(reSizeTexture(grassBlock,w,h), x , y, WHITE);
				break;
			case STONE:
				DrawTexture(reSizeTexture(stoneBlock,w,h), x , y, WHITE);
				break;
			case AIR:
				// DrawRectangle(x, y,w,h, GRAY);
				break;
			case WATER:
				DrawTexture(reSizeTexture(waterBlock,w,h), x , y, WHITE);
				break;
			case BEDROCK:
				DrawTexture(reSizeTexture(bedRock,w,h), x , y, WHITE);
				break;
			case LAVA:
				DrawTexture(reSizeTexture(lavaBlock,w,h), x , y, WHITE);
				break;
			case DIAMOND_ORE:
				DrawTexture(reSizeTexture(diamondOre,w,h), x , y, WHITE);
				break;
			case IRON_ORE:
				DrawTexture(reSizeTexture(ironOre,w,h), x , y, WHITE);
				break;
			case GOLD_ORE:
				DrawTexture(reSizeTexture(goldOre,w,h), x , y, WHITE);
				break;
			case EMERALD_ORE:
				DrawTexture(reSizeTexture(emeraldOre,w,h), x , y, WHITE);
				break;
			case COBBLESTONE:
				DrawTexture(reSizeTexture(cobbleStone,w,h), x , y, WHITE);
				break;
			case OBSIDIAN:
				DrawTexture(reSizeTexture(Obsidian,w,h), x , y, WHITE);
				break;
			case OAKLOG:
				DrawTexture(reSizeTexture(oakLog,w,h), x , y, WHITE);
				break;
			case OAKLEAF:
				DrawTexture(reSizeTexture(oakLeaf,w,h), x , y, WHITE);
				break;
			default:
				break;
		}
		if(i == slotIndex){
			DrawRectangleLinesEx((Rectangle){x, y, w, h}, 3, BLACK);
		}else{
			DrawRectangleLines(x,y,w, h, BLACK);
		}

		if(player.inventory.count[i] > 0){
			int font = w/3;
			char itemCount[50];
			sprintf(itemCount, "%d",player.inventory.count[i]);
			DrawText(itemCount,x+w-font,y+h-font,font, WHITE);
		}
	}
}

void getInventorySlotIndex(int* slotIndex)
{
	if(IsKeyPressed(KEY_ONE))   *slotIndex =  0;
	if(IsKeyPressed(KEY_TWO))   *slotIndex =  1;
	if(IsKeyPressed(KEY_THREE)) *slotIndex =  2;
	if(IsKeyPressed(KEY_FOUR))  *slotIndex =  3;
	if(IsKeyPressed(KEY_FIVE)) 	*slotIndex =  4;
	if(IsKeyPressed(KEY_SIX)) 	*slotIndex =  5;
	if(IsKeyPressed(KEY_SEVEN)) *slotIndex =  6;
	if(IsKeyPressed(KEY_EIGHT))	*slotIndex =  7;
	if(IsKeyPressed(KEY_NINE)) 	*slotIndex =  8;
}

void deleteItemInventory(Inventory* inventory, int slotIndex, BlockType type)
{
	if(slotIndex < 0 || slotIndex >= INVENTORY_SLOT_COUNT) return;	

	if(inventory->type[slotIndex] == type){
		if(inventory->count[slotIndex] > 1){
			inventory->count[slotIndex]--;
		}else{
			inventory->type[slotIndex] = AIR;
			inventory->count[slotIndex] = 0;
		}
	}
}

void addItem(DA_Item* item, BlockType type, Vector2 position)
{
	float vy = GetRandomValue(-100,-150);
	float vx = GetRandomValue(50,100);

	if(GetRandomValue(0,1)){
		vx *= -1;
	}

	Item newItem = {
		.type = type,
		.position = position,
		.velocity = (Vector2){vx, vy},
		.despawnTime = ITEM_DESPAWN_TIME
	};

	da_append(*item, newItem);
}

void updateItem(DA_Item* item, World world, float dt)
{
	Vector2 itemSize = {BLOCK_WIDTH/2, BLOCK_HEIGHT/2};

	for(int i = item->size - 1; i >= 0; i--){
		item->items[i].despawnTime -= dt;

		if(item->items[i].despawnTime <= 0){
			//replace it
			for(int j = i; j < item->size-1; j++){
				item->items[j] = item->items[j+1];
			}
			item->size--;
			continue;
		}

		item->items[i].position.x += item->items[i].velocity.x * dt;
		item->items[i].velocity.x *= 0.99f;

		Vector2 pos = item->items[i].position;

		if(ItemCollide(world, pos, itemSize)){
			item->items[i].position.x -= item->items[i].velocity.x * dt;
			item->items[i].velocity.x = 0;
		}

		item->items[i].velocity.y += GRAVITY * dt;
		item->items[i].position.y += item->items[i].velocity.y * dt;

		pos = item->items[i].position;

		if(ItemCollide(world, pos, itemSize)){
			item->items[i].position.y -= item->items[i].velocity.y * dt;

			if(item->items[i].velocity.y > 0){
				item->items[i].velocity.x = 0; //stop slipping
			}
			item->items[i].velocity.y = 0;
		}

		// Buried item Check (Block placed on top)
		Vector2 itemCenter = {
			item->items[i].position.x + (itemSize.x / 2.0f),
			item->items[i].position.y + (itemSize.y / 2.0f)
		};
		
		int chunkIdx = chunk_index(chunk_coord(itemCenter.x));
		if(chunkIdx >= 0 && chunkIdx < world.capacity){
			int lx = getLocalX(world, itemCenter.x);
			int ly = getLocalY(world, itemCenter.x, itemCenter.y);
			int idx = getIndex(lx, ly);

			if(idx >= 0 && idx < CHUNK_WIDTH * CHUNK_HEIGHT){
				// If item's center is  inside solid block:
				if(!world.items[chunkIdx].blocks[idx].isBreak){
					// Calculate world Y top of this block
					float chunk_world_y = world.items[chunkIdx].position.y; 
					float blockTopY = chunk_world_y + (ly * BLOCK_HEIGHT);
					
					// Only push up if the item is positioned below or inside the top line
					float targetY = blockTopY - itemSize.y;
					if (item->items[i].position.y >= targetY - 2.0f) {
						item->items[i].position.y = targetY;
						item->items[i].velocity.y = 0; // Kill downward velocity
					}
				}
			}
		}
	}
}

void drawItems(DA_Item item)
{
	float w = BLOCK_WIDTH/2; 
	float h = BLOCK_HEIGHT/2;

	for(int i = 0; i < item.size; i++){

		float x = item.items[i].position.x - CAMERA.x;
		float y = item.items[i].position.y - CAMERA.y;

		switch(item.items[i].type){
			case GRASS:
				DrawTexture(reSizeTexture(grassBlock,w,h), x , y, WHITE);
				break;
			case STONE:
				DrawTexture(reSizeTexture(stoneBlock,w,h), x , y, WHITE);
				break;
			case WATER:
				DrawTexture(reSizeTexture(waterBlock,w,h), x , y, WHITE);
				break;
			case LAVA:
				DrawTexture(reSizeTexture(lavaBlock,w,h), x , y, WHITE);
				break;
			case DIAMOND_ORE:
				DrawTexture(reSizeTexture(diamondOre,w,h), x , y, WHITE);
				break;
			case IRON_ORE:
				DrawTexture(reSizeTexture(ironOre,w,h), x , y, WHITE);
				break;
			case GOLD_ORE:
				DrawTexture(reSizeTexture(goldOre,w,h), x , y, WHITE);
				break;
			case EMERALD_ORE:
				DrawTexture(reSizeTexture(emeraldOre,w,h), x , y, WHITE);
				break;
			case COBBLESTONE:
				DrawTexture(reSizeTexture(cobbleStone,w,h), x , y, WHITE);
				break;
			case OBSIDIAN:
				DrawTexture(reSizeTexture(Obsidian,w,h), x , y, WHITE);
				break;
			case OAKLOG:
				DrawTexture(reSizeTexture(oakLog,w,h), x , y, WHITE);
				break;
			case OAKLEAF:
				DrawTexture(reSizeTexture(oakLeaf,w,h), x , y, WHITE);
				break;
			default:
				break;
		}	
		DrawRectangleLines(x, y, w, h, BLACK);
	}
}

bool ItemCollide(World world, Vector2 itemPos, Vector2 itemSize)
{
    // Shrink the bounding box slightly (by 2 pixels) to prevent false corner/edge snags
    float inset = 2.0f;
    float left = itemPos.x + inset;
    float right = itemPos.x + itemSize.x - inset;
    float top = itemPos.y + inset;
    float bottom = itemPos.y + itemSize.y - inset;

    int minX = floor(left / BLOCK_WIDTH);
    int maxX = floor(right / BLOCK_WIDTH);
    int minY = floor(top / BLOCK_HEIGHT);
    int maxY = floor(bottom / BLOCK_HEIGHT);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int chunkIdx = chunk_index(chunk_coord(x * BLOCK_WIDTH));   
            if (chunkIdx < 0 || chunkIdx >= world.capacity) continue;

            Vector2 chunk_pos = world.items[chunkIdx].position;
            int localX = x - (int)(chunk_pos.x / BLOCK_WIDTH);
            int localY = y - (int)(chunk_pos.y / BLOCK_HEIGHT);

            if (localX < 0 || localX >= CHUNK_WIDTH) continue;
            if (localY < 0 || localY >= CHUNK_HEIGHT) continue;

            if (!world.items[chunkIdx].blocks[getIndex(localX, localY)].isBreak) {
                return true; // Solid block hit
            }
        }
    }
    return false;
}

bool addItemToInventory(Player* player, Item item, int slotIndex)
{
	if(slotIndex >= 0 && slotIndex < INVENTORY_SLOT_COUNT){
		if(player->inventory.type[slotIndex] == AIR){
			player->inventory.type[slotIndex] = item.type;
			player->inventory.count[slotIndex] = 1;
			return true;
		}
		if(player->inventory.type[slotIndex] == item.type && player->inventory.count[slotIndex] < ITEM_STACK_COUNT){
			player->inventory.count[slotIndex]++;
			return true;
		}
	}

	for(int i = 0; i < INVENTORY_SLOT_COUNT; i++){
		if(player->inventory.type[i] == AIR){
			player->inventory.type[i] = item.type;	
			player->inventory.count[i] = 1;
			return true;
		}

		if(player->inventory.type[i] == item.type && player->inventory.count[i] < ITEM_STACK_COUNT){
			player->inventory.count[i]++;
			return true;
		}
	}
	return false;
}

void pickItem(Player *player, DA_Item* item, int slotIndex)
{
	float pickUpRadius = 48.0f;

	Vector2 playerCenter = {
		player->position.x + (player->size.x/2.0f),
		player->position.y + (player->size.y/2.0f)
	};

	Vector2 itemSize = {BLOCK_WIDTH/2, BLOCK_HEIGHT/2};

	for(int i = item->size-1; i >= 0; i--){

		Vector2 itemCenter = {
            item->items[i].position.x + (itemSize.x/2),
            item->items[i].position.y +	(itemSize.y/2) 
        };

        float dx = playerCenter.x - itemCenter.x;
        float dy = playerCenter.y - itemCenter.y;

        float dist_sq = (dx*dx) + (dy*dy);

        if(dist_sq <= pickUpRadius*pickUpRadius){

        	bool added = addItemToInventory(player, item->items[i], slotIndex);

        	if(added){
        		PlaySound(itemPick);

        		for(int j = i; j < item->size-1; j++){
        			item->items[j] = item->items[j+1];
        		}
        		item->size--;
        	}
        }
	}
}
