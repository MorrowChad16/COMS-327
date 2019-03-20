#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <endian.h>
#include <limits.h>
#include <unistd.h>

#include "heap.h"

#define MAX_ROOMS 10
#define MIN_X_ROOM 4
#define MAX_MINUS_MIN_X 10
#define MIN_Y_ROOM 3
#define MAX_MINUS_MIN_Y 11
#define WINDOW_Y 24
#define WINDOW_X 80
#define MAX_CONSTRAINTS 4
#define TRUE 1
#define FALSE 0
#define GAME_HEIGHT 21
#define GAME_WIDTH 80
#define X_LOC 0
#define Y_LOC 1
#define X_LEN 2
#define Y_LEN 3
#define NUM_MONSTERS 10
#define MAX_MONSTERS 21
#define min(x, y) (x < y ? x : y)

//defines user actions when running game.c
typedef enum action{
	save,
	load,
	load_save,
	num_mon
} user_action;

typedef struct Cell {
  heap_node_t *hn;
  uint8_t pos[2];
  int32_t cost;
} Cell_t;

typedef struct PC{
	//add pc specific characteristics later
} PC_t;

typedef struct NPC{
	int i; //intelligence
	int t; //telepathy
	int tu; //tunneling ability
	int e; //erratic
	int pcLoc[2]; //last known pc-location for intelligent monsters
	int cn; //character number
	char lv; //holds last value AKA value it is replacing while still
} NPC_t;

typedef struct Character {
	PC_t *PC; //tells us the character is a pc
	NPC_t *NPC; //tells us the character is a pc
	int s; //speed PC:10, MONST: 5-20
	int i; //intelligence
	int t; //telepathy
	int tu; //tunneling ability
	int e; //erratic
	int pcLoc[2]; //last known pc-location for intelligent monsters
	char c; //Symbol for the character
	int a; //tells whether character is alive or not
	int pos[2]; //position of character
	int nt; //next turn value for priority queue
	int sn; //sequence number for priority queue
	heap_node_t *hn; //heap node for priority queue
	char lv; //holds last value AKA value it is replacing while still
} Character_t;

void generateDungeon(char dungeonArray[WINDOW_Y][WINDOW_X], int hardnessArray[GAME_HEIGHT][GAME_WIDTH]){
	int i, j;
	int loadingDungeon = FALSE;
	if(hardnessArray[0][0] != 0){
		loadingDungeon = TRUE;
	}
	//Generates the window array of 21 x 80
	for(i = 0; i < WINDOW_Y; i++){
		for(j = 0; j < WINDOW_X; j++){
			//Ceiling or Floor = immutable rock
			if(i == 0 || i == GAME_HEIGHT - 1){
				dungeonArray[i][j] = '-';
				hardnessArray[i][j] = 255;
			//Left or Right Wall = immutable rock
			} else if((j == 0 || j == GAME_WIDTH - 1) && i < GAME_HEIGHT - 1){
				dungeonArray[i][j] = '|';
				hardnessArray[i][j] = 255;
			//Everything inside this range has a random mutable rock range from [1, 254]
			} else if(j < GAME_WIDTH - 1 && i < GAME_HEIGHT - 1) {
				dungeonArray[i][j] = ' ';
				if(!loadingDungeon){
					hardnessArray[i][j] = 1 + rand() % 253;
				}
			//Bottom 3 rows for displaying messages has no hardness and is set to an empty cell
			} else {
				dungeonArray[i][j] = ' ';
			}
		}
	}
}

void generateRooms(int roomsArray[MAX_ROOMS][MAX_CONSTRAINTS], char dungeonArray[WINDOW_Y][WINDOW_X], int hardnessArray[GAME_HEIGHT][GAME_WIDTH], int *numRooms){
	int totalArea = 0;
	int i, j;
	int roomBuffer = *numRooms;
	int loadingDungeon = FALSE;
	if(*numRooms != 0){
		loadingDungeon = TRUE;
	}
	
	//Min: 4x by 3y
	//OWN MAX: 14x by 14y
	//103 is about 7% of a 78x19 area, which is the hard rock area available for placement
	while((((*numRooms - roomBuffer) < *numRooms && loadingDungeon == TRUE) || ((*numRooms - roomBuffer) < 6 && loadingDungeon == FALSE))  && totalArea < 103){
		if(loadingDungeon == FALSE){
			roomsArray[*numRooms][X_LOC] = 1 + rand() % GAME_WIDTH - 2; //x-position
			roomsArray[*numRooms][Y_LOC] = 1 + rand() % GAME_HEIGHT - 2; //y-position
			roomsArray[*numRooms][X_LEN] = MIN_X_ROOM + (rand() % MAX_MINUS_MIN_X); //x-length
			roomsArray[*numRooms][Y_LEN] = MIN_Y_ROOM + (rand() % MAX_MINUS_MIN_Y); //y-length
		}
		
		/*
		boolean int to check if room is occupied before placing it
		0: NOT OCCUPIED
		1: OCCUPIED
		*/
		int isOccupied = FALSE;
		
		//Checks if area is occupied before pasting
		//Checks ONE extra Y and X position to keep at least 1 white space between rooms
		if(loadingDungeon == FALSE){
			for(i = -1; i < roomsArray[*numRooms - roomBuffer][Y_LEN] + 1; i++){
				for(j = -1; j < roomsArray[*numRooms - roomBuffer][X_LEN] + 1; j++){
					if(!hardnessArray[roomsArray[*numRooms - roomBuffer][Y_LOC] + i][roomsArray[*numRooms - roomBuffer][X_LOC] + j] 
						|| hardnessArray[roomsArray[*numRooms - roomBuffer][Y_LOC] + i][roomsArray[*numRooms - roomBuffer][X_LOC] + j] == 255){
						isOccupied = TRUE;
						break;
					}
				}
			}
		}
				
		//pastes room in dungeon
		if(isOccupied == FALSE){
			for(i = 0; i < roomsArray[*numRooms - roomBuffer][Y_LEN]; i++){
				for(j = 0; j < roomsArray[*numRooms - roomBuffer][X_LEN]; j++){
					//Sets the cell value to '.' to represent a segment of the room
					dungeonArray[roomsArray[*numRooms - roomBuffer][Y_LOC] + i][roomsArray[*numRooms - roomBuffer][X_LOC] + j] = '.';
					if(loadingDungeon == FALSE){
						//Sets hardness to 0 for every cell within the room
						hardnessArray[roomsArray[*numRooms - roomBuffer][Y_LOC] + i][roomsArray[*numRooms - roomBuffer][X_LOC] + j] = 0;
					}
				}
			}
		}
		
		if(isOccupied == FALSE && loadingDungeon == FALSE){
			*numRooms = *numRooms + 1;
			totalArea += (roomsArray[*numRooms][Y_LEN] * roomsArray[*numRooms][X_LEN]);
		}
		if(roomBuffer != 0){
			roomBuffer--;
		}
	}
}

