#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <endian.h>
#include <limits.h>

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
#define PC_Y 1
#define PC_X 0

//defines user actions when running game.c
typedef enum action{
	save,
	load,
	load_save
} user_action;

// typedef struct{
	// // heap_node_t *hn;
	// int x; //x-coordinate
	// int y; //y-coordinate
	// int cost; //cost
	// // int v;
// } Cell;

typedef struct Cell {
  heap_node_t *hn;
  uint8_t pos[2];
  int32_t cost;
} Cell_t;

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
			roomsArray[*numRooms][0] = 1 + rand() % GAME_WIDTH - 2; //x-position
			roomsArray[*numRooms][1] = 1 + rand() % GAME_HEIGHT - 2; //y-position
			roomsArray[*numRooms][2] = MIN_X_ROOM + (rand() % MAX_MINUS_MIN_X); //x-length
			roomsArray[*numRooms][3] = MIN_Y_ROOM + (rand() % MAX_MINUS_MIN_Y); //y-length
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
			for(i = -1; i < roomsArray[*numRooms - roomBuffer][3] + 1; i++){
				for(j = -1; j < roomsArray[*numRooms - roomBuffer][2] + 1; j++){
					if(hardnessArray[roomsArray[*numRooms - roomBuffer][1] + i][roomsArray[*numRooms - roomBuffer][0] + j] == 0 || hardnessArray[roomsArray[*numRooms - roomBuffer][1] + i][roomsArray[*numRooms - roomBuffer][0] + j] == 255){
						isOccupied = TRUE;
						break;
					}
				}
			}
		}
				
		//pastes room in dungeon
		if(isOccupied == FALSE){
			for(i = 0; i < roomsArray[*numRooms - roomBuffer][3]; i++){
				for(j = 0; j < roomsArray[*numRooms - roomBuffer][2]; j++){
					//Sets the cell value to '.' to represent a segment of the room
					dungeonArray[roomsArray[*numRooms - roomBuffer][1] + i][roomsArray[*numRooms - roomBuffer][0] + j] = '.';
					if(loadingDungeon == FALSE){
						//Sets hardness to 0 for every cell within the room
						hardnessArray[roomsArray[*numRooms - roomBuffer][1] + i][roomsArray[*numRooms - roomBuffer][0] + j] = 0;
					}
				}
			}
		}
		
		if(isOccupied == FALSE && loadingDungeon == FALSE){
			*numRooms = *numRooms + 1;
			totalArea += (roomsArray[*numRooms][3] * roomsArray[*numRooms][2]);
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
			if(abs(rooms[roomNumber + 1][0] - rooms[roomNumber][0]) != 0){
				stepOverDirectionX = ((rooms[roomNumber + 1][0] - rooms[roomNumber][0]) / (abs(rooms[roomNumber + 1][0] - rooms[roomNumber][0])));
			}
			if(abs(rooms[roomNumber + 1][1] - rooms[roomNumber][1]) != 0){
				stepOverDirectionY = ((rooms[roomNumber + 1][1] - rooms[roomNumber][1]) / (abs(rooms[roomNumber + 1][1] - rooms[roomNumber][1])));
			}
			
			int currentX = rooms[roomNumber][0];
			int currentY = rooms[roomNumber][1];
			
			// Places # characters from the first room X to the corner X
			while(currentX != rooms[roomNumber + 1][0]){
				if(hardnessArray[currentY][currentX] != 0){
					dungeon[currentY][currentX] = '#';
					//Sets hardness to 0 to represent an open cell
					hardnessArray[currentY][currentX] = 0;
				}
				currentX += stepOverDirectionX;
			}
			
			//Places # characters from the corner Y to the second room Y
			while(currentY != rooms[roomNumber + 1][1]){
				if(hardnessArray[currentY][currentX] != 0){
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
				if(hardnessArray[i][j] == 0 && dungeon[i][j] != '.' && dungeon[i][j] != '@'){
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
	if((*numUpCases != 0) && (*numDownCases != 0)){
		dungeonLoading = TRUE;
	}
	
	if(dungeonLoading == FALSE){
		while(*numUpCases == 0){
			randomY = 1 + rand() % GAME_HEIGHT - 1;
			randomX = 1 + rand() % GAME_WIDTH - 2;
			if((hardnessArray[randomY][randomX] == 0) && *numUpCases == 0 && dungeon[randomY][randomX] != '>'){
				dungeon[randomY][randomX] = '<';
				upwardCases[*numUpCases][0] = randomX;
				upwardCases[*numUpCases][1] = randomY;
				*numUpCases = *numUpCases + 1;
			}
		}
			
		while(*numDownCases == 0){
			randomY = 1 + rand() % GAME_HEIGHT - 1;
			randomX = 1 + rand() % GAME_WIDTH - 2;
			if((hardnessArray[randomY][randomX] == 0) && *numDownCases == 0 && dungeon[randomY][randomX] != '<'){
				dungeon[randomY][randomX] = '>';
				downwardCases[*numDownCases][0] = randomX;
				downwardCases[*numDownCases][1] = randomY;
				*numDownCases = *numDownCases + 1;
			}
		}
		
		while(!pcPlaced){
			randomY = 1 + rand() % GAME_HEIGHT - 1;
			randomX = 1 + rand() % GAME_WIDTH - 2;
			if((hardnessArray[randomY][randomX] == 0) && dungeon[randomY][randomX] != '>' && dungeon[randomY][randomX] != '<'){
				dungeon[randomY][randomX] = '@';
				pc_location[PC_Y] = randomY;
				pc_location[PC_X] = randomX;
				pcPlaced = TRUE;
			}
		}
		
	} else{
		for(i = 0; i < *numUpCases; i++){
			dungeon[upwardCases[i][1]][upwardCases[i][0]] = '<';
		}
		for(i = 0; i < *numDownCases; i++){
			dungeon[downwardCases[i][1]][downwardCases[i][0]] = '>';
		}
		dungeon[pc_location[PC_Y]][pc_location[PC_X]] = '@';
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

//copied from professor's heap.c code
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
				map[y][x].pos[0] = y;
				map[y][x].pos[1] = x;
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
	map[pc[PC_Y]][pc[PC_X]].cost = 0;
	
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
	while(p = heap_remove_min(&h)){
		p->hn = NULL;
		
		int extraCost;
		if(hardness[p->pos[0]][p->pos[1]] == 0){
			extraCost = 1;
		} else {
			extraCost = INT_MAX;
		}
	
		//TOP LEFT
		if(map[p->pos[0] - 1][p->pos[1] - 1].hn && map[p->pos[0] - 1][p->pos[1] -1].cost > p->cost + extraCost && hardness[p->pos[0] - 1][p->pos[1] - 1] == 0){
			map[p->pos[0] - 1][p->pos[1] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[0] - 1][p->pos[1] - 1].hn);
		}
		
		//LEFT
		if(map[p->pos[0]][p->pos[1] - 1].hn && map[p->pos[0]][p->pos[1] -1].cost > p->cost + extraCost && hardness[p->pos[0]][p->pos[1] - 1] == 0){
			map[p->pos[0]][p->pos[1] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[0]][p->pos[1] - 1].hn);
		}
		
		//BOTTOM LEFT
		if(map[p->pos[0] + 1][p->pos[1] - 1].hn && map[p->pos[0] + 1][p->pos[1] -1].cost > p->cost + extraCost && hardness[p->pos[0] + 1][p->pos[1] - 1] == 0){
			map[p->pos[0] + 1][p->pos[1] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[0] + 1][p->pos[1] - 1].hn);
		}
		
		//BOTTOM
		if(map[p->pos[0] + 1][p->pos[1]].hn && map[p->pos[0] + 1][p->pos[1]].cost > p->cost + extraCost && hardness[p->pos[0] + 1][p->pos[1]] == 0){
			map[p->pos[0] + 1][p->pos[1]].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[0] + 1][p->pos[1]].hn);
		}
		
		//BOTTOM RIGHT
		if(map[p->pos[0] + 1][p->pos[1] + 1].hn && map[p->pos[0] + 1][p->pos[1] + 1].cost > p->cost + extraCost && hardness[p->pos[0] + 1][p->pos[1] + 1] == 0){
			map[p->pos[0] + 1][p->pos[1] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[0] + 1][p->pos[1] + 1].hn);
		}
		
		//RIGHT
		if(map[p->pos[0]][p->pos[1] + 1].hn && map[p->pos[0]][p->pos[1] + 1].cost > p->cost + extraCost && hardness[p->pos[0]][p->pos[1] + 1] == 0){
			map[p->pos[0]][p->pos[1] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[0]][p->pos[1] + 1].hn);
		}
		
		//TOP RIGHT
		if(map[p->pos[0] - 1][p->pos[1] + 1].hn && map[p->pos[0] - 1][p->pos[1] + 1].cost > p->cost + extraCost && hardness[p->pos[0] - 1][p->pos[1] + 1] == 0){
			map[p->pos[0] - 1][p->pos[1] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[0] - 1][p->pos[1] + 1].hn);
		}
		
		//TOP
		if(map[p->pos[0] - 1][p->pos[1]].hn && map[p->pos[0] - 1][p->pos[1]].cost > p->cost + extraCost && hardness[p->pos[0] - 1][p->pos[1]] == 0){
			map[p->pos[0] - 1][p->pos[1]].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, map[p->pos[0] - 1][p->pos[1]].hn);
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
				wMap[y][x].pos[0] = y;
				wMap[y][x].pos[1] = x;
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
	wMap[pc[PC_Y]][pc[PC_X]].cost = 0;
	
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
		if(hardness[p->pos[0]][p->pos[1]] >= 0 && hardness[p->pos[0]][p->pos[1]] < 85){
			extraCost = 1;
		} else if(hardness[p->pos[0]][p->pos[1]] >= 85 && hardness[p->pos[0]][p->pos[1]] < 171){
			extraCost = 2;
		} else {
			extraCost = 3;
		} 
		
		//TOP LEFT
		if(wMap[p->pos[0] - 1][p->pos[1] - 1].hn && wMap[p->pos[0] - 1][p->pos[1] -1].cost > p->cost + extraCost){
			wMap[p->pos[0] - 1][p->pos[1] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[0] - 1][p->pos[1] - 1].hn);
		}
		
		//LEFT
		if(wMap[p->pos[0]][p->pos[1] - 1].hn && wMap[p->pos[0]][p->pos[1] -1].cost > p->cost + extraCost){
			wMap[p->pos[0]][p->pos[1] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[0]][p->pos[1] - 1].hn);
		}
		
		//BOTTOM LEFT
		if(wMap[p->pos[0] + 1][p->pos[1] - 1].hn && wMap[p->pos[0] + 1][p->pos[1] -1].cost > p->cost + extraCost){
			wMap[p->pos[0] + 1][p->pos[1] - 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[0] + 1][p->pos[1] - 1].hn);
		}
		
		//BOTTOM
		if(wMap[p->pos[0] + 1][p->pos[1]].hn && wMap[p->pos[0] + 1][p->pos[1]].cost > p->cost + extraCost){
			wMap[p->pos[0] + 1][p->pos[1]].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[0] + 1][p->pos[1]].hn);
		}
		
		//BOTTOM RIGHT
		if(wMap[p->pos[0] + 1][p->pos[1] + 1].hn && wMap[p->pos[0] + 1][p->pos[1] + 1].cost > p->cost + extraCost){
			wMap[p->pos[0] + 1][p->pos[1] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[0] + 1][p->pos[1] + 1].hn);
		}
		
		//RIGHT
		if(wMap[p->pos[0]][p->pos[1] + 1].hn && wMap[p->pos[0]][p->pos[1] + 1].cost > p->cost + extraCost){
			wMap[p->pos[0]][p->pos[1] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[0]][p->pos[1] + 1].hn);
		}
		
		//TOP RIGHT
		if(wMap[p->pos[0] - 1][p->pos[1] + 1].hn && wMap[p->pos[0] - 1][p->pos[1] + 1].cost > p->cost + extraCost){
			wMap[p->pos[0] - 1][p->pos[1] + 1].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[0] - 1][p->pos[1] + 1].hn);
		}
		
		//TOP
		if(wMap[p->pos[0] - 1][p->pos[1]].hn && wMap[p->pos[0] - 1][p->pos[1]].cost > p->cost + extraCost){
			wMap[p->pos[0] - 1][p->pos[1]].cost = (p->cost + extraCost) % 10;
			heap_decrease_key_no_replace(&h, wMap[p->pos[0] - 1][p->pos[1]].hn);
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
			} else if(y == pc[PC_Y] && x == pc[PC_X]){
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
			} else if(y == pc[PC_Y] && x == pc[PC_X]){
				printf("@");
			} else {
				printf("%d", wholeMap[y][x]);
			}
		}
		printf("\n");
	}
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
	
	
	user_action action;
	if(!strcmp(argv[1], "--save")){
		action = save;
	} else if(!strcmp(argv[1], "--load")){
		action = load;
	} else if(!strcmp(argv[1], "--load--save")){
		action = load_save;
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
			printRoomHeatMap(roomMap, PC);
			wholeHeatMapGenerator(PC, wholeMap, hardness);
			printWholeHeatMap(wholeMap, PC);
			// free(directory);	
			break;
		case save:
			generateDungeon(dungeon, hardness);
			generateRooms(rooms, dungeon, hardness, &number_of_rooms);
			generateCorridors(dungeon, rooms, &number_of_rooms, hardness, &number_of_upstairs);
			generateStairs(dungeon, hardness, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, PC);
			printDungeon(dungeon);
			saveDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
			roomHeatMapGenerator(PC, roomMap, hardness);
			printRoomHeatMap(roomMap, PC);
			wholeHeatMapGenerator(PC, wholeMap, hardness);
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
			printRoomHeatMap(roomMap, PC);
			wholeHeatMapGenerator(PC, wholeMap, hardness);
			printWholeHeatMap(wholeMap, PC);
			saveDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
			// free(directory);
			break;
	}
	
	return 0;
}