void generateCorridors(char dungeon[WINDOW_Y][WINDOW_X], int rooms[MAX_ROOMS][MAX_CONSTRAINTS], int *numRooms, int hardnessArray[GAME_HEIGHT][GAME_WIDTH], int *number_of_upstairs){
	int i, j;
	int roomNumber = 0;
	int loadingDungeon = FALSE;
	if(*number_of_upstairs != 0){
		loadingDungeon = TRUE;
	}
	
	//If were not loading in a dungeon, attach rooms with corridors
	if(loadingDungeon == FALSE){
		// Iterate through rooms array to connect the rooms
		while(roomNumber < *numRooms - 1){
			//Direction to travel: X1-X0/|X1-X0|
			//avoid dividing by 0
			int stepOverDirectionX = 0;
			int stepOverDirectionY = 0;
			if(abs(rooms[roomNumber + 1][X_LOC] - rooms[roomNumber][X_LOC])){
				stepOverDirectionX = ((rooms[roomNumber + 1][X_LOC] - rooms[roomNumber][X_LOC]) / (abs(rooms[roomNumber + 1][X_LOC] - rooms[roomNumber][X_LOC])));
			}
			if(abs(rooms[roomNumber + 1][Y_LOC] - rooms[roomNumber][Y_LOC])){
				stepOverDirectionY = ((rooms[roomNumber + 1][Y_LOC] - rooms[roomNumber][Y_LOC]) / (abs(rooms[roomNumber + 1][Y_LOC] - rooms[roomNumber][Y_LOC])));
			}
			
			int currentX = rooms[roomNumber][X_LOC];
			int currentY = rooms[roomNumber][Y_LOC];
			
			// Places # characters from the first room X to the corner X
			while(currentX != rooms[roomNumber + 1][X_LOC]){
				if(hardnessArray[currentY][currentX]){
					dungeon[currentY][currentX] = '#';
					//Sets hardness to 0 to represent an open cell
					hardnessArray[currentY][currentX] = 0;
				}
				currentX += stepOverDirectionX;
			}
			
			//Places # characters from the corner Y to the second room Y
			while(currentY != rooms[roomNumber + 1][Y_LOC]){
				if(hardnessArray[currentY][currentX]){
					dungeon[currentY][currentX] = '#';
					//Sets hardness to 0 to represent an open cell
					hardnessArray[currentY][currentX] = 0;
				}
				currentY += stepOverDirectionY;
			}
			roomNumber++;
		}
	} else {
		for(i = 0; i < GAME_HEIGHT; i++){
			for(j = 0; j < GAME_WIDTH; j++){
				if(!hardnessArray[i][j] && dungeon[i][j] != '.' && dungeon[i][j] != '@'){
					dungeon[i][j] = '#';
				}
			}
		}
	}
}

void generateStairs(char dungeon[WINDOW_Y][WINDOW_X], int hardnessArray[GAME_HEIGHT][GAME_WIDTH], int upwardCases[MAX_ROOMS][2], int downwardCases[MAX_ROOMS][2], int *numUpCases, int *numDownCases, int pc_location[2]){
	int i, randomX, randomY;
	
	/*
	Boolean int to track whether the pc was randomly placed within a room or corridor
	*/
	int pcPlaced = FALSE;	
	/*
	Boolean int to track whether we're reading in loaded data 
	or creating new data.
	TRUE: loading data
	FALSE: creating new data
	*/
	int dungeonLoading = FALSE;
	//Checks if both values are empty or not. if empty then we create new data, otherwise load in.
	if((*numUpCases) && (*numDownCases)){
		dungeonLoading = TRUE;
	}
	
	if(dungeonLoading == FALSE){
		while(!*numUpCases){
			randomY = 1 + rand() % GAME_HEIGHT - 1;
			randomX = 1 + rand() % GAME_WIDTH - 2;
			if(!hardnessArray[randomY][randomX] && !*numUpCases && dungeon[randomY][randomX] != '>'){
				dungeon[randomY][randomX] = '<';
				upwardCases[*numUpCases][X_LOC] = randomX;
				upwardCases[*numUpCases][Y_LOC] = randomY;
				*numUpCases = *numUpCases + 1;
			}
		}
			
		while(!*numDownCases){
			randomY = 1 + rand() % GAME_HEIGHT - 1;
			randomX = 1 + rand() % GAME_WIDTH - 2;
			if(!hardnessArray[randomY][randomX] && !*numDownCases && dungeon[randomY][randomX] != '<'){
				dungeon[randomY][randomX] = '>';
				downwardCases[*numDownCases][X_LOC] = randomX;
				downwardCases[*numDownCases][Y_LOC] = randomY;
				*numDownCases = *numDownCases + 1;
			}
		}
		
		while(!pcPlaced){
			randomY = 1 + rand() % GAME_HEIGHT - 1;
			randomX = 1 + rand() % GAME_WIDTH - 2;
			if(!hardnessArray[randomY][randomX] && dungeon[randomY][randomX] != '>' && dungeon[randomY][randomX] != '<'){
				dungeon[randomY][randomX] = '@';
				pc_location[Y_LOC] = randomY;
				pc_location[X_LOC] = randomX;
				pcPlaced = TRUE;
			}
		}
		
	} else{
		for(i = 0; i < *numUpCases; i++){
			dungeon[upwardCases[i][Y_LOC]][upwardCases[i][X_LOC]] = '<';
		}
		for(i = 0; i < *numDownCases; i++){
			dungeon[downwardCases[i][Y_LOC]][downwardCases[i][X_LOC]] = '>';
		}
		dungeon[pc_location[Y_LOC]][pc_location[X_LOC]] = '@';
	}
}

void printDungeon(char dungeon[WINDOW_Y][WINDOW_X]){ //, int hardnessArray[GAME_HEIGHT][GAME_WIDTH]
	int i, j;
	//prints seed
	int seed = rand();
	printf("Seed: %d\n", seed);
	//prints final array of game area and components
	for(i = 0; i < WINDOW_Y; i++){
		for(j = 0; j < WINDOW_X; j++){
			printf("%c", dungeon[i][j]);
		}
		printf("\n");
	}
	
	// for(i = 0; i < GAME_HEIGHT; i++){
		// for(j = 0; j < GAME_WIDTH; j++){
			// printf("%d\n", hardnessArray[10][j]);
		// }
	// }
}

void loadDungeon(int hardness[GAME_HEIGHT][GAME_WIDTH], char *directory, int pc[2], int *number_of_rooms, int upwardCases[MAX_ROOMS][2], int downwardCases[MAX_ROOMS][2], int *number_of_upstairs, int *number_of_downstairs, int rooms[MAX_ROOMS][MAX_CONSTRAINTS]){
	FILE *file;
	if((file = fopen(directory, "rb")) == NULL){
		fprintf(stderr, "FAILED TO OPEN FILE: %s\n", directory);
		return;
	}
	
	//reads semantic file-type marker
	fseek(file, 0, SEEK_SET);
	char fileType[13];
	fread(fileType, sizeof(fileType), 1, file);
	
	//reads unsigned file version with value of 0
	fseek(file, 12, SEEK_SET);
	u_int32_t fileVersion;
	fread(&fileVersion, sizeof(fileVersion), 1, file);
	u_int32_t fileVersion_two = be32toh(fileVersion);
	
	//reads unsigned integer size of file
	fseek(file, 16, SEEK_SET);
	u_int32_t fileSize;
	fread(&fileSize, sizeof(fileSize), 1, file);
	u_int32_t fileSize_two = be32toh(fileSize);
	
	//Reads PC coordinates
	fseek(file, 20, SEEK_SET);
	int pos = 20;
	int i;
	for(i = 0; i < 2; i++){
		fseek(file, pos + i, SEEK_SET);
		u_int8_t PC_Location; 
		fread(&PC_Location, sizeof(PC_Location), 1, file);
		pc[i] = PC_Location;
	}
	
	//Reads in matrix with hardness
	fseek(file, 22, SEEK_SET);
	u_int8_t copyOfHardness;
	int j;
	for(i = 0; i < GAME_HEIGHT - 1; i++){
		for(j = 0; j < GAME_WIDTH - 1; j++){
			fread(&copyOfHardness, sizeof(copyOfHardness), 1, file);
			hardness[i][j] = copyOfHardness;
		}
	}
	
	//reads in number of rooms
	fseek(file, 1702, SEEK_SET);
	u_int16_t numberOfRooms;
	fread(&numberOfRooms, 1702, 1, file);
	u_int16_t numberOfRooms_two = be16toh(numberOfRooms);
	*number_of_rooms = numberOfRooms_two;
	
	//reads room position and size
	fseek(file, 1704, SEEK_SET);
	pos = 1704;
	u_int8_t roomSpecs;
	for(i = 0; i < *number_of_rooms; i++){
		for(j = 0; j < MAX_CONSTRAINTS; j++){
			fseek(file, pos, SEEK_SET);
			fread(&roomSpecs, sizeof(roomSpecs), 1, file);
			rooms[i][j] = roomSpecs;
			pos++;
		}
	}
	
	//reads number of upward stairs
	fseek(file, 1704 + (*number_of_rooms * 4), SEEK_SET);
	u_int16_t numUp;
	fread(&numUp, sizeof(numUp), 1, file);
	u_int16_t numUp_two = be16toh(numUp);
	*number_of_upstairs = numUp_two;
	
	//reads coordinates of staircases
	fseek(file, 1706 + (*number_of_rooms * 4), SEEK_SET);
	pos = 1706 + (*number_of_rooms * 4);
	u_int8_t upward;
	for(i = 0; i < *number_of_upstairs; i++){
		for(j = 0; j < 2; j++){
			fseek(file, pos, SEEK_SET);
			fread(&upward, sizeof(upward), 1, file);
			upwardCases[i][j] = upward;
			pos++;
		}
	}
	
	//reads number of downward stairs
	fseek(file, 1706 + (*number_of_rooms * 4) + (*number_of_upstairs * 2), SEEK_SET);
	u_int16_t numDown;
	fread(&numDown, sizeof(numDown), 1, file);
	u_int16_t numDown_two = be16toh(numDown);
	*number_of_downstairs = numDown_two;
	
	//reads coordinates of staircases
	fseek(file, 1708 + (*number_of_rooms * 4) + (*number_of_upstairs * 2), SEEK_SET);
	pos = 1708 + (*number_of_rooms * 4) + (*number_of_upstairs * 2);
	u_int8_t downward;
	for(i = 0; i < *number_of_downstairs; i++){
		for(j = 0; j < 2; j++){
			fseek(file, pos, SEEK_SET);
			fread(&downward, sizeof(downward), 1, file);
			downwardCases[i][j] = downward;
			pos++;
		}
	}
	
	fclose(file);
}

void saveDungeon(int hardness[GAME_HEIGHT][GAME_WIDTH], char *directory, int pc[2], int *number_of_rooms, int upwardCases[MAX_ROOMS][2], int downwardCases[MAX_ROOMS][2], int *number_of_upstairs, int *number_of_downstairs, int rooms[MAX_ROOMS][MAX_CONSTRAINTS]){
	FILE *file;
	
	if((file = fopen(directory, "wb")) == NULL){
		fprintf(stderr, "FAILED TO OPEN FILE: %s\n", directory);
		return;
	}
	
	//write file marker
	fseek(file, 0, SEEK_SET);
	char fileMarker[13];
	strcpy(fileMarker, "RLG327-S2019");
	fwrite(fileMarker, sizeof(char), 12, file);

	//Writes unsigned 32-bit file version marker
	fseek(file, 12, SEEK_SET);
	u_int32_t fileVersionMarker = 0;
	u_int32_t fileVersionMarker_two = htobe32(fileVersionMarker);
	fwrite(&fileVersionMarker_two, sizeof(fileVersionMarker_two), 1, file);
	
	//Writes unsigned 32-bit size of file
	fseek(file, 16, SEEK_SET);
	u_int32_t fileSizeMarker = 1708 + (*number_of_rooms * 4) + (*number_of_upstairs * 2) + (*number_of_downstairs * 2);
	u_int32_t fileSizeMarker_two = htobe32(fileSizeMarker);
	fwrite(&fileSizeMarker_two, sizeof(fileSizeMarker_two), 1, file);
	
	//Writes PC location
	fseek(file, 20, SEEK_SET);
	int i;
	int pos = 20;
	u_int8_t pcLocation;
	for(i = 0; i < 2; i++){
		fseek(file, pos + i, SEEK_SET);
		pcLocation = pc[i];
		fwrite(&pcLocation, sizeof(pcLocation), 1, file);
	}
	
	//writes dungeon hardness for every map
	fseek(file, 22, SEEK_SET);
	pos = 22;
	u_int8_t copyOfHardness;
	int j;
	for(i = 0; i < GAME_HEIGHT - 1; i++){
		for(j = 0; j < GAME_WIDTH - 1; j++){
			fseek(file, pos, SEEK_SET);
			copyOfHardness = hardness[i][j];
			fwrite(&copyOfHardness, sizeof(copyOfHardness), 1, file);
			pos++;
		}
	}
	
	//writes number of rooms
	fseek(file, 1702, SEEK_SET);
	u_int16_t numbRooms = *number_of_rooms;
	u_int16_t numbRooms_two = htobe16(numbRooms);
	fwrite(&numbRooms_two, sizeof(numbRooms_two), 1, file);
	
	//writes rooms specifications
	fseek(file, 1704, SEEK_SET);
	pos = 1704;
	u_int8_t roomSpecs;
	for(i = 0; i < *number_of_rooms; i++){
		for(j = 0; j < MAX_CONSTRAINTS; j++){
			fseek(file, pos, SEEK_SET);
			roomSpecs = rooms[i][j];
			fwrite(&roomSpecs, sizeof(roomSpecs), 1, file);
			pos++;
		}
	}
	
	//Writes number of upward staircases
	fseek(file, 1704 + (*number_of_rooms * 4), SEEK_SET);
	u_int16_t numbUpCases = *number_of_upstairs;
	u_int16_t numbUpCases_two = htobe16(numbUpCases);
	fwrite(&numbUpCases_two, sizeof(numbUpCases_two), 1, file);
	
	//Writes upward cases coordinates
	fseek(file, 1706 + (*number_of_rooms * 4), SEEK_SET);
	pos = 1706 + (*number_of_rooms * 4);
	u_int8_t upwardCoordinates;
	for(i = 0; i < *number_of_upstairs; i++){
		for(j = 0; j < 2; j++){
			fseek(file, pos, SEEK_SET);
			upwardCoordinates = upwardCases[i][j];
			fwrite(&upwardCoordinates, sizeof(upwardCoordinates), 1, file);
			pos++;
		}
	}
	
	//Writes number of downward staircases
	fseek(file, 1706 + (*number_of_rooms * 4) + (*number_of_upstairs * 2), SEEK_SET);
	u_int16_t numbDownCases = *number_of_downstairs;
	u_int16_t numbDownCases_two = htobe16(numbDownCases);
	fwrite(&numbDownCases_two, sizeof(numbDownCases_two), 1, file);
	
	//Writes downward cases coordinates
	fseek(file, 1708 + (*number_of_rooms * 4) + (*number_of_upstairs * 2), SEEK_SET);
	pos = 1708 + (*number_of_rooms * 4) + (*number_of_upstairs * 2);
	u_int8_t downwardCoordinates;
	for(i = 0; i < *number_of_downstairs; i++){
		for(j = 0; j < 2; j++){
			fseek(file, pos, SEEK_SET);
			downwardCoordinates = downwardCases[i][j];
			fwrite(&downwardCoordinates, sizeof(downwardCoordinates), 1, file);
			pos++;
		}
	}
	
	fclose(file);
	
}

//comparitor for Cell_t type pieces
static int32_t corridor_path_cmp(const void *key, const void *with) {
  return ((Cell_t *) key)->cost - ((Cell_t *) with)->cost;
}

//generates heat map for open space
void roomHeatMapGenerator(int pc[2], int roomMap[GAME_HEIGHT][GAME_WIDTH], int hardness[GAME_HEIGHT][GAME_WIDTH]){
	static Cell_t map[GAME_HEIGHT][GAME_WIDTH], *p;
	static uint32_t initialized = 0;
	heap_t h;
	uint32_t x, y;
	
	//Creates the map if not already initialized
	if (!initialized) {
		for (y = 0; y < GAME_HEIGHT; y++) {
			for (x = 0; x < GAME_WIDTH; x++) {
				//Backwards. need to fix issue throughout code
				map[y][x].pos[X_LOC] = x;
				map[y][x].pos[Y_LOC] = y;
			}
		}
		initialized = TRUE;	
	}
	
	//Sets values in ROOM HEAT MAP to INFINITY (essentially)
	for(y = 0; y < GAME_HEIGHT; y++){
		for(x = 0; x < GAME_WIDTH; x++){
			map[y][x].cost = INT_MAX;
		}
	}
	
	//intialize heap
	heap_init(&h, corridor_path_cmp, NULL);
	//Sets PC cost to 0
	map[pc[Y_LOC]][pc[X_LOC]].cost = 0;
	
	//Reads in every point in the dungeon
	for (y = 0; y < GAME_HEIGHT; y++) {
		for (x = 0; x < GAME_WIDTH; x++) {
			if (hardness[y][x] != 255) {
				map[y][x].hn = heap_insert(&h, &map[y][x]);
			} else {
				map[y][x].hn = NULL;
			}
		}
	}
	
	//Pulls node out of heap until every node has been analyzed
	//0: y-coordinate
	//1: x-coordinate
	while(p = heap_remove_min(&h)){
		p->hn = NULL;
		
		int extraCost;
		if(!hardness[p->pos[Y_LOC]][p->pos[X_LOC]]){
			extraCost = 1;
		} else {
			extraCost = INT_MAX;
		}
	
		//TOP LEFT
		if(map[p->pos[Y_LOC] - 1][p->pos[X_LOC] - 1].hn && map[p->pos[Y_LOC] - 1][p->pos[X_LOC] -1].cost > p->cost + extraCost && hardness[p->pos[Y_LOC] - 1][p->pos[X_LOC] - 1] == 0){
			map[p->pos[Y_LOC] - 1][p->pos[X_LOC] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[Y_LOC] - 1][p->pos[X_LOC] - 1].hn);
		}
		
		//LEFT
		if(map[p->pos[Y_LOC]][p->pos[X_LOC] - 1].hn && map[p->pos[Y_LOC]][p->pos[X_LOC] -1].cost > p->cost + extraCost && hardness[p->pos[Y_LOC]][p->pos[X_LOC] - 1] == 0){
			map[p->pos[Y_LOC]][p->pos[X_LOC] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[Y_LOC]][p->pos[X_LOC] - 1].hn);
		}
		
		//BOTTOM LEFT
		if(map[p->pos[Y_LOC] + 1][p->pos[X_LOC] - 1].hn && map[p->pos[Y_LOC] + 1][p->pos[X_LOC] -1].cost > p->cost + extraCost && hardness[p->pos[Y_LOC] + 1][p->pos[X_LOC] - 1] == 0){
			map[p->pos[Y_LOC] + 1][p->pos[X_LOC] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[Y_LOC] + 1][p->pos[X_LOC] - 1].hn);
		}
		
		//BOTTOM
		if(map[p->pos[Y_LOC] + 1][p->pos[X_LOC]].hn && map[p->pos[Y_LOC] + 1][p->pos[X_LOC]].cost > p->cost + extraCost && hardness[p->pos[Y_LOC] + 1][p->pos[X_LOC]] == 0){
			map[p->pos[Y_LOC] + 1][p->pos[X_LOC]].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[Y_LOC] + 1][p->pos[X_LOC]].hn);
		}
		
		//BOTTOM RIGHT
		if(map[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1].hn && map[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1].cost > p->cost + extraCost && hardness[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1] == 0){
			map[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1].hn);
		}
		
		//RIGHT
		if(map[p->pos[Y_LOC]][p->pos[X_LOC] + 1].hn && map[p->pos[Y_LOC]][p->pos[X_LOC] + 1].cost > p->cost + extraCost && hardness[p->pos[Y_LOC]][p->pos[X_LOC] + 1] == 0){
			map[p->pos[Y_LOC]][p->pos[X_LOC] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[Y_LOC]][p->pos[X_LOC] + 1].hn);
		}
		
		//TOP RIGHT
		if(map[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1].hn && map[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1].cost > p->cost + extraCost && hardness[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1] == 0){
			map[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1].hn);
		}
		
		//TOP
		if(map[p->pos[Y_LOC] - 1][p->pos[X_LOC]].hn && map[p->pos[Y_LOC] - 1][p->pos[X_LOC]].cost > p->cost + extraCost && hardness[p->pos[Y_LOC] - 1][p->pos[X_LOC]] == 0){
			map[p->pos[Y_LOC] - 1][p->pos[X_LOC]].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[Y_LOC] - 1][p->pos[X_LOC]].hn);
		}
	}
	
	for(y = 0; y < GAME_HEIGHT; y++){
		for(x = 0; x < GAME_WIDTH; x++){
			roomMap[y][x] = map[y][x].cost;
		}
	}
}

//generates heat map for the whole dungeon
void wholeHeatMapGenerator(int pc[2], int wholeMap[GAME_HEIGHT][GAME_WIDTH], int hardness[GAME_HEIGHT][GAME_WIDTH]){
	static Cell_t wMap[GAME_HEIGHT][GAME_WIDTH], *p;
	static uint32_t initialized = 0;
	heap_t h;
	uint32_t x, y;
	
	//Creates the map if not already initialized
	if (!initialized) {
		for (y = 0; y < GAME_HEIGHT; y++) {
			for (x = 0; x < GAME_WIDTH; x++) {
				wMap[y][x].pos[X_LOC] = x;
				wMap[y][x].pos[Y_LOC] = y;
			}
		}
		initialized = TRUE;	
	}
	
	//Sets values in WHOLE HEAT MAP to INFINITY (essentially)
	for(y = 0; y < GAME_HEIGHT; y++){
		for(x = 0; x < GAME_WIDTH; x++){
			wMap[y][x].cost = INT_MAX;
		}
	}
	
	heap_init(&h, corridor_path_cmp, NULL);
	//Sets PC cost to 0
	wMap[pc[Y_LOC]][pc[X_LOC]].cost = 0;
	
	//Reads in every point in the dungeon
	for (y = 0; y < GAME_HEIGHT; y++) {
		for (x = 0; x < GAME_WIDTH; x++) {
			if (hardness[y][x] != 255) {
				wMap[y][x].hn = heap_insert(&h, &wMap[y][x]);
			} else {
				wMap[y][x].hn = NULL;
			}
		}
	}
	
	while(p = heap_remove_min(&h)){
		p->hn = NULL;
		
		int extraCost;
		if(hardness[p->pos[Y_LOC]][p->pos[X_LOC]] >= 0 && hardness[p->pos[Y_LOC]][p->pos[X_LOC]] < 85){
			extraCost = 1;
		} else if(hardness[p->pos[Y_LOC]][p->pos[X_LOC]] >= 85 && hardness[p->pos[Y_LOC]][p->pos[X_LOC]] < 171){
			extraCost = 2;
		} else {
			extraCost = 3;
		} 
		
		//TOP LEFT
		if(wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC] - 1].hn && wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC] -1].cost > p->cost + extraCost){
			wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC] - 1].hn);
		}
		
		//LEFT
		if(wMap[p->pos[Y_LOC]][p->pos[X_LOC] - 1].hn && wMap[p->pos[Y_LOC]][p->pos[X_LOC] -1].cost > p->cost + extraCost){
			wMap[p->pos[Y_LOC]][p->pos[X_LOC] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[Y_LOC]][p->pos[X_LOC] - 1].hn);
		}
		
		//BOTTOM LEFT
		if(wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC] - 1].hn && wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC] -1].cost > p->cost + extraCost){
			wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC] - 1].hn);
		}
		
		//BOTTOM
		if(wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC]].hn && wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC]].cost > p->cost + extraCost){
			wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC]].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC]].hn);
		}
		
		//BOTTOM RIGHT
		if(wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1].hn && wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1].cost > p->cost + extraCost){
			wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[Y_LOC] + 1][p->pos[X_LOC] + 1].hn);
		}
		
		//RIGHT
		if(wMap[p->pos[Y_LOC]][p->pos[X_LOC] + 1].hn && wMap[p->pos[Y_LOC]][p->pos[X_LOC] + 1].cost > p->cost + extraCost){
			wMap[p->pos[Y_LOC]][p->pos[X_LOC] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[Y_LOC]][p->pos[X_LOC] + 1].hn);
		}
		
		//TOP RIGHT
		if(wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1].hn && wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1].cost > p->cost + extraCost){
			wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC] + 1].hn);
		}
		
		//TOP
		if(wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC]].hn && wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC]].cost > p->cost + extraCost){
			wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC]].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[Y_LOC] - 1][p->pos[X_LOC]].hn);
		}
	}
	
	for(y = 0; y < GAME_HEIGHT; y++){
		for(x = 0; x < GAME_WIDTH; x++){
			wholeMap[y][x] = wMap[y][x].cost;
		}
	}
}


void printRoomHeatMap(int roomMap[GAME_HEIGHT][GAME_WIDTH], int pc[2]){
	int x, y;
	for(y = 0; y < GAME_HEIGHT; y++){
		for(x = 0; x < GAME_WIDTH; x++){
			if(roomMap[y][x] == INT_MAX){
				printf(" ");
			} else if(y == pc[Y_LOC] && x == pc[X_LOC]){
				printf("@");
			} else {
				printf("%d", roomMap[y][x]);
			}
		}
		printf("\n");
	}
}

void printWholeHeatMap(int wholeMap[GAME_HEIGHT][GAME_WIDTH], int pc[2]){
	int x, y;
	for(y = 0; y < GAME_HEIGHT; y++){
		for(x = 0; x < GAME_WIDTH; x++){
			if(wholeMap[y][x] == INT_MAX){
				printf("X");
			} else if(y == pc[Y_LOC] && x == pc[X_LOC]){
				printf("@");
			} else {
				printf("%d", wholeMap[y][x]);
			}
		}
		printf("\n");
	}
}

Character_t generateMonsters(char dungeon[WINDOW_Y][WINDOW_X], int hardness[GAME_HEIGHT][GAME_WIDTH], 
							int PC[2], char symbol){
	/*
	PC_t *PC; //tells us the character is a pc
	NPC_t *NPC; //tells us the character is a pc
	int s; //speed PC:10, MONST: 5-20
	int i; //intelligence
	int t; //telepathy
	int tu; //tunneling ability
	int e; //erratic
	int pcLoc[2]; //last known pc-location for intelligent monsters
	char c; //Symbol for the character
	int a; //tells whether character is alive or not
	int pos[2]; //position of character
	int cn; //character number
	int nt; //next turn value for priority queue
	int sn; //sequence number for priority queue
	heap_node_t *hn; //heap node for priority queue
	char lv; //holds last value AKA value it is replacing while still
	*/
	Character_t character;
	int randomX, randomY;
	int monsterPlaced = FALSE;
	
	character.c = symbol;
	character.a = TRUE;
	
	/*
	PC is set not to move AT ALL
	*/
	if(character.c == '@'){
		character.NPC = NULL;
		character.a = TRUE;
		character.pos[X_LOC] = PC[X_LOC];
		character.pos[Y_LOC] = PC[Y_LOC];
		character.s = 10;
		character.nt = 0;
		character.sn = 0;
	} else {
		character.PC = NULL;
		character.s = 5 + (rand() % 16);
		character.i = rand() % 2;
		character.t = rand() % 2;
		character.tu = rand() % 2;
		character.e = rand() % 2;
		character.nt = 0;
	}
	
	/*
	Hexadecimal format of the characters characteristics
	*/
	if(character.i == FALSE && character.t == FALSE && character.tu == FALSE && character.e == FALSE){
		character.c = '0';
	} else if(character.i == TRUE && character.t == FALSE && character.tu == FALSE && character.e == FALSE){
		character.c = '1';
	} else if(character.i == FALSE && character.t == TRUE && character.tu == FALSE && character.e == FALSE){
		character.c = '2';
	} else if(character.i == TRUE && character.t == TRUE && character.tu == FALSE && character.e == FALSE){
		character.c = '3';
	} else if(character.i == FALSE && character.t == FALSE && character.tu == TRUE && character.e == FALSE){
		character.c = '4';
	} else if(character.i == TRUE && character.t == FALSE && character.tu == TRUE && character.e == FALSE){
		character.c = '5';
	} else if(character.i == FALSE && character.t == TRUE && character.tu == TRUE && character.e == FALSE){
		character.c = '6';
	} else if(character.i == TRUE && character.t == TRUE && character.tu == TRUE && character.e == FALSE){
		character.c = '7';
	} else if(character.i == FALSE && character.t == FALSE && character.tu == FALSE && character.e == TRUE){
		character.c = '8';
	} else if(character.i == TRUE && character.t == FALSE && character.tu == FALSE && character.e == TRUE){
		character.c = '9';
	} else if(character.i == FALSE && character.t == TRUE && character.tu == FALSE && character.e == TRUE){
		character.c = 'a';
	} else if(character.i == TRUE && character.t == TRUE && character.tu == FALSE && character.e == TRUE){
		character.c = 'b';
	} else if(character.i == FALSE && character.t == FALSE && character.tu == TRUE && character.e == TRUE){
		character.c = 'c';
	} else if(character.i == TRUE && character.t == FALSE && character.tu == TRUE && character.e == TRUE){
		character.c = 'd';
	} else if(character.i == FALSE && character.t == TRUE && character.tu == TRUE && character.e == TRUE){
		character.c = 'e';
	} else if(character.i == TRUE && character.t == TRUE && character.tu == TRUE && character.e == TRUE){
		character.c = 'f';
	}
	
	/*
	Monster is placed randomly INSIDE one of the open spaces
	*/
	if(character.c != '@'){
		while(!monsterPlaced){
			randomY = 1 + rand() % GAME_HEIGHT - 1;
			randomX = 1 + rand() % GAME_WIDTH - 2;
			if(!hardness[randomY][randomX] && (dungeon[randomY][randomX] == '.' || dungeon[randomY][randomX] == '#')){	
				character.lv = dungeon[randomY][randomX];
				dungeon[randomY][randomX] = character.c;
				character.pos[0] = randomX;
				character.pos[1] = randomY;
				monsterPlaced = TRUE;
			}
		}
	}
	return character;
}

void inSight(char dungeon[WINDOW_Y][WINDOW_X], Character_t *character, int rooms[MAX_ROOMS][MAX_CONSTRAINTS], int *number_of_rooms, int PC[2]){
	int inSight = FALSE;
	int	i, j;
	
	if(!character->t){ //skip this if the character is telepathic
		/*
		Checks if PC is in the same room as the monster. In which case the monster, telepathic or not, will have the PC's last location
		*/
		if(character->c != '@'){
			for(i = 0; i < *number_of_rooms; i++){
				if((PC[X_LOC] >= rooms[i][X_LOC] && PC[Y_LOC] >= rooms[i][Y_LOC]) && 
					(PC[X_LOC] <= (rooms[i][X_LOC] + rooms[i][X_LEN]) && PC[Y_LOC] <= (rooms[i][Y_LOC] + rooms[i][Y_LEN]))){
					if((character->pos[X_LOC] >= rooms[i][X_LOC] && character->pos[Y_LOC] >= rooms[i][Y_LOC]) && 
						(character->pos[X_LOC] <= (rooms[i][X_LOC] + rooms[i][X_LEN]) && character->pos[Y_LOC] <= (rooms[i][Y_LOC] + rooms[i][Y_LEN]))){
						inSight = TRUE;
					}
				}
			}
		}
	} 
	
	/*
	If the monster is telepathic then the pc's location is assigned to the character
	If the PC is visible to the monster then he also remebers the last location
	*/
	if(character->t || inSight){
		character->pcLoc[X_LOC] = PC[X_LOC];
		character->pcLoc[Y_LOC] = PC[Y_LOC];
	} else {
		character->pcLoc[X_LOC] = 0;
		character->pcLoc[Y_LOC] = 0;
	}
}

//function deletes a monster and updates the characters array
int deleteMonster(char dungeon[WINDOW_Y][WINDOW_X], Character_t *characters[MAX_MONSTERS], int newXpos, int newYpos, int *num_Mon){
	int i, j;
	//skip 0: PC location
	for(i = 1; i < *num_Mon + 1; i++){ //loops through all our characters
		if(newYpos == characters[i]->pos[Y_LOC] && newXpos == characters[i]->pos[X_LOC]){ //checks if the new position is occupied by a monster
			dungeon[newYpos][newXpos] = characters[i]->lv; //Replace the pos with the dungeon char
			for(j = i; j < *num_Mon; j++){ //overwrite the deleted monster with the succeeding monsters
				characters[j] = characters[j + 1];
			}
			*num_Mon -= 1; //Essentially deletes the monster
			return 1;
		}
	}
	return 0;
}

void scanWall(int wholeMap[GAME_HEIGHT][GAME_WIDTH], int roomMap[GAME_HEIGHT][GAME_WIDTH], int direction, Character_t *p, int move[2]){
	int totalScanX[8] = {1, 1, 1, 0, -1, -1, -1, 0};
	int totalScanY[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
	int scanX[3];
	int scanY[3];
	int i, moveXdir, moveYdir;
	/*
	1: RIGHT
	2: BOTTOM RIGHT
	3. BOTTOM
	4. BOTTOM LEFT
	5. LEFT
	6. TOP LEFT
	7. TOP
	8. TOP RIGHT
	*/
	
	if(direction == 1){ //RIGHT
		for(i = 0; i < 3; i++){
			scanX[i] = totalScanX[i];
			scanY[i] = totalScanY[i];
		}
	} else if(direction == 2){ //BOTTOM RIGHT CORNER
		for(i = 0; i < 3; i++){
			scanX[i] = totalScanX[i + 1];
			scanY[i] = totalScanY[i + 1];
		}
	} else if(direction == 3){ //BOTTOM
		for(i = 0; i < 3; i++){
			scanX[i] = totalScanX[i + 2];
			scanY[i] = totalScanY[i + 2];
		}
	} else if(direction == 4){ //BOTTOM LEFT CORNER
		for(i = 0; i < 3; i++){
			scanX[i] = totalScanX[i + 3];
			scanY[i] = totalScanY[i + 3];
		}
	} else if(direction == 5){ //LEFT
		for(i = 0; i < 3; i++){
			scanX[i] = totalScanX[i + 4];
			scanY[i] = totalScanY[i + 4];
		}
	} else if(direction == 6){ //TOP LEFT CORNER
		for(i = 0; i < 3; i++){
			scanX[i] = totalScanX[i + 5];
			scanY[i] = totalScanY[i + 5];
		}
	} else if(direction == 7) { //TOP
		for(i = 0; i < 3; i++){
			if((i + 6) < 8){
				scanX[i] = totalScanX[i + 6];
				scanY[i] = totalScanY[i + 6];
			} else {
				scanX[i] = totalScanX[0];
				scanY[i] = totalScanY[0];
			}
		}
	} else { //TOP LEFT CORNER //UPDATE!!!
		int buffer = 1;
		for(i = 0; i < 3; i++){
			if((i + 7) < 8){
				scanX[i] = totalScanX[i + 6];
				scanY[i] = totalScanY[i + 6];
			} else {
				//Subtract the buffer from the i value to get the looped value
				scanX[i] = totalScanX[i - buffer]; 
				scanY[i] = totalScanY[i - buffer];
			}
		}
	}
	
	if(p->tu){ //use the whole map
		if(wholeMap[p->pos[Y_LOC] + scanY[1]][p->pos[X_LOC] + scanX[1]] <= wholeMap[p->pos[Y_LOC] + scanY[0]][p->pos[X_LOC] + scanX[0]]){
			if(wholeMap[p->pos[Y_LOC] + scanY[1]][p->pos[X_LOC] + scanX[1]] <= wholeMap[p->pos[Y_LOC] + scanY[2]][p->pos[X_LOC] + scanX[2]]){
				move[X_LOC] = scanX[1];
				move[Y_LOC] = scanY[1];
			} else {
				move[X_LOC] = scanX[2];
				move[Y_LOC] = scanY[2];
			}
		} else {
			if(wholeMap[p->pos[Y_LOC] + scanY[0]][p->pos[X_LOC] + scanX[0]] <= wholeMap[p->pos[Y_LOC] + scanY[2]][p->pos[X_LOC] + scanY[2]]){
				move[X_LOC] = scanX[0];
				move[Y_LOC] = scanY[0];
			} else {
				move[X_LOC] = scanX[2];
				move[Y_LOC] = scanY[2];
			}
		}
	} else { //use the room map
		if(roomMap[p->pos[Y_LOC] + scanY[1]][p->pos[X_LOC] + scanX[1]] <= roomMap[p->pos[Y_LOC] + scanY[0]][p->pos[X_LOC] + scanX[0]]){
			if(roomMap[p->pos[Y_LOC] + scanY[1]][p->pos[X_LOC] + scanX[1]] <= roomMap[p->pos[Y_LOC] + scanY[2]][p->pos[X_LOC] + scanX[2]]){
				move[X_LOC] = scanX[1];
				move[Y_LOC] = scanY[1];
			} else {
				move[X_LOC] = scanX[2];
				move[Y_LOC] = scanY[2];
			}
		} else {
			if(roomMap[p->pos[Y_LOC] + scanY[0]][p->pos[X_LOC] + scanX[0]] <= roomMap[p->pos[Y_LOC] + scanY[2]][p->pos[X_LOC] + scanY[2]]){
				move[X_LOC] = scanX[0];
				move[Y_LOC] = scanY[0];
			} else {
				move[X_LOC] = scanX[2];
				move[Y_LOC] = scanY[2];
			}
		}
	}
}

//comparitor for Character_t types
static int32_t compare_characters(const void *c1, const void *c2) {
	if(((Character_t *) c1)->nt == ((Character_t *) c2)->nt){
		return ((Character_t *) c1)->sn - ((Character_t *) c2)->sn;
	} else {
		return ((Character_t *) c1)->nt - ((Character_t *) c2)->nt;
	}
}

//simulates the game with the given monster characteristics
void simulateGame(char dungeon[WINDOW_Y][WINDOW_X], int hardness[GAME_HEIGHT][GAME_WIDTH], 
					int PC[2], Character_t **characters, int roomMap[GAME_HEIGHT][GAME_WIDTH],
					int wholeMap[GAME_HEIGHT][GAME_WIDTH], int rooms[MAX_ROOMS][MAX_CONSTRAINTS], int *num_Mon){
	heap_t h;
	heap_init(&h, compare_characters, NULL);
	static Character_t *p;
	int i, j, randX, randY;
	int moveXdir = 0;
	int moveYdir = 0;
	int isErratic = FALSE;
	int inSight = FALSE;
	
	//Insert all the initial characters
	for(i = 0; i <= *num_Mon; i++){
		characters[i]->hn = heap_insert(&h, &characters[i]);
	}
	
	// // //While the PC is alive, move monsters
	// while(characters[0]->a){ //pull the next moving monster to move him
		// p = heap_remove_min(&h); 
		// // //PRINTS INFO FOR EVERY MONSTER INFO
		// // printf("NAME: %c\n", p->c);
		// // printf("SPEED: %d\n", p->s);
		// // printf("INTELLIGENCE: %d\n", p->i);
		// // printf("TELEPATHY: %d\n", p->t);
		// // printf("TUNNELLING: %d\n", p->tu);
		// // printf("ERRATIC: %d\n", p->e);
		// // printf("PC LOC: %d %d\n", p->pcLoc[Y_LOC], p->pcLoc[X_LOC]);
		// // printf("ALIVE: %d\n", p->a);
		// // printf("LOCATION: %d %d\n", p->pos[Y_LOC], p->pos[X_LOC]);
		// // printf("OLD CHAR: %c\n", p->lv);
		// // printf("----------------------------------------\n");
		// p->hn = NULL;
		// int i, moveTo[2];
						
		// //If the node pulled out is the PC then print the updated dungeon and pause
		// if(p->c == '@'){
			// printDungeon(dungeon);
			// usleep(250000);
		// } else {
			// if(p->pcLoc[X_LOC] && p->pcLoc[Y_LOC]){
				// inSight = TRUE;
			// }
			
			// if(p->e){
				// //If the character is erratic then this will determine whether it acts erratic on the next step
				// isErratic = rand() % 2; //0: normal behavior, 1: erratic behavior
			// }
			
			// /*
			// 1: RIGHT
			// 2: BOTTOM RIGHT
			// 3. BOTTOM
			// 4. BOTTOM LEFT
			// 5. LEFT
			// 6. TOP LEFT
			// 7. TOP
			// 8. TOP RIGHT
			// */
			// if(!isErratic){
				// if(p->i && inSight){ //intelligent monster and telepathic or can see the PC
					// if(characters[0]->pos[X_LOC] > p->pos[X_LOC]){ //PC is to the right of the monster
						// if(characters[0]->pos[Y_LOC] > p->pos[Y_LOC]){ //DOWN-RIGHT
							// scanWall(wholeMap, roomMap, 2, p, moveTo);
							// moveXdir = moveTo[X_LOC];
							// moveYdir = moveTo[Y_LOC];
						// } else if (characters[0]->pos[Y_LOC] < p->pos[Y_LOC]) { //UP-RIGHT
							// scanWall(wholeMap, roomMap, 8, p, moveTo);
							// moveXdir = moveTo[X_LOC];
							// moveYdir = moveTo[Y_LOC];
						// } else { //PC is only to the right of the monster so scan the right "wall" of vertices
							// scanWall(wholeMap, roomMap, 1, p, moveTo);
							// moveXdir = moveTo[X_LOC];
							// moveYdir = moveTo[Y_LOC];
						// }
					// } else if (characters[0]->pos[X_LOC] < p->pos[X_LOC]) { //PC is to the left of the monster
						// if(characters[0]->pos[Y_LOC] > p->pos[Y_LOC]){ //DOWN-LEFT
							// scanWall(wholeMap, roomMap, 4, p, moveTo);
							// moveXdir = moveTo[X_LOC];
							// moveYdir = moveTo[Y_LOC];
						// } else if (characters[0]->pos[Y_LOC] < p->pos[Y_LOC]) { //UP-LEFT
							// scanWall(wholeMap, roomMap, 6, p, moveTo);
							// moveXdir = moveTo[X_LOC];
							// moveYdir = moveTo[Y_LOC];
						// } else { //PC is only to the left of the monster so scan the left "wall" of vertices
							// scanWall(wholeMap, roomMap, 5, p, moveTo);
							// moveXdir = moveTo[X_LOC];
							// moveYdir = moveTo[Y_LOC];
						// }
					// } else { //Not intelligent
						// if(characters[0]->pos[Y_LOC] > p->pos[Y_LOC]){ //PC is directly below the monster
							// scanWall(wholeMap, roomMap, 3, p, moveTo);
							// moveXdir = moveTo[X_LOC];
							// moveYdir = moveTo[Y_LOC];
						// } else { //PC is directly above the monster
							// scanWall(wholeMap, roomMap, 7, p, moveTo);
							// moveXdir = moveTo[X_LOC];
							// moveYdir = moveTo[Y_LOC];
						// }
					// }
				// } else { //not intelligent or can't see the PC
					// //default if the monster knows where the PC is but doens't have any special abilities
					// if(inSight){
						// //calculate the X move direction to the PC
						// if(abs(p->pcLoc[X_LOC] - p->pos[X_LOC])){
							// moveXdir = ((p->pcLoc[X_LOC] - p->pos[X_LOC]) / (abs(p->pcLoc[X_LOC] - p->pos[X_LOC])));
						// }
						// //calculate the Y move direction to the PC
						// if(abs(p->pcLoc[Y_LOC] - p->pos[Y_LOC])){
							// moveYdir = ((p->pcLoc[Y_LOC] - p->pos[Y_LOC]) / (abs(p->pcLoc[Y_LOC] - p->pos[Y_LOC])));
						// }	
					// }  else { //default erratic behavior for non-intelligent monsters. Intelligent monsters stay still, preserving "energy"
						// if(!p->i){
							// moveXdir = (rand() % 3) - 1;
							// moveYdir = (rand() % 3) - 1;
						// }	
					// }
				// }
			// } else { //Erratic behavior
				// moveXdir = (rand() % 3) - 1;
				// moveYdir = (rand() % 3) - 1;
			// }
			
			// //Move the monster if the random new space is different from itself
			// //and the new location is an open area
			// //or it can tunnel through rock
			// //Can move to any of the 8 surrounding cells
			// if((moveYdir || moveXdir) && ((p->tu && hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] < 255) || !hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir]) 
				// && dungeon[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] != '@'){
				// //Checks if the new space is a monster
				// // int didDeleteMonst = deleteMonster(dungeon, characters, p->pos[X_LOC] + moveXdir, p->pos[Y_LOC] + moveYdir, num_Mon);
				// //if the monster was deleted then don't do anything
				// if(!deleteMonster(dungeon, characters, p->pos[X_LOC] + moveXdir, p->pos[Y_LOC] + moveYdir, num_Mon)){
					// //if the new location is an open room, corridor or staircase then update stats
					// if(hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] == 0){ 
						// dungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->lv; //replace monster with old room or corridor char
						// p->pos[X_LOC] = p->pos[X_LOC] + moveXdir; //update monster x location
						// p->pos[Y_LOC] = p->pos[Y_LOC] + moveYdir; //update monster y location
						// p->lv = dungeon[p->pos[Y_LOC]][p->pos[X_LOC]];
						// dungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->c; //replace dungeon character with new monster character
						// if((p->pos[Y_LOC] == p->pcLoc[Y_LOC]) && (p->pos[X_LOC] == p->pcLoc[X_LOC])){ //Monster is in pc last seen position but it isn't there
							// p->pcLoc[X_LOC] = 0; //remove x pc location
							// p->pcLoc[Y_LOC] = 0; //remove y pc location
						// }
					// } else if(p->tu && hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir]){ //monster can tunnel so it starts "smashing" the wall
						// hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] = hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] - 85; //update hardness
						// if(hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] <= 0){ //mutable rock has been broken open so monster can go there
							// dungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->lv; //replace monster with old room or corridor char
							// p->lv = '#';
							// p->pos[X_LOC] = p->pos[X_LOC] + moveXdir; //update monster x location
							// p->pos[Y_LOC] = p->pos[Y_LOC] + moveYdir; //update monster y location
							// dungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->c; //replace dungeon character with new monster character
							// //Update heat maps
							// roomHeatMapGenerator(PC, roomMap, hardness);
							// wholeHeatMapGenerator(PC, wholeMap, hardness);
						// } //mutable rock still hasn't broken so do nothing
					// } 
				// }
			// } else if (dungeon[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] == '@'){ //found the pc and killed him
				// characters[0]->a = FALSE;
			// }
		// }
		
		// p->nt = p->nt + 1000/p->s;
		// heap_insert(&h, p);
	// }
	
	printf("YOU LOSE!\n");
	heap_delete(&h);
	free(*characters);
}

int main(int argc, char *argv[]){
	int number_of_rooms = 0; //holds values for number of rooms generated in the dungeon
	int number_of_upstairs = 0; //holds values for number of upcases generated in the dungeon
	int number_of_downstairs = 0; //holds values for number of downcases generated in the dungeon
	
	char dungeon[WINDOW_Y][WINDOW_X]; //array containing everything about the dungeon for the user
	int hardness[GAME_HEIGHT][GAME_WIDTH];
	/*	
	Array containing hardness of cells. 
	255: immutable rock, 
	0:Room or corridor, 
	127: mutable rock
	*/
	
	int rooms[MAX_ROOMS][MAX_CONSTRAINTS];
	/*
	0: x-position
	1: y-position
	2: x-length
	3: y-length
	*/
	
	int upwardCases[MAX_ROOMS][2];
	/*
	holds upward staircases
	0: x-coordinate
	1: y-coordinate
	*/
	
	int downwardCases[MAX_ROOMS][2];
	/*
	holds downward staircases
	0: x-coordinate
	1: y-coordinate
	*/
	
	int PC[2];
	/*
	0: x-coordinate
	1: y-coordinate
	*/
	
	int roomMap[GAME_HEIGHT][GAME_WIDTH];
	/*
	contains open space distance map from PC based on Dijkstra's alg
	*/
	
	int wholeMap[GAME_HEIGHT][GAME_WIDTH];
	/*
	contains whole distance map from PC based on Dijkstra's alg
	*/

	Character_t *characters = malloc(MAX_MONSTERS * sizeof(Character_t));
	/*
	Array containing every monster in our dungeon
	*/
	
	//Takes the time to randomize our room generation
	srand(time(NULL));
	
	/*
	char *home = getenv("HOME");
	char *directory;
	directory = malloc(strlen(home) + strlen("/.rlg327/dungeon") + 1);
	strcpy(directory, home);
	strcat(directory, "/.rlg327/dungeon");
	*/		
			
	char *directory = "C:/Users/morro/.rlg327/dungeon.txt";
	// char *directory = "C:/Users/morro/.rlg327/01.rlg327";
	// char *directory = "/home/student/.rlg327/01.rlg327";
	
	int i = 0;
	int j = 0;
	int num_Mon = 1;
	
	user_action action;
	if(!strcmp(argv[1], "--save")){
		action = save;
	} else if(!strcmp(argv[1], "--load")){
		action = load;
	} else if(!strcmp(argv[1], "--load--save")){
		action = load_save;
	} else if(!strcmp(argv[1], "--nummon")){
		num_Mon = atoi(argv[2]);
		if(num_Mon >= MAX_MONSTERS){
			num_Mon = MAX_MONSTERS - 1;
		}
		action = num_mon;
	} else{
		printf("Please enter: <--save, --load, --load--save> to perforam an action");
		return -1;
	}
	
	switch(action){
		case load:
			loadDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
			generateDungeon(dungeon, hardness);
			generateRooms(rooms, dungeon, hardness, &number_of_rooms);
			generateCorridors(dungeon, rooms, &number_of_rooms, hardness, &number_of_upstairs);
			generateStairs(dungeon, hardness, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, PC);
			printDungeon(dungeon);
			roomHeatMapGenerator(PC, roomMap, hardness);
			// printRoomHeatMap(roomMap, PC);
			wholeHeatMapGenerator(PC, wholeMap, hardness);
			// printWholeHeatMap(wholeMap, PC);
			// free(directory);	
			break;
		case save:
			generateDungeon(dungeon, hardness);
			generateRooms(rooms, dungeon, hardness, &number_of_rooms);
			generateCorridors(dungeon, rooms, &number_of_rooms, hardness, &number_of_upstairs);
			generateStairs(dungeon, hardness, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, PC);
			printDungeon(dungeon);
			saveDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
			// roomHeatMapGenerator(PC, roomMap, hardness);
			printRoomHeatMap(roomMap, PC);
			// wholeHeatMapGenerator(PC, wholeMap, hardness);
			printWholeHeatMap(wholeMap, PC);
			// free(directory);
			break;
		case load_save:
			loadDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
			generateDungeon(dungeon, hardness);
			generateRooms(rooms, dungeon, hardness, &number_of_rooms);
			generateCorridors(dungeon, rooms, &number_of_rooms, hardness, &number_of_upstairs);
			generateStairs(dungeon, hardness, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, PC);
			printDungeon(dungeon);
			roomHeatMapGenerator(PC, roomMap, hardness);
			// printRoomHeatMap(roomMap, PC);
			wholeHeatMapGenerator(PC, wholeMap, hardness);
			// printWholeHeatMap(wholeMap, PC);
			saveDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
			// free(directory);
			break;
		case num_mon:
			generateDungeon(dungeon, hardness);
			generateRooms(rooms, dungeon, hardness, &number_of_rooms);
			generateCorridors(dungeon, rooms, &number_of_rooms, hardness, &number_of_upstairs);
			generateStairs(dungeon, hardness, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, PC);
			roomHeatMapGenerator(PC, roomMap, hardness);
			wholeHeatMapGenerator(PC, wholeMap, hardness);
			while(i < num_Mon + 1){ //generate all the monsters and their characteristics
				Character_t character;
				if(i == 0){
					character = generateMonsters(dungeon, hardness, PC, '@'); //generate the pc and its characteristcs
				} else {
					character = generateMonsters(dungeon, hardness, PC, 'm'); //generate the monsters and its characteristics
					inSight(dungeon, &character, rooms, &number_of_rooms, PC); //check if the monster can see the PC
				}
				character.sn = i;
				characters[i] = character;
				i++;
			}
			simulateGame(dungeon, hardness, PC, &characters, roomMap, wholeMap, rooms, &num_Mon);
			// free(directory);
			break;
	}
	
	return 0;
}