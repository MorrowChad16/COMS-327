#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <endian.h>
#include <limits.h>
#include <unistd.h>
#include <ncurses.h>
#include <iostream>
#include <fstream>

#include "heap.h"

using namespace std;

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
#define NUM_MONSTERS 100
#define MAX_MONSTERS 21
#define MAX_EQUIPMENT 100
#define min(x, y) (x < y ? x : y)
#define MESSAGES 21


//defines user actions when running game.c
typedef enum action{
	save,
	load,
	load_save,
	num_mon
} user_action;

class Cell_t {
	public:
	  heap_node_t *hn;
	  uint8_t pos[2];
	  int32_t cost;
};

class Color {
public:
	bool BLACK;
	bool RED;
	bool GREEN;
	bool YELLOW;
	bool BLUE;
	bool MAGENTA;
	bool CYAN;
	bool WHITE;
};

class Dice {
	public: 
		int base;
		int dice;
		int sides;
};

//implement PC and NPC classes later
class Character_t {
	public:
		int s; //speed PC:10, MONST: 5-20
		int i; //intelligence
		int t; //telepathy
		int tu; //tunneling ability
		int e; //erratic
		int p; //pass
		int pu; //pickup
		int d; //destroy
		int u; //unique
		int b; //final boss
		int pcLoc[2]; //last known pc-location for intelligent monsters
		char c; //Symbol for the character
		bool a; //tells whether character is alive or not
		int pos[2]; //position of character
		int nt; //next turn value for priority queue
		int sn; //sequence number for priority queue
		heap_node_t *hn; //heap node for priority queue
		char lv; //holds last value AKA value it is replacing while still
		/*
		0: BLACK
		1: RED
		2: GREEN
		3: YELLOW
		4: BLUE
		5: MAGENTA
		6: CYAN
		7: WHITE
		*/
		Color color; //holds the color of the monster
		//having issues with using strings so using char array instead
		char name[78]; //holds the name of the monster
		char description[1024]; //holds the description of the monster
		int health; //Dice class for health
		Dice attackDamage; //Dice class for attackDamage
		int rarity; //odds of spawing the monster
};

class Items{
	public:
		bool weapon;
		bool offhand;
		bool ranged;
		bool armor;
		bool helmet;
		bool cloak;
		bool gloves;
		bool boots;
		bool ring;
		bool amulet;
		bool light;
		bool scroll;
		bool book;
		bool flask;
		bool gold;
		bool ammunition;
		bool food;
		bool wand;
		bool container;
};

class Equipment{
	public:
		char name[78]; //holds the name of the equipment
		char description[1024]; //holds the description of the monster
		Items equipment;
		Color color;
		int HB; //hit bonus
		Dice DB; //damage bonus
		int DDB; //dodge bonus
		int DFB; //defense bonus
		int weight;
		int SB; //speed bonus
		int SA; //special attribute
		int value;
		bool status;
		int rarity;
		char lv; //holds the dungeon character
		bool a; //tells whether the equipment is alive or not/can be placed or not
		int pos[2]; //position of equipment
		char c; //holds the equipment character
};

// class PC_t : public Character_t{
	// //add pc specific characteristics later
// };

// class NPC_t : public Character_t{
// public:
	// int i; //intelligence
	// int t; //telepathy
	// int tu; //tunneling ability
	// int e; //erratic
	// int p; //pass
	// int pu; //pickup
	// int d; //destroy
	// int u; //unique
	// int b; //final boss
	// int pcLoc[2]; //last known pc-location for intelligent monsters
	// bool color[8]; //holds the color of the monster
	// string name; //holds the name of the monster
	// string description; //holds the description of the monster
	// Dice_t health; //Dice class for health
	// Dice_t attackDamage; //Dice class for attackDamage
	// int rarity; //odds of spawing the monster
// };

void generateDungeon(char dungeonArray[WINDOW_Y][WINDOW_X], int hardnessArray[GAME_HEIGHT][GAME_WIDTH], char fogDungeon[GAME_HEIGHT][GAME_WIDTH]){
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
				fogDungeon[i][j] = '-';
				hardnessArray[i][j] = 255;
			//Left or Right Wall = immutable rock
			} else if((j == 0 || j == GAME_WIDTH - 1) && i < GAME_HEIGHT - 1){
				dungeonArray[i][j] = '|';
				fogDungeon[i][j] = '|';
				hardnessArray[i][j] = 255;
			//Everything inside this range has a random mutable rock range from [1, 254]
			} else if(j < GAME_WIDTH - 1 && i < GAME_HEIGHT - 1) {
				dungeonArray[i][j] = ' ';
				fogDungeon[i][j] = ' ';
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

void generateRooms(int roomsArray[MAX_ROOMS][MAX_CONSTRAINTS], char dungeon[WINDOW_Y][WINDOW_X], int hardnessArray[GAME_HEIGHT][GAME_WIDTH], int *numRooms){
	int i, j;
	int roomBuffer = *numRooms;
	int loadingDungeon = FALSE;
	if(*numRooms != 0){
		loadingDungeon = TRUE;
	}

	//Min: 4x by 3y
	//OWN MAX: 14x by 14y
	//103 is about 7% of a 78x19 area, which is the hard rock area available for placement
	while(((*numRooms - roomBuffer) < *numRooms && loadingDungeon == TRUE) || ((*numRooms - roomBuffer) < 6 && loadingDungeon == FALSE)){
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
					dungeon[roomsArray[*numRooms - roomBuffer][Y_LOC] + i][roomsArray[*numRooms - roomBuffer][X_LOC] + j] = '.';
					if(loadingDungeon == FALSE){
						//Sets hardness to 0 for every cell within the room
						hardnessArray[roomsArray[*numRooms - roomBuffer][Y_LOC] + i][roomsArray[*numRooms - roomBuffer][X_LOC] + j] = 0;
					}
				}
			}
		}
		if(isOccupied == FALSE && loadingDungeon == FALSE){
			*numRooms += 1;
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

void printDungeon(char dungeon[WINDOW_Y][WINDOW_X], int hardness[GAME_HEIGHT][GAME_WIDTH]){
	int i, j;
	//prints seed
	int seed = rand();
	// printf("Seed: %d\n", seed);
	//prints final array of game area and components
	for(i = 0; i < WINDOW_Y; i++){
		for(j = 0; j < WINDOW_X; j++){
			printf("%c", dungeon[i][j]);
		}
		printf("\n");
	}
	
	for(i = 0; i < GAME_HEIGHT; i++){
		for(j = 0; j < GAME_WIDTH; j++){
			printf("%d", hardness[i][j] / 85);
		}
		printf("\n");
	}
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
	while(p = (Cell_t *) heap_remove_min(&h)){
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
	
	while(p = (Cell_t *) heap_remove_min(&h)){
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

//Prints monsters and there relative location to the PC
void printMonsters(Character_t monsters[MAX_MONSTERS], int *num_Mon){
	int PC_LOC[2]; //Holds the PC location for comparison
	char REL_LOC[*num_Mon][2]; //Holds relative location from the PC
	char X_REL[5]; //Holds location to the PC X
	char Y_REL[7]; //Holds location to the PC Y
	char COMP_REL[*num_Mon][50];
	int i, j;
	
	int ableToScroll = FALSE;
	if(*num_Mon > WINDOW_Y){
		ableToScroll = TRUE;
	}
	
	PC_LOC[Y_LOC] = monsters[0].pos[Y_LOC];
	PC_LOC[X_LOC] = monsters[0].pos[X_LOC];
	
	for(i = 1; i <= *num_Mon; i++){
		REL_LOC[i][Y_LOC] = PC_LOC[Y_LOC] - monsters[i].pos[Y_LOC]; //Gets relative location from PC to the monster in the Y
		REL_LOC[i][X_LOC] = PC_LOC[X_LOC] - monsters[i].pos[X_LOC]; //Gets relative location from PC to the monster in the X
		
		if(REL_LOC[i][Y_LOC] < 0){ //NORTH
			strncpy(Y_REL, "north", 6);
		} else{ //SOUTH
			strncpy(Y_REL, "south", 6);
		}
		
		if(REL_LOC[i][X_LOC] < 0){ //WEST
			strncpy(X_REL, "west", 4);
		} else{ //EAST
			strncpy(X_REL, "east", 4);
		}
		
		sprintf(COMP_REL[i - 1], "%c: %d %s and %d %s", monsters[i].c, abs(REL_LOC[i][Y_LOC]), Y_REL, abs(REL_LOC[i][X_LOC]), X_REL);
	}
	
	//Secondary window for displaying monster
	WINDOW *w;
	w = newwin(24, 80, 0, 0);
	int begin = 1;
	int end = 0;
	if(ableToScroll){
		end = 24;
	} else {
		end = *num_Mon;
	}

	RST:;
	wrefresh(w);
	clear();
	if(*num_Mon == 1){
		printw("You can see %d monster\n", *num_Mon);
	} else {
		printw("You can see %d monsters\n", *num_Mon);
	}
	for(i = begin; i <= end; i++){
		printw("%s\n", COMP_REL[i - 1]);
	}

	int com;
	com = getch();
	switch(com){
		case KEY_DOWN: //Scroll down monster list
			if(!ableToScroll){
				goto RST;
			} else if(end + 1 <= *num_Mon){ //if there is more at the end of the list then scroll down to view it. 
				begin++;
				end++;
			} else {
				mvprintw(10, 40, "No more monsters to view below!");
			}
			
			goto RST;
			break;
		case KEY_UP: //Scroll up monster list
			if(!ableToScroll){
				goto RST;
			} else if(begin - 1 >= 0){ //if there is more at the beginning of the list then scroll up to view it. 
				begin--;
				end--;
			} else {
				mvprintw(10, 40, "No more monsters to view above!");
			}

			goto RST;
			break;
		case 27: //quit viewing monster list
			break;
		default: //print commands
			mvprintw(10, 40, "COMMANDS:\n<UP ARROW>: Scroll up list\n<DOWN ARROW>: Scroll down list\n<ESC>: Return to game\n");
			goto RST;
	}
	delwin(w);
}

//function deletes a monster and updates the characters array
int deleteMonster(char dungeon[WINDOW_Y][WINDOW_X], Character_t characters[MAX_MONSTERS], int newXpos, int newYpos, int *num_Mon, int isTeleporting){
	int i, j;
	if(!isTeleporting){ //not teleporting so add PC checking for new position
		//skip 0: PC location
		for(i = 0; i < MAX_MONSTERS; i++){ //loops through all our characters
			if(newYpos == characters[i].pos[Y_LOC] && newXpos == characters[i].pos[X_LOC] && characters[i].a){ //checks if the new position is occupied by a monster
				dungeon[newYpos][newXpos] = characters[i].lv; //Replace the pos with the dungeon char
				characters[i].a = false;
				characters[i].pos[X_LOC] = 0;
				characters[i].pos[Y_LOC] = 0;
				return 1;
			}
		}
	} else { //is teleporting so skip the PC for the new position
		for(i = 1; i < MAX_MONSTERS; i++){ //loops through all our characters
			if(newYpos == characters[i].pos[Y_LOC] && newXpos == characters[i].pos[X_LOC] && characters[i].a){ //checks if the new position is occupied by a monster
				dungeon[newYpos][newXpos] = characters[i].lv; //Replace the pos with the dungeon char
				characters[i].a = false;
				characters[i].pos[X_LOC] = 0;
				characters[i].pos[Y_LOC] = 0;
				return 1;
			}
		}
	}
	return 0;
}

void generateNewFloor(char dungeon[WINDOW_Y][WINDOW_X], int hardness[GAME_HEIGHT][GAME_WIDTH], int roomMap[GAME_HEIGHT][GAME_WIDTH], 
						int wholeMap[GAME_HEIGHT][GAME_WIDTH], int PC[2], int upwardCases[MAX_ROOMS][2], int downwardCases[MAX_ROOMS][2], 
						int *number_of_upstairs, int *number_of_downstairs, int rooms[MAX_ROOMS][MAX_CONSTRAINTS], int *number_of_rooms,						
						int generatingNewDung, int *numMon, Character_t characters[MAX_MONSTERS], char fogDungeon[GAME_HEIGHT][GAME_WIDTH]){
	int i;
	if(generatingNewDung){
	    *number_of_rooms = 0;
		*number_of_downstairs = 0;
		*number_of_upstairs = 0;
		hardness[0][0] = 0;
	}
	generateDungeon(dungeon, hardness, fogDungeon);
	generateRooms(rooms, dungeon, hardness, number_of_rooms);
	generateCorridors(dungeon, rooms, number_of_rooms, hardness, number_of_upstairs);
	generateStairs(dungeon, hardness, upwardCases, downwardCases, number_of_upstairs, number_of_downstairs, PC);
	//creates new heat maps if the PC has been initiated
	if(!generatingNewDung){
		roomHeatMapGenerator(PC, roomMap, hardness);
		//printRoomHeatMap(roomMap, PC);
		wholeHeatMapGenerator(PC, wholeMap, hardness);
		//printWholeHeatMap(wholeMap, PC);
	}
}

//function to control the PC movement from the User
void User_Input(Character_t characters[MAX_MONSTERS], int *num_Mon, char dungeon[WINDOW_Y][WINDOW_X], int hardness[GAME_HEIGHT][GAME_WIDTH], int roomMap[GAME_HEIGHT][GAME_WIDTH], 
						int wholeMap[GAME_HEIGHT][GAME_WIDTH], int PC[2], int upwardCases[MAX_ROOMS][2], int downwardCases[MAX_ROOMS][2], 
						int *number_of_upstairs, int *number_of_downstairs, int rooms[MAX_ROOMS][MAX_CONSTRAINTS], int *number_of_rooms, int *gameStatus,
						char fogDungeon[GAME_HEIGHT][GAME_WIDTH], int *num_Equip, Equipment equipment[MAX_EQUIPMENT]){
	int row, col, i, cmnd, randX, randY, j, maxLoop;
	int errorMessage = -1;
	int key;
	int isError = 0;
	
	
	if(errorMessage == 3){
		CMD:;
		clear();
		mvprintw(0, 0, "Commands:\n<7 or y>: Move PC Up-Left\n<8 or k>: Move PC Up\n<9 or u>: Move PC Up-Right\n<6 or l>: Move PC Right\n<3 or n>: Move PC Down-Right\n<2 or j>: Move PC Down\n<1 or b>: Move PC Down-Left\n<4 or h>: Move PC Left\n<'>'>: Go Down Stairs\n<'<'>: Go Up Stairs\n<5 or space or .>: Rest PC\n<m>: Display Monsters\n<Q>: Quit\n\nPress any key to exit");
		key = getch();
		switch(key){
			default:
				goto DFT;
				break;
		}
	}
	
	DFT:;
	refresh();
	clear();
	//updates fog map
	for(row = characters[0].pos[Y_LOC] - 2; row < characters[0].pos[Y_LOC] + 3; row++){
		for(col = characters[0].pos[X_LOC] - 2; col < characters[0].pos[X_LOC] + 3; col++){
			fogDungeon[row][col] = dungeon[row][col];
		}
	}
	
	//Pastes the fog dungeon
	for(row = 0; row < GAME_HEIGHT; row++){
		for(col = 0; col < GAME_WIDTH; col++){
			if(fogDungeon[row][col] == '.' || fogDungeon[row][col] == '#' || fogDungeon[row][col] == ' ' || fogDungeon[row][col] == '|' 
				|| fogDungeon[row][col] == '-' || fogDungeon[row][col] == '@' || fogDungeon[row][col] == '<' || fogDungeon[row][col] == '>'){ //always white
				mvaddch(row, col, fogDungeon[row][col]);
			} else {
				j = 0;
				//Could use macro here to speed up but forgot how to and I'm lazy
				if(*num_Mon > *num_Equip){
					maxLoop = *num_Mon;
				} else {
					maxLoop = *num_Equip;
				}
				//REALLY LENGTHY WAY OF DOING THIS, HAVE TO BE AN EASIER TO WAY TO QUICKLY LOOK UP WHAT COLOR AND CHARACTER IS THERE
				while(j < maxLoop){
					if(characters[j].pos[Y_LOC] == row && characters[j].pos[X_LOC] == col && characters[j].a){ //generates monsters with colors
						if(characters[j].color.BLACK){
							attron(COLOR_PAIR(COLOR_BLACK));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_BLACK));
						} else if(characters[j].color.RED){
							attron(COLOR_PAIR(COLOR_RED));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_RED));
						} else if(characters[j].color.GREEN){
							attron(COLOR_PAIR(COLOR_GREEN));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_GREEN));
						} else if(characters[j].color.YELLOW){
							attron(COLOR_PAIR(COLOR_YELLOW));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_YELLOW));
						} else if(characters[j].color.BLUE){
							attron(COLOR_PAIR(COLOR_BLUE));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_BLUE));
						} else if(characters[j].color.MAGENTA){
							attron(COLOR_PAIR(COLOR_MAGENTA));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_MAGENTA));
						} else if(characters[j].color.CYAN){
							attron(COLOR_PAIR(COLOR_CYAN));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_CYAN));
						} else if(characters[j].color.WHITE){
							attron(COLOR_PAIR(COLOR_WHITE));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_WHITE));
						}
					} else if(equipment[j].pos[Y_LOC] == row && equipment[j].pos[X_LOC] == col && equipment[j].a){ //generates equipment with colors
						if(equipment[j].color.BLACK){
							attron(COLOR_PAIR(COLOR_BLACK));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_BLACK));
						} else if(equipment[j].color.RED){
							attron(COLOR_PAIR(COLOR_RED));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_RED));
						} else if(equipment[j].color.GREEN){
							attron(COLOR_PAIR(COLOR_GREEN));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_GREEN));
						} else if(equipment[j].color.YELLOW){
							attron(COLOR_PAIR(COLOR_YELLOW));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_YELLOW));
						} else if(equipment[j].color.BLUE){
							attron(COLOR_PAIR(COLOR_BLUE));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_BLUE));
						} else if(equipment[j].color.MAGENTA){
							attron(COLOR_PAIR(COLOR_MAGENTA));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_MAGENTA));
						} else if(equipment[j].color.CYAN){
							attron(COLOR_PAIR(COLOR_CYAN));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_CYAN));
						} else if(equipment[j].color.WHITE){
							attron(COLOR_PAIR(COLOR_WHITE));
							mvaddch(row, col, fogDungeon[row][col]);
							attroff(COLOR_PAIR(COLOR_WHITE));
						} 
					}
					j++;
				}
			}
		}
	}

	ER:;
	
	if(errorMessage == 0){
		mvprintw(MESSAGES, 0, "YOU ARE RUNNING INTO A WALL! GO IN A NEW DIRECTION!\n");
	} else if(errorMessage == 1){
		mvprintw(MESSAGES, 0, "YOU ARE NOT ON A DOWN STAIR CASE!\n");
	} else if(errorMessage == 2){
		mvprintw(MESSAGES, 0, "YOU ARE NOT ON AN UP STAIR CASE!\n");
	}
	
	key = getch();
	switch(key) {
		case 'y': //UP-LEFT
			Y:;
			if(!hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC] - 1]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC] - 1, characters[0].pos[Y_LOC] - 1, num_Mon, 0);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[Y_LOC] -= 1; //Update Y
				characters[0].pos[X_LOC] -= 1; //Update X
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				errorMessage = 0;
				goto ER;
			}
			break;
		case '7': //UP-LEFT
			goto Y; //Goes to default UP-LEFT case
			break;
		case KEY_HOME: //UP-LEFT
			goto Y; //Goes to default UP-LEFT case
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case 'k': //UP
			K:;
			
			if(!hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC]]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC], characters[0].pos[Y_LOC] - 1, num_Mon, 0);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[Y_LOC] -= 1; //Update Y
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				errorMessage = 0;
				goto ER;
			}
			break;
		case '8': //UP
			goto K; //Goes to default UP case
			break;
		case KEY_UP: //UP
			goto K; //Goes to default UP case
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case 'u': //UP-RIGHT
			U:;
			if(!hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC] + 1]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC] + 1, characters[0].pos[Y_LOC] - 1, num_Mon, 0);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[Y_LOC] -= 1; //Update Y
				characters[0].pos[X_LOC] += 1; //Update X
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				errorMessage = 0;
				goto ER;
			}
			break;
		case '9': //UP-RIGHT
			goto U; //Goes to default UP-RIGHT case
			break;
		case KEY_PPAGE: //UP-RIGHT
			goto U; //Goes to default UP-RIGHT case
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case 'l': //RIGHT
			L:;
			if(!hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] + 1]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC] + 1, characters[0].pos[Y_LOC], num_Mon, 0);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[X_LOC] += 1; //Update X
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon

			} else {
				errorMessage = 0;
				goto ER;
				
			}
			break;
		case '6': //RIGHT
			goto L; //Goes to default RIGHT case
			break;
		case KEY_RIGHT: //RIGHT
			goto L; //Goes to default RIGHT case
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case 'n': //DOWN-RIGHT
			N:;
			
			if(!hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC] + 1]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC] + 1, characters[0].pos[Y_LOC] + 1, num_Mon, 0);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[Y_LOC] += 1; //Update Y
				characters[0].pos[X_LOC] += 1; //Update X
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				errorMessage = 0;
				goto ER;
			}
			break;
		case '3': //DOWN-RIGHT
			goto N; //Goes to default DOWN-RIGHT case
			break;
		case KEY_NPAGE: //DOWN-RIGHT
			goto N; //Goes to default DOWN-RIGHT case 
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case 'j': //DOWN
			J:;
			if(!hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC]]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC], characters[0].pos[Y_LOC] + 1, num_Mon, 0);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[Y_LOC] += 1; //Update Y
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				errorMessage = 0;
				goto ER;
			}
			break;
		case '2': //DOWN
			goto J; //Goes to default DOWN case
			break;
		case KEY_DOWN: //DOWN
			goto J; //Goes to default DOWN case
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case 'b': //DOWN-LEFT
			B:;
			if(!hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC] - 1]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC] - 1, characters[0].pos[Y_LOC] + 1, num_Mon, 0);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[Y_LOC] += 1; //Update Y
				characters[0].pos[X_LOC] -= 1; //Update X
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				errorMessage = 0;
				goto ER;
			}
			break;
		case '1': //DOWN-LEFT
			goto B; //Goes to default DOWN-LEFT case
			break;
		case KEY_END: //DOWN-LEFT
			goto B; //Goes to default DOWN-LEFT case
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case 'h': //LEFT
			H:;
			if(!hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] - 1]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC] - 1, characters[0].pos[Y_LOC], num_Mon, 0);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[X_LOC] -= 1; //Update X
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				errorMessage = 0;
				goto ER;
			}
			break;
		case '4': //LEFT
			goto H; //Goes to default LEFT case
			break;
		case KEY_LEFT: //LEFT
			goto H; //Goes to default LEFT case
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case '.': //REST
			R:;
			//PC doesn't do anything
			break;
		case ' ': //REST
			goto R; //Goes to default REST case
			break;
		case '5': //REST
			goto R; //Goes to default REST case
			break;
		case KEY_B2: //REST
			goto R; //Goes to default REST case
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case '>': //attempts to go downstairs
			if(characters[0].lv == '>'){
				generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, number_of_upstairs, number_of_downstairs, rooms, number_of_rooms, 1, num_Mon, characters, fogDungeon);
				*gameStatus = 2;
			} else {
				errorMessage = 1;
				goto ER;
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case '<': //attempts to go upstairs
			if(characters[0].lv == '<'){
				generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, number_of_upstairs, number_of_downstairs, rooms, number_of_rooms, 1, num_Mon, characters, fogDungeon);
				*gameStatus = 2;
			} else {
				errorMessage = 2;
				goto ER;
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case 'm': //Print Monsters
			printMonsters(characters, num_Mon);
			goto DFT;
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case 'Q': //Quit game
			*gameStatus = 1;
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case 'f':
			WINDOW *w;
			w = newwin(24, 80, 0, 0);
			wrefresh(w);
			clear();
			for(row = 0; row < GAME_HEIGHT; row++){
				for(col = 0; col < GAME_WIDTH; col++){
					if(dungeon[row][col] == '.' || dungeon[row][col] == '#' || dungeon[row][col] == ' ' || dungeon[row][col] == '|' 
						|| dungeon[row][col] == '-' || dungeon[row][col] == '@' || dungeon[row][col] == '<' || dungeon[row][col] == '>'){ //always white
						mvaddch(row, col, dungeon[row][col]);
					} else {
						j = 0;
						//Could use macro here to speed up but forgot how to and I'm lazy
						if(*num_Mon > *num_Equip){
							maxLoop = *num_Mon;
						} else {
							maxLoop = *num_Equip;
						}
						//REALLY LENGTHY WAY OF DOING THIS, HAVE TO BE AN EASIER TO WAY TO QUICKLY LOOK UP WHAT COLOR AND CHARACTER IS THERE
						while(j < maxLoop){
							if(characters[j].pos[Y_LOC] == row && characters[j].pos[X_LOC] == col && characters[j].a){ //generates monsters with colors
								if(characters[j].color.BLACK){
									attron(COLOR_PAIR(COLOR_BLACK));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_BLACK));
								} else if(characters[j].color.RED){
									attron(COLOR_PAIR(COLOR_RED));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_RED));
								} else if(characters[j].color.GREEN){
									attron(COLOR_PAIR(COLOR_GREEN));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_GREEN));
								} else if(characters[j].color.YELLOW){
									attron(COLOR_PAIR(COLOR_YELLOW));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_YELLOW));
								} else if(characters[j].color.BLUE){
									attron(COLOR_PAIR(COLOR_BLUE));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_BLUE));
								} else if(characters[j].color.MAGENTA){
									attron(COLOR_PAIR(COLOR_MAGENTA));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_MAGENTA));
								} else if(characters[j].color.CYAN){
									attron(COLOR_PAIR(COLOR_CYAN));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_CYAN));
								} else if(characters[j].color.WHITE){
									attron(COLOR_PAIR(COLOR_WHITE));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_WHITE));
								}
							} else if(equipment[j].pos[Y_LOC] == row && equipment[j].pos[X_LOC] == col && equipment[j].a){ //generates equipment with colors
								if(equipment[j].color.BLACK){
									attron(COLOR_PAIR(COLOR_BLACK));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_BLACK));
								} else if(equipment[j].color.RED){
									attron(COLOR_PAIR(COLOR_RED));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_RED));
								} else if(equipment[j].color.GREEN){
									attron(COLOR_PAIR(COLOR_GREEN));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_GREEN));
								} else if(equipment[j].color.YELLOW){
									attron(COLOR_PAIR(COLOR_YELLOW));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_YELLOW));
								} else if(equipment[j].color.BLUE){
									attron(COLOR_PAIR(COLOR_BLUE));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_BLUE));
								} else if(equipment[j].color.MAGENTA){
									attron(COLOR_PAIR(COLOR_MAGENTA));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_MAGENTA));
								} else if(equipment[j].color.CYAN){
									attron(COLOR_PAIR(COLOR_CYAN));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_CYAN));
								} else if(equipment[j].color.WHITE){
									attron(COLOR_PAIR(COLOR_WHITE));
									mvaddch(row, col, dungeon[row][col]);
									attroff(COLOR_PAIR(COLOR_WHITE));
								} 
							}
							j++;
						}
					}
				}
			}
			mvprintw(MESSAGES, 0, "PRESS ANY KEY TO EXIT AND GO TO NEXT TURN\n");
			key = getch();
			switch(key){
				default:
					break;
			}
			delwin(w);
			break;
		case 't':
			// teleportPC(dungeon, fogDungeon, hardness, characters, num_Mon);
			//change the PC character to the dungeon character, update the specs later after user has chosen position to move
			dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv;
			fogDungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv;
			int cmnd;
			BSC:;
			refresh();
			clear();
			for(row = 0; row < GAME_HEIGHT; row++){
				for(col = 0; col < GAME_WIDTH; col++){
					if(row == characters[0].pos[Y_LOC] && col == characters[0].pos[X_LOC]){ //print '*' for the place the user plans on moving
						mvaddch(row, col, '*');
					} else { //print normal dungeon
						if(dungeon[row][col] == '.' || dungeon[row][col] == '#' || dungeon[row][col] == ' ' || dungeon[row][col] == '|' 
						|| dungeon[row][col] == '-' || dungeon[row][col] == '@' || dungeon[row][col] == '<' || dungeon[row][col] == '>'){ //always white
						mvaddch(row, col, dungeon[row][col]);
						} else {
							j = 0;
							//Could use macro here to speed up but forgot how to and I'm lazy
							if(*num_Mon > *num_Equip){
								maxLoop = *num_Mon;
							} else {
								maxLoop = *num_Equip;
							}
							//REALLY LENGTHY WAY OF DOING THIS, HAVE TO BE AN EASIER TO WAY TO QUICKLY LOOK UP WHAT COLOR AND CHARACTER IS THERE
							while(j < maxLoop){
								if(characters[j].pos[Y_LOC] == row && characters[j].pos[X_LOC] == col && characters[j].a){ //generates monsters with colors
									if(characters[j].color.BLACK){
										attron(COLOR_PAIR(COLOR_BLACK));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_BLACK));
									} else if(characters[j].color.RED){
										attron(COLOR_PAIR(COLOR_RED));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_RED));
									} else if(characters[j].color.GREEN){
										attron(COLOR_PAIR(COLOR_GREEN));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_GREEN));
									} else if(characters[j].color.YELLOW){
										attron(COLOR_PAIR(COLOR_YELLOW));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_YELLOW));
									} else if(characters[j].color.BLUE){
										attron(COLOR_PAIR(COLOR_BLUE));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_BLUE));
									} else if(characters[j].color.MAGENTA){
										attron(COLOR_PAIR(COLOR_MAGENTA));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_MAGENTA));
									} else if(characters[j].color.CYAN){
										attron(COLOR_PAIR(COLOR_CYAN));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_CYAN));
									} else if(characters[j].color.WHITE){
										attron(COLOR_PAIR(COLOR_WHITE));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_WHITE));
									}
								} else if(equipment[j].pos[Y_LOC] == row && equipment[j].pos[X_LOC] == col && equipment[j].a){ //generates equipment with colors
									if(equipment[j].color.BLACK){
										attron(COLOR_PAIR(COLOR_BLACK));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_BLACK));
									} else if(equipment[j].color.RED){
										attron(COLOR_PAIR(COLOR_RED));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_RED));
									} else if(equipment[j].color.GREEN){
										attron(COLOR_PAIR(COLOR_GREEN));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_GREEN));
									} else if(equipment[j].color.YELLOW){
										attron(COLOR_PAIR(COLOR_YELLOW));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_YELLOW));
									} else if(equipment[j].color.BLUE){
										attron(COLOR_PAIR(COLOR_BLUE));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_BLUE));
									} else if(equipment[j].color.MAGENTA){
										attron(COLOR_PAIR(COLOR_MAGENTA));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_MAGENTA));
									} else if(equipment[j].color.CYAN){
										attron(COLOR_PAIR(COLOR_CYAN));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_CYAN));
									} else if(equipment[j].color.WHITE){
										attron(COLOR_PAIR(COLOR_WHITE));
										mvaddch(row, col, dungeon[row][col]);
										attroff(COLOR_PAIR(COLOR_WHITE));
									} 
								}
								j++;
							}
						}
					}
				}
			}
			if(isError){
				OOP:;
				clear();
				mvprintw(0, 0, "Commands:\n<7 or y>: Move Cursor Up-Left\n<8 or k>: Move Cursor Up\n<9 or u>: Move Cursor Up-Right\n<6 or l>: Move Cursor Right\n<3 or n>: Move Cursor Down-Right\n<2 or j>: Move Cursor Down\n<1 or b>: Move Cursor Down-Left\n<4 or h>: Move Cursor Left\n<t>: Teleport PC to cursor location\n<r>: Randomly teleport the PC\n\nPress any key to exit");
				cmnd = getch();
				switch(cmnd){
					default:
						isError = 0;
						goto BSC;
						break;
				}
			}
			cmnd = getch();
			switch(cmnd){
				case 'y': //UP-LEFT
					UL:;
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC] - 1] != 255){
						characters[0].pos[Y_LOC] -= 1;
						characters[0].pos[X_LOC] -= 1;
					}
					goto BSC;
					break;
				case '7': //UP-LEFT
					goto UL; //Goes to default UP-LEFT case
					break;
				case KEY_HOME: //UP-LEFT
					goto UL; //Goes to default UP-LEFT case
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case 'k': //UP
					UP:;
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC]] != 255){
						characters[0].pos[Y_LOC] -= 1;
					}
					goto BSC;
					break;
				case '8': //UP
					goto UP; //Goes to default UP case
					break;
				case KEY_UP: //UP
					goto UP; //Goes to default UP case
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case 'u': //UP-RIGHT
					UR:;
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC] + 1] != 255){
						characters[0].pos[Y_LOC] -= 1;
						characters[0].pos[X_LOC] += 1;
					}
					goto BSC;
					break;
				case '9': //UP-RIGHT
					goto UR; //Goes to default UP-RIGHT case
					break;
				case KEY_PPAGE: //UP-RIGHT
					goto UR; //Goes to default UP-RIGHT case
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case 'l': //RIGHT
					RG:;
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] + 1] != 255){
						characters[0].pos[X_LOC] += 1;
					}
					goto BSC;
					break;
				case '6': //RIGHT
					goto RG; //Goes to default RIGHT case
					break;
				case KEY_RIGHT: //RIGHT
					goto RG; //Goes to default RIGHT case
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case 'n': //DOWN-RIGHT
					DR:;
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC] + 1] != 255){
						characters[0].pos[Y_LOC] += 1;
						characters[0].pos[X_LOC] += 1;
					}
					goto BSC;
					break;
				case '3': //DOWN-RIGHT
					goto DR; //Goes to default DOWN-RIGHT case
					break;
				case KEY_NPAGE: //DOWN-RIGHT
					goto DR; //Goes to default DOWN-RIGHT case 
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case 'j': //DOWN
					D:;
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC]] != 255){
						characters[0].pos[Y_LOC] += 1;
					}
					goto BSC;
					break;
				case '2': //DOWN
					goto D; //Goes to default DOWN case
					break;
				case KEY_DOWN: //DOWN
					goto D; //Goes to default DOWN case
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case 'b': //DOWN-LEFT
					DL:;
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC] - 1] != 255){
						characters[0].pos[Y_LOC] += 1;
						characters[0].pos[X_LOC] -= 1;
					}
					goto BSC;
					break;
				case '1': //DOWN-LEFT
					goto DL; //Goes to default DOWN-LEFT case
					break;
				case KEY_END: //DOWN-LEFT
					goto DL; //Goes to default DOWN-LEFT case
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case 'h': //LEFT
					LF:;
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] - 1] != 255){
						characters[0].pos[X_LOC] -= 1;
					}
					goto BSC;
					break;
				case '4': //LEFT
					goto LF; //Goes to default LEFT case
					break;
				case KEY_LEFT: //LEFT
					goto LF; //Goes to default LEFT case
					break;
				case 'r': //random location
					//with these random values we can never reach the immutable rock
					randY = 2 + rand() % (GAME_HEIGHT - 3);
					randX = 2 + rand() % (GAME_WIDTH - 3);
					characters[0].pos[Y_LOC] = randY; //apply new pos to the PC
					characters[0].pos[X_LOC] = randX; //apply new pos to the PC
					deleteMonster(dungeon, characters, characters[0].pos[X_LOC], characters[0].pos[Y_LOC], num_Mon, 1); //Checks if space is occupied by a monster
					if(dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] == ' '){
						characters[0].lv = '#';
					} else {
						characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]];
					}
					dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //put PC in dungeon
					//updates fog map for user
					for(row = characters[0].pos[Y_LOC] - 2; row < characters[0].pos[Y_LOC] + 3; row++){
						for(col = characters[0].pos[X_LOC] - 2; col < characters[0].pos[X_LOC] + 3; col++){
							fogDungeon[row][col] = dungeon[row][col];
						}
					}
					break;
				case 't': //user chosen position
					deleteMonster(dungeon, characters, characters[0].pos[X_LOC], characters[0].pos[Y_LOC], num_Mon, 1); //Checks if space is occupied by a monster
					characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //update PC's lv
					dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update the dungeon with the new PC location
					//updates fog map for user
					for(row = characters[0].pos[Y_LOC] - 2; row < characters[0].pos[Y_LOC] + 3; row++){
						for(col = characters[0].pos[X_LOC] - 2; col < characters[0].pos[X_LOC] + 3; col++){
							fogDungeon[row][col] = dungeon[row][col];
						}
					}
					break;
				default:
					isError = 1;
					goto OOP;
					break;
			}
			break;
		default:
			errorMessage = 3;
			goto CMD;
			break;
	}
	refresh();
}

void inSight(char dungeon[WINDOW_Y][WINDOW_X], Character_t *character, int rooms[MAX_ROOMS][MAX_CONSTRAINTS], int *number_of_rooms, int PC[2]){
	int inSight = FALSE;
	int pcInRoom = FALSE;
	int	i, j;
	
	if(!character->t || character->c == '@'){ //skip this if the character is telepathic
		/*
		Checks if PC is in the same room as the monster. In which case the monster, telepathic or not, will have the PC's last location
		*/
		for(i = 0; i < *number_of_rooms; i++){
			if((PC[X_LOC] >= rooms[i][X_LOC] && PC[Y_LOC] >= rooms[i][Y_LOC]) && 
				(PC[X_LOC] <= (rooms[i][X_LOC] + rooms[i][X_LEN]) && PC[Y_LOC] <= (rooms[i][Y_LOC] + rooms[i][Y_LEN]))){
					pcInRoom = TRUE;
				if((character->pos[X_LOC] >= rooms[i][X_LOC] && character->pos[Y_LOC] >= rooms[i][Y_LOC]) && 
					(character->pos[X_LOC] <= (rooms[i][X_LOC] + rooms[i][X_LEN]) && character->pos[Y_LOC] <= (rooms[i][Y_LOC] + rooms[i][Y_LEN]))){
					inSight = TRUE;
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
	
	if(pcInRoom && character->c == '@'){
		character->lv = '.';
	} else if(character->c == '@'){
		character->lv = '#';
	}
}

void generateMonsters(char dungeon[WINDOW_Y][WINDOW_X], int hardness[GAME_HEIGHT][GAME_WIDTH], int PC[2], 
					int *num_Mon, Character_t characters[MAX_MONSTERS], int rooms[MAX_ROOMS][MAX_CONSTRAINTS], int *number_of_rooms){
	int i = 0;
	int randomX, randomY;
	bool monsterPlaced = false;
	while(i <= *num_Mon){ //generate all the monsters and their characteristics
		/*
		PC is set not to move AT ALL
		*/
		if(i == 0){
			characters[0].c = '@';
			// characters[0].NPC = NULL;
			characters[0].pos[X_LOC] = PC[X_LOC];
			characters[0].pos[Y_LOC] = PC[Y_LOC];
			characters[0].s = 10;
			characters[0].nt = 0;
			characters[0].sn = 0;
			characters[i].a = true;
		} else {
			int rarityNum = rand() % 100;
			if(rarityNum < characters[i].rarity && characters[i].a){ //if the rarity num is less than the monsters rarity and its alive
				// characters[i].PC = NULL;
				characters[i].nt = 0;
				characters[i].sn = i;
				/*
				Monster is placed randomly INSIDE one of the open spaces
				*/
				while(!monsterPlaced){
					randomY = 1 + rand() % GAME_HEIGHT - 1;
					randomX = 1 + rand() % GAME_WIDTH - 2;
					if(!hardness[randomY][randomX] && ((dungeon[randomY][randomX] == '.' || dungeon[randomY][randomX] == '#'))){	
						characters[i].lv = dungeon[randomY][randomX];
						dungeon[randomY][randomX] = characters[i].c;
						characters[i].pos[X_LOC] = randomX;
						characters[i].pos[Y_LOC] = randomY;
						monsterPlaced = true;
						characters[i].a = true;
					}
				}
				monsterPlaced = false;
				inSight(dungeon, characters, rooms, number_of_rooms, PC); //check if the monster can see the PC OR PC is in a room
			} else {
				characters[i].a = false;
			}
		}
		i++;
	}
}

void generateEquipment(char dungeon[WINDOW_Y][WINDOW_X], int hardness[GAME_HEIGHT][GAME_WIDTH], int *num_Equip, 
						Equipment equipment[MAX_MONSTERS]){
	int i = 0;
	int	numEquip = 0;
	int minEquip = 10;
	int randomX, randomY;
	bool equipPlaced = false;
	while(i <= *num_Equip && numEquip < minEquip){ //generate all the equipment and place them if rarity allows
		int rarityNum = rand() % 100;
		if(rarityNum < equipment[i].rarity && equipment[i].a){ //if the rarity num is less than the monsters rarity and its alive
			/*
			Object is randomly placed INSIDE one of the open spaces
			*/
			while(!equipPlaced){
				randomY = 1 + rand() % GAME_HEIGHT - 1;
				randomX = 1 + rand() % GAME_WIDTH - 2;
				if(!hardness[randomY][randomX] && ((dungeon[randomY][randomX] == '.' || dungeon[randomY][randomX] == '#'))){	
					equipment[i].lv = dungeon[randomY][randomX];
					dungeon[randomY][randomX] = equipment[i].c;
					equipment[i].pos[X_LOC] = randomX;
					equipment[i].pos[Y_LOC] = randomY;
					equipPlaced = true;
					numEquip++;
				}
			}
			equipPlaced = false;
		}
		i++;
		if(i == *num_Equip && numEquip < minEquip){ //looped through once, go through again to get at least 10 items
			i = 0;
		}
	}
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
	} else { //TOP LEFT CORNER
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
void simulateGame(Character_t characters[MAX_MONSTERS], int *num_Mon, char dungeon[WINDOW_Y][WINDOW_X], int hardness[GAME_HEIGHT][GAME_WIDTH], int roomMap[GAME_HEIGHT][GAME_WIDTH], 
						int wholeMap[GAME_HEIGHT][GAME_WIDTH], int PC[2], int upwardCases[MAX_ROOMS][2], int downwardCases[MAX_ROOMS][2], 
						int *number_of_upstairs, int *number_of_downstairs, int rooms[MAX_ROOMS][MAX_CONSTRAINTS], int *number_of_rooms,
						char fogDungeon[GAME_HEIGHT][GAME_WIDTH], int *num_Equip, Equipment equipment[MAX_EQUIPMENT]){
	heap_t h;
	heap_init(&h, compare_characters, NULL);
	static Character_t *p;
	int i, j, randX, randY;
	int moveXdir = 0;
	int moveYdir = 0;
	int isErratic = FALSE;
	int gameStatus = 0;
	bool gameOver = true;
	
	generateMonsters(dungeon, hardness, PC, num_Mon, characters, rooms, number_of_rooms); //generate the PC and the monsters
	generateEquipment(dungeon, hardness, num_Equip, equipment); //generates and places the equipment
 	
	int inSight = FALSE;
	//Insert all the initial characters
	for(i = 0; i <= *num_Mon; i++){
		if(characters[i].a && characters[i].pos[X_LOC] && characters[i].pos[Y_LOC]){ //adds monster to the movement queue if it is alive with rarity applied
			characters[i].hn = heap_insert(&h, &characters[i]);
		}
	}
	
	//While the PC is alive, move monsters
	while(characters[0].a && gameStatus == 0){
		p = (Character_t *) heap_remove_min(&h); //pull the next moving monster to move him
		if(p->a){
			p->hn = NULL;
			int i, moveTo[2];
							
			//If the node pulled out is the PC then print the updated dungeon and pause
			if(p->c == '@'){
				//Runs commands for moving the PC
				User_Input(characters, num_Mon, dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, number_of_upstairs, number_of_downstairs, rooms, number_of_rooms, &gameStatus, fogDungeon, num_Equip, equipment);
				for(int k = 0; k < WINDOW_Y; k++){ //bad way of checking if there are not monsters in the dungeon
					for(int l = 0; l < WINDOW_X; l++){
						int temp = (int) dungeon[k][l];
						if(isalpha(temp) || gameStatus == 2){
							gameOver = false;
						}
 					}
				}
				if(gameOver){
					gameStatus = 3;
				}
				gameOver = true;
				if(gameStatus == 2){
					for(i = 0; i < MAX_MONSTERS; i++){
						heap_remove_min(&h); //remove all of the old monsters from the heap
						if(!characters[i].u){ //if the monsters aren't unique then bring them back to life for the next level
							characters[i].a = true;
						}
					}
					generateMonsters(dungeon, hardness, PC, num_Mon, characters, rooms, number_of_rooms); //generate the new monsters
					generateEquipment(dungeon, hardness, num_Equip, equipment); //generates and places the equipment
					//Insert all the new characters
					for(i = 0; i <= *num_Mon; i++){
						if(characters[i].a && characters[i].c){
							characters[i].hn = heap_insert(&h, &characters[i]);	
						}
					}
					gameStatus = 0; //sets game back to normal running mode
				}
				//Updates heat maps after every PC move, no matter if it stands still. Should change later but simple solution
				roomHeatMapGenerator(PC, roomMap, hardness);
				wholeHeatMapGenerator(PC, wholeMap, hardness);
			} else {
				if(p->pcLoc[X_LOC] && p->pcLoc[Y_LOC]){
					inSight = TRUE;
				}
				
				if(p->e){
					//If the character is erratic then this will determine whether it acts erratic on the next step
					isErratic = rand() % 2; //0: normal behavior, 1: erratic behavior
				}
				
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
				if(!isErratic){
					if(p->i && inSight){ //intelligent monster and telepathic or can see the PC
						if(characters[0].pos[X_LOC] > p->pos[X_LOC]){ //PC is to the right of the monster
							if(characters[0].pos[Y_LOC] > p->pos[Y_LOC]){ //DOWN-RIGHT
								scanWall(wholeMap, roomMap, 2, p, moveTo);
								moveXdir = moveTo[X_LOC];
								moveYdir = moveTo[Y_LOC];
							} else if (characters[0].pos[Y_LOC] < p->pos[Y_LOC]) { //UP-RIGHT
								scanWall(wholeMap, roomMap, 8, p, moveTo);
								moveXdir = moveTo[X_LOC];
								moveYdir = moveTo[Y_LOC];
							} else { //PC is only to the right of the monster so scan the right "wall" of vertices
								scanWall(wholeMap, roomMap, 1, p, moveTo);
								moveXdir = moveTo[X_LOC];
								moveYdir = moveTo[Y_LOC];
							}
						} else if (characters[0].pos[X_LOC] < p->pos[X_LOC]) { //PC is to the left of the monster
							if(characters[0].pos[Y_LOC] > p->pos[Y_LOC]){ //DOWN-LEFT
								scanWall(wholeMap, roomMap, 4, p, moveTo);
								moveXdir = moveTo[X_LOC];
								moveYdir = moveTo[Y_LOC];
							} else if (characters[0].pos[Y_LOC] < p->pos[Y_LOC]) { //UP-LEFT
								scanWall(wholeMap, roomMap, 6, p, moveTo);
								moveXdir = moveTo[X_LOC];
								moveYdir = moveTo[Y_LOC];
							} else { //PC is only to the left of the monster so scan the left "wall" of vertices
								scanWall(wholeMap, roomMap, 5, p, moveTo);
								moveXdir = moveTo[X_LOC];
								moveYdir = moveTo[Y_LOC];
							}
						} else { //Not intelligent
							if(characters[0].pos[Y_LOC] > p->pos[Y_LOC]){ //PC is directly below the monster
								scanWall(wholeMap, roomMap, 3, p, moveTo);
								moveXdir = moveTo[X_LOC];
								moveYdir = moveTo[Y_LOC];
							} else { //PC is directly above the monster
								scanWall(wholeMap, roomMap, 7, p, moveTo);
								moveXdir = moveTo[X_LOC];
								moveYdir = moveTo[Y_LOC];
							}
						}
					} else { //not intelligent or can't see the PC
						//default if the monster knows where the PC is but doens't have any special abilities
						if(inSight){
							//calculate the X move direction to the PC
							if(abs(p->pcLoc[X_LOC] - p->pos[X_LOC])){
								moveXdir = ((p->pcLoc[X_LOC] - p->pos[X_LOC]) / (abs(p->pcLoc[X_LOC] - p->pos[X_LOC])));
							}
							//calculate the Y move direction to the PC
							if(abs(p->pcLoc[Y_LOC] - p->pos[Y_LOC])){
								moveYdir = ((p->pcLoc[Y_LOC] - p->pos[Y_LOC]) / (abs(p->pcLoc[Y_LOC] - p->pos[Y_LOC])));
							}	
						}  else { //default erratic behavior for non-intelligent monsters. Intelligent monsters stay still, preserving "energy"
							if(!p->i){
								moveXdir = (rand() % 3) - 1;
								moveYdir = (rand() % 3) - 1;
							}	
						}
					}
				} else { //Erratic behavior
					moveXdir = (rand() % 3) - 1;
					moveYdir = (rand() % 3) - 1;
				}
				
				//Move the monster if the random new space is different from itself
				//and the new location is an open area
				//or it can tunnel through rock
				//Can move to any of the 8 surrounding cells
				if((moveYdir || moveXdir) && ((p->tu && hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] < 255) || !hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir])){
					//Checks if the new space is a monster
					//if the monster was deleted then don't do anything
					deleteMonster(dungeon, characters, p->pos[X_LOC] + moveXdir, p->pos[Y_LOC] + moveYdir, num_Mon, 0);
					//if the new location is an open room, corridor or staircase then update stats
					if(hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] == 0){ 
						dungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->lv; //replace monster with old room or corridor char
						if(fogDungeon[p->pos[Y_LOC]][p->pos[X_LOC]] != ' '){
							fogDungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->lv; //replace monster with old room or corridor char in fog dungeon to eliminate the double monster issue
						}						
						p->pos[X_LOC] += moveXdir; //update monster x location
						p->pos[Y_LOC] += moveYdir; //update monster y location
						p->lv = dungeon[p->pos[Y_LOC]][p->pos[X_LOC]];
						dungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->c; //replace dungeon character with new monster character
						if((p->pos[Y_LOC] == p->pcLoc[Y_LOC]) && (p->pos[X_LOC] == p->pcLoc[X_LOC])){ //Monster is in pc last seen position but it isn't there
							p->pcLoc[X_LOC] = 0; //remove x pc location
							p->pcLoc[Y_LOC] = 0; //remove y pc location
						}
					} else if(p->tu && hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir]){ //monster can tunnel so it starts "smashing" the wall
						hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] = hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] - 85; //update hardness
						if(hardness[p->pos[Y_LOC] + moveYdir][p->pos[X_LOC] + moveXdir] <= 0){ //mutable rock has been broken open so monster can go there
							dungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->lv; //replace monster with old room or corridor char
							if(fogDungeon[p->pos[Y_LOC]][p->pos[X_LOC]] != ' '){
								fogDungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->lv; //replace monster with old room or corridor char in fog dungeon to eliminate the double monster issue
							}
							p->lv = '#';
							p->pos[X_LOC] += moveXdir; //update monster x location
							p->pos[Y_LOC] += moveYdir; //update monster y location
							dungeon[p->pos[Y_LOC]][p->pos[X_LOC]] = p->c; //replace dungeon character with new monster character
							hardness[p->pos[Y_LOC]][p->pos[X_LOC]] = 0;
							//Update heat maps
							roomHeatMapGenerator(PC, roomMap, hardness);
							wholeHeatMapGenerator(PC, wholeMap, hardness);
						} //mutable rock still hasn't broken so do nothing
					} 
				}
			}
			
			p->nt = p->nt + 1000/p->s;
			heap_insert(&h, p);
		}
	}
	
	endwin();
	if(gameStatus == 0){
	const char *tombstone =
	  "\n\n\n\n                /\"\"\"\"\"/\"\"\"\"\"\"\".\n"
	  "               /     /         \\             __\n"
	  "              /     /           \\            ||\n"
	  "             /____ /   Rest in   \\           ||\n"
	  "            |     |    Pieces     |          ||\n"
	  "            |     |               |          ||\n"
	  "            |     |   A. Luser    |          ||\n"
	  "            |     |               |          ||\n"
	  "            |     |     * *   * * |         _||_\n"
	  "            |     |     *\\/* *\\/* |        | TT |\n"
	  "            |     |     *_\\_  /   ...\"\"\"\"\"\"| |"
	  "| |.\"\"....\"\"\"\"\"\"\"\".\"\"\n"
	  "            |     |         \\/..\"\"\"\"\"...\"\"\""
	  "\\ || /.\"\"\".......\"\"\"\"...\n"
	  "            |     |....\"\"\"\"\"\"\"........\"\"\"\"\""
	  "\"^^^^\".......\"\"\"\"\"\"\"\"..\"\n"
	  "            |......\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"......"
	  "..\"\"\"\"\"....\"\"\"\"\"..\"\"...\"\"\".\n\n"
	  "            You're dead.  Better luck in the next life.\n\n\n";
	  printf("%s\n", tombstone);
	} else if(gameStatus == 1) {
			char *quit = (char *)
		" /$$     /$$                         /$$$$$$            /$$   /$$    /$$\n"
		"|  $$   /$$/                        /$$__  $$          |__/  | $$   | $$\n"
		" \\  $$ /$$//$$$$$$  /$$   /$$      | $$  \\ $$ /$$   /$$ /$$ /$$$$$$ | $$\n"
		"  \\  $$$$//$$__  $$| $$  | $$      | $$  | $$| $$  | $$| $$|_  $$_/ | $$\n"
		"   \\  $$/| $$  \\ $$| $$  | $$      | $$  | $$| $$  | $$| $$  | $$   |__/\n"
		"    | $$ | $$  | $$| $$  | $$      | $$/$$ $$| $$  | $$| $$  | $$ /$$   \n"
		"    | $$ |  $$$$$$/|  $$$$$$/      |  $$$$$$/|  $$$$$$/| $$  |  $$$$//$$\n"
		"    |__/  \\______/  \\______/        \\____ $$$ \\______/ |__/   \\___/ |__/\n"
		"                                         \\__/                           \n"
		"                                                                        \n"
		"                                                                        \n";
		printf("%s\n", quit);
	} else {
				const char *victory =
	  "\n                                       o\n"
	  "                                      $\"\"$o\n"
	  "                                     $\"  $$\n"
	  "                                      $$$$\n"
	  "                                      o \"$o\n"
	  "                                     o\"  \"$\n"
	  "                oo\"$$$\"  oo$\"$ooo   o$    \"$    ooo\"$oo  $$$\"o\n"
	  "   o o o o    oo\"  o\"      \"o    $$o$\"     o o$\"\"  o$      \"$  "
	  "\"oo   o o o o\n"
	  "   \"$o   \"\"$$$\"   $$         $      \"   o   \"\"    o\"         $"
	  "   \"o$$\"    o$$\n"
	  "     \"\"o       o  $          $\"       $$$$$       o          $  ooo"
	  "     o\"\"\n"
	  "        \"o   $$$$o $o       o$        $$$$$\"       $o        \" $$$$"
	  "   o\"\n"
	  "         \"\"o $$$$o  oo o  o$\"         $$$$$\"        \"o o o o\"  "
	  "\"$$$  $\n"
	  "           \"\" \"$\"     \"\"\"\"\"            \"\"$\"            \""
	  "\"\"      \"\"\" \"\n"
	  "            \"oooooooooooooooooooooooooooooooooooooooooooooooooooooo$\n"
	  "             \"$$$$\"$$$$\" $$$$$$$\"$$$$$$ \" \"$$$$$\"$$$$$$\"  $$$\""
	  "\"$$$$\n"
	  "              $$$oo$$$$   $$$$$$o$$$$$$o\" $$$$$$$$$$$$$$ o$$$$o$$$\"\n"
	  "              $\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\""
	  "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"$\n"
	  "              $\"                                                 \"$\n"
	  "              $\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\""
	  "$\"$\"$\"$\"$\"$\"$\"$\n"
	  "                                   You win!\n\n";
		printf("%s\n", victory);
	}
	heap_delete(&h);
}

void init_terminal(void){
	initscr(); //creates stdscr
	raw(); //allows input with no exit way
	noecho(); //???
	curs_set(0); //???
	keypad(stdscr, TRUE); //allows keypad commands
	start_color();
	init_pair(COLOR_BLACK, COLOR_WHITE, COLOR_BLACK);
	init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
	init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
	init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
	init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
	init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
	init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

void parseMonstFile(const char* monst_file, Character_t characters[MAX_MONSTERS], int *num_Mon){
	ifstream f(monst_file);
	string testChar;
	int i = 1;
	int j = 0;
	int k, tempDice, tempSides;
	int maxDesc = 78;
	string s, s1, s2;
	bool readingMonst = false;
	bool readingInfo = false;
	bool error = false;
	//checks each step is complete
	bool compSymb = false;
	bool compColor = false;
	bool compName = false;
	bool compDesc = false;
	bool compAbil = false;
	bool compHP = false;
	bool compAD = false;
	bool compRar = false;
	bool compSpeed = false;
	getline(f, s);
	s.erase(s.length()-1);
	if(!s.compare("RLG327 MONSTER DESCRIPTION 1")){ //check the file has the signature to begin reading the data
		while(f >> s && !error){ //while theres output keep parsing
			if(!s.compare("BEGIN")){
				f >> s;
				if(!s.compare("MONSTER")){
					f.get();
					readingMonst = true;
				}
			}
			while(readingMonst){
				f >> s;
				if(!s.compare("NAME")){
					s = "";
					f.get();
					getline(f, s);
					s.erase(s.length()-1);
					for(int k = 0; k < s.length(); k++){
						characters[i].name[k] = s[k];
					}
					compName = true;
					// cout << characters[i].name << endl;
				} else if(!s.compare("SYMB")){
					s = "";
					f.get();
					getline(f, s);
					s.erase(s.length()-1);
					characters[i].c = s[0];
					compSymb = true;
					// cout << characters[i].c << endl;	
				} else if(!s.compare("COLOR")){
					s = "";
					s1 = "";
					s2 = "";
					readingInfo = true;
					f.get();
					while(readingInfo){
						s2 = (char) f.get();
						if(!s2.compare(" ") || !s2.compare("\n")){
							if(!s2.compare("\n")){ //if its a new line then we know were done reading data about the abilities
								s.erase(s.length()-1);
								s1.erase(s1.length()-1);
								readingInfo = false;
							}
							if(!s2.compare(" ")){ 
								s += s2;
							}
							if(!compColor){
								if(!s1.compare("BLACK")){
									compColor = true;
									characters[i].color.BLACK = true;
								} else if(!s1.compare("RED")){
									compColor = true;
									characters[i].color.RED = true;
								} else if(!s1.compare("GREEN")){
									compColor = true;
									characters[i].color.GREEN = true;
								} else if(!s1.compare("YELLOW")){
									compColor = true;
									characters[i].color.YELLOW = true;
								} else if(!s1.compare("BLUE")){
									compColor = true;
									characters[i].color.MAGENTA = true;
								} else if(!s1.compare("MAGENTA")){
									compColor = true;
									characters[i].color.MAGENTA = true;
								} else if(!s1.compare("CYAN")){
									compColor = true;
									characters[i].color.CYAN = true;
								} else if(!s1.compare("WHITE")){
									compColor = true;
									characters[i].color.WHITE = true;
								}
							}
							s1 = ""; //clears the s1 string for the next abilities reading
						} else {
							s1 += s2;
							s += s2;
						}
					}
					// cout << characters[i].color.BLACK << endl;	
					// cout << characters[i].color.RED << endl;
					// cout << characters[i].color.GREEN << endl;
					// cout << characters[i].color.YELLOW << endl;
					// cout << characters[i].color.BLUE << endl;
					// cout << characters[i].color.MAGENTA << endl;
					// cout << characters[i].color.CYAN << endl;
					// cout << characters[i].color.WHITE << endl;					
				} else if(!s.compare("DESC")){
					getline(f, s); //clears whitespace and new line after DESC
					s = ""; //clear s to add the complete description
					readingInfo = true;
					while(j <= maxDesc && readingInfo){
						s1 = (char) f.get();
						if(!s1.compare("\n")){
							s.erase(s.length()-1);
							j = 0;
							testChar = (char) f.get();
							if(!testChar.compare(".")){
								readingInfo = false;
							} else {
								s1 += testChar;
							}
						}
						if(readingInfo){
							s += s1;
							j++;
						}
						if(j > maxDesc) {
							readingInfo = false;
							error = true;
						}
					}
					compDesc = true;
					for(int k = 0; k < s.length(); k++){
						characters[i].description[k] = s[k];
					}	
					// cout << characters[i].description << endl;
				} else if(!s.compare("SPEED")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							characters[i].s = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){ //grab number of dice
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){ //Grab number of sides
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compSpeed = true;
						}
					}
					for(k = 0; k < tempDice; k++){
						characters[i].s += 	1 + rand() % tempSides;
					}
					// cout << characters[i].s << endl;
				} else if(!s.compare("DAM")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							characters[i].attackDamage.base = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							characters[i].attackDamage.dice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							characters[i].attackDamage.sides = stoi(s1);
							readingInfo = false;
							compAD = true;
						}
						
					}
					// cout << characters[i].attackDamage.base << "+" << characters[i].attackDamage.dice << "d" << characters[i].attackDamage.sides << endl;
				} else if(!s.compare("HP")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							characters[i].health = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compHP = true;
						}
						
					}
					for(k = 0; k < tempDice; k++){
						characters[i].health += 1 + rand() % tempSides;
					}
					// cout << characters[i].health << endl;
				} else if(!s.compare("ABIL")){
					s = "";
					s1 = "";
					s2 = "";
					readingInfo = true;
					f.get();
					while(readingInfo){
						s2 = (char) f.get();
						if(!s2.compare(" ") || !s2.compare("\n")){
							if(!s2.compare("\n")){ //if its a new line then we know were done reading data about the abilities
								s.erase(s.length()-1);
								s1.erase(s1.length()-1);
								readingInfo = false;
							}
							if(!s2.compare(" ")){ 
								s += s2;
							}
							if(!s1.compare("SMART")){
								compAbil = true;
								characters[i].i = TRUE;
							} else if(!s1.compare("TELE")){
								compAbil = true;
								characters[i].t = TRUE;
							} else if(!s1.compare("TUNNEL")){
								compAbil = true;
								characters[i].tu = TRUE;
							} else if(!s1.compare("ERRATIC")){
								compAbil = true;
								characters[i].e = TRUE;
							} else if(!s1.compare("PASS")){
								compAbil = true;
								characters[i].p = TRUE;
							} else if(!s1.compare("PICKUP")){
								compAbil = true;
								characters[i].pu = TRUE;
							} else if(!s1.compare("DESTROY")){
								compAbil = true;
								characters[i].d = TRUE;
							} else if(!s1.compare("UNIQ")){
								compAbil = true;
								characters[i].u = TRUE;
							} else if(!s1.compare("BOSS")){
								compAbil = true;
								characters[i].b = TRUE;
							}
							s1 = ""; //clears the s1 string for the next abilities reading
						} else {
							s1 += s2;
							s += s2;
						}
					}
					// cout << s << endl;
				} else if(!s.compare("RRTY")){
					s = "";
					f.get();
					getline(f, s);
					s.erase(s.length()-1);
					characters[i].rarity = stoi(s);
					compRar = true;	
					// cout << characters[i].rarity << endl;	
				} else if(!s.compare("END")){
					*num_Mon += 1;
					characters[i].a = true;
					i++;
					// cout << "\n";
					readingMonst = false;
				}
			}
			if(!compName || !compDesc || !compColor || !compSpeed || !compAbil || !compHP || !compAD || !compSymb || !compRar || error){
				cout << "Error parsing monster info. Info has been wiped. Please reformat and rerun" << endl;
				i--;
				characters[i].a = FALSE; //character is no longer alive because of bad data. I go back with my i to overwrite it or leave it and it isn't pasted in the dungeon
			}
			compSymb = compColor = compName = compDesc = compAbil = compHP = compAD = compRar = compSpeed = error = false;
		}
	}
}

void parseObjFile(const char* equip_file, Equipment equipment[MAX_EQUIPMENT], int *num_Equip){
	ifstream f(equip_file);
	string testChar;
	int i = 1;
	int j = 0;
	int maxDesc = 78;
	int tempSides, tempDice, k;
	string s, s1, s2;
	bool readingEquip = false;
	bool readingInfo = false;
	bool error = false;
	//checks each step is complete
	bool compName = false;
	bool compType = false;
	bool compColor = false;
	bool compWeight = false;
	bool compHit = false;
	bool compDam = false;
	bool compAttr = false;
	bool compVal = false;
	bool compDodge = false;
	bool compDef = false;
	bool compSpeed = false;
	bool compArt = false;
	bool compRar = false;
	bool compDesc = false;
	getline(f, s);
	s.erase(s.length()-1);
	if(!s.compare("RLG327 OBJECT DESCRIPTION 1")){ //check the file has the signature to begin reading the data
		while(f >> s && !error){ //while theres output keep parsing
			if(!s.compare("BEGIN")){
				f >> s;
				if(!s.compare("OBJECT")){
					f.get();
					readingEquip = true;
				}
			}
			while(readingEquip){
				f >> s;
				if(!s.compare("NAME")){
					s = "";
					f.get();
					getline(f, s);
					s.erase(s.length()-1);
					for(int k = 0; k < s.length(); k++){
						equipment[i].name[k] = s[k];
					}
					compName = true;
					// cout << equipment[i].name << endl;
				} else if(!s.compare("TYPE")){
					s = "";
					f.get();
					getline(f, s);
					s.erase(s.length()-1);
					if(!s.compare("WEAPON")){
						equipment[i].equipment.weapon = true;
						equipment[i].c = '|';
					} else if(!s.compare("OFFHAND")){
						equipment[i].equipment.offhand = true;
						equipment[i].c = ')';
					} else if(!s.compare("RANGED")){
						equipment[i].equipment.ranged = true;
						equipment[i].c = '}';
					} else if(!s.compare("ARMOR")){
						equipment[i].equipment.armor = true;
						equipment[i].c = '[';
					} else if(!s.compare("HELMET")){
						equipment[i].equipment.helmet = true;
						equipment[i].c = ']';
					} else if(!s.compare("CLOAK")){
						equipment[i].equipment.cloak = true;
						equipment[i].c = '(';
					} else if(!s.compare("GLOVES")){
						equipment[i].equipment.gloves = true;
						equipment[i].c = '{';
					} else if(!s.compare("BOOTS")){
						equipment[i].equipment.boots = true;
						equipment[i].c = '\\';
					} else if(!s.compare("RING")){
						equipment[i].equipment.ring = true;
						equipment[i].c = '=';
					} else if(!s.compare("AMULET")){
						equipment[i].equipment.amulet = true;
						equipment[i].c = '"';
					} else if(!s.compare("LIGHT")){
						equipment[i].equipment.light = true;
						equipment[i].c = '_';
					} else if(!s.compare("SCROLL")){
						equipment[i].equipment.scroll = true;
						equipment[i].c = '~';
					} else if(!s.compare("BOOK")){
						equipment[i].equipment.book = true;
						equipment[i].c = '?';
					} else if(!s.compare("FLASK")){
						equipment[i].equipment.flask = true;
						equipment[i].c = '!';
					} else if(!s.compare("GOLD")){
						equipment[i].equipment.gold = true;
						equipment[i].c = '$';
					} else if(!s.compare("AMMUNITION")){
						equipment[i].equipment.ammunition = true;
						equipment[i].c = '/';
					} else if(!s.compare("FOOD")){
						equipment[i].equipment.food = true;
						equipment[i].c = ',';
					} else if(!s.compare("WAND")){
						equipment[i].equipment.wand = true;
						equipment[i].c = '-';
					} else if(!s.compare("CONTAINER")){
						equipment[i].equipment.container = true;
						equipment[i].c = '%';
					}
					compType = true;
					// cout << s << endl;
				} else if(!s.compare("DESC")){
					getline(f, s); //clears whitespace and new line after DESC
					s = ""; //clear s to add the complete description
					readingInfo = true;
					while(j <= maxDesc && readingInfo){
						s1 = (char) f.get();
						if(!s1.compare("\n")){
							s.erase(s.length()-1);
							j = 0;
							testChar = (char) f.get();
							if(!testChar.compare(".")){
								readingInfo = false;
							} else {
								s1 += testChar;
							}
						}
						if(readingInfo){
							s += s1;
							j++;
						}
						if(j > maxDesc) {
							readingInfo = false;
							error = true;
						}
					}
					compDesc = true;
					for(int k = 0; k < s.length(); k++){
						equipment[i].description[k] = s[k];
					}	
					// cout << equipment[i].description << endl;
				} else if(!s.compare("COLOR")){
					s = "";
					s1 = "";
					s2 = "";
					readingInfo = true;
					f.get();
					while(readingInfo){
						s2 = (char) f.get();
						if(!s2.compare(" ") || !s2.compare("\n")){
							if(!s2.compare("\n")){ //if its a new line then we know were done reading data about the abilities
								s.erase(s.length()-1);
								s1.erase(s1.length()-1);
								readingInfo = false;
							}
							if(!s2.compare(" ")){ 
								s += s2;
							}
							if(!s1.compare("BLACK")){
								compColor = true;
								equipment[i].color.BLACK = true;
							} else if(!s1.compare("RED")){
								compColor = true;
								equipment[i].color.RED = true;
							} else if(!s1.compare("GREEN")){
								compColor = true;
								equipment[i].color.GREEN = true;
							} else if(!s1.compare("YELLOW")){
								compColor = true;
								equipment[i].color.YELLOW = true;
							} else if(!s1.compare("BLUE")){
								compColor = true;
								equipment[i].color.MAGENTA = true;
							} else if(!s1.compare("MAGENTA")){
								compColor = true;
								equipment[i].color.MAGENTA = true;
							} else if(!s1.compare("CYAN")){
								compColor = true;
								equipment[i].color.CYAN = true;
							} else if(!s1.compare("WHITE")){
								compColor = true;
								equipment[i].color.WHITE = true;
							}
							s1 = ""; //clears the s1 string for the next abilities reading
						} else {
							s1 += s2;
							s += s2;
						}
					}
					// cout << s << endl;			
				} else if(!s.compare("WEIGHT")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							equipment[i].weight = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compWeight = true;
						}
					}
					for(k = 0; k < tempDice; k++){
						equipment[i].weight += 1 + rand() % tempSides;
					}
					// cout << equipment[i].weight << endl;
				} else if(!s.compare("HIT")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							equipment[i].HB = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compHit = true;
						}
						
					}
					for(k = 0; k < tempDice; k++){
						equipment[i].HB += 1 + rand() % tempSides;
					}
					// cout << equipment[i].HB << endl;
				} else if(!s.compare("DAM")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							equipment[i].DB.base = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							equipment[i].DB.dice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							equipment[i].DB.sides = stoi(s1);
							readingInfo = false;
							compDam = true;
						}
					}
					// cout << equipment[i].DB.base << "+" << equipment[i].DB.dice << "d" << equipment[i].DB.sides << endl;
				} else if(!s.compare("ATTR")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							equipment[i].SA = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compAttr = true;
						}
						
					}
					for(k = 0; k < tempDice; k++){
						equipment[i].SA += 1 + rand() % tempSides;
					}
					// cout << equipment[i].SA << endl;
				} else if(!s.compare("VAL")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							equipment[i].value = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compVal = true;
						}
						
					}
					for(k = 0; k < tempDice; k++){
						equipment[i].value += 1 + rand() % tempSides;
					}
					// cout << equipment[i].value << endl;
				} else if(!s.compare("DODGE")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							equipment[i].DDB = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compDodge = true;
						}
						
					}
					for(k = 0; k < tempDice; k++){
						equipment[i].DDB += 1 + rand() % tempSides;
					}
					// cout << equipment[i].DDB << endl;
				} else if(!s.compare("DEF")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							equipment[i].DFB = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compDef = true;
						}
						
					}
					for(k = 0; k < tempDice; k++){
						equipment[i].DFB += 1 + rand() % tempSides;
					}
					// cout << equipment[i].DFB << endl;
				} else if(!s.compare("SPEED")){
					s = "";
					s1 = "";
					s2 = "";
					f.get();
					readingInfo = true;
					while(readingInfo){
						s2 = (char) f.get();
						s1 += s2;
						s += s2;
						if(!s2.compare("+")){
							s1.erase(s1.length()-1);
							equipment[i].SB = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							tempDice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							tempSides = stoi(s1);
							readingInfo = false;
							compSpeed = true;
						}
						
					}
					for(k = 0; k < tempDice; k++){
						equipment[i].SB += 1 + rand() % tempSides;
					}
					// cout << equipment[i].SB << endl;
				} else if(!s.compare("RRTY")){
					s = "";
					f.get();
					getline(f, s);
					s.erase(s.length()-1);
					equipment[i].rarity = stoi(s);
					compRar = true;	
					// cout << equipment[i].rarity << endl;	
				} else if(!s.compare("ART")){
					s = "";
					f.get();
					getline(f, s);
					s.erase(s.length()-1);
					if(!s.compare("FALSE")){
						equipment[i].status = false;
					} else {
						equipment[i].status = true;
					}
					compArt = true;	
					// cout << s << endl;	
				} else if(!s.compare("END")){
					*num_Equip += 1;
					equipment[i].a = true;
					i++;
					// cout << "\n";
					readingEquip = false;
				}
			}
			if(!compName || !compType || !compColor || !compWeight || !compHit || !compDam || !compAttr || !compVal || !compDodge ||
				!compDef || !compSpeed || !compArt || !compRar || !compDesc || error){
				cout << "Error parsing object info. Info has been wiped. Please reformat and rerun" << endl;
				i--;
			}
			compName = compType = compColor = compWeight = compHit = compDam = compAttr = compVal = compDodge = compDef = compSpeed = compArt = compRar = compDesc = error = false;
		}
	}
}

int main(int argc, char *argv[]){
	int number_of_rooms = 0; //holds values for number of rooms generated in the dungeon
	int number_of_upstairs = 0; //holds values for number of upcases generated in the dungeon
	int number_of_downstairs = 0; //holds values for number of downcases generated in the dungeon
	
	char dungeon[WINDOW_Y][WINDOW_X]; //array containing everything about the dungeon for the user
	char fogDungeon[GAME_HEIGHT][GAME_WIDTH]; //array containing everything about the dungeon for the user
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
	
	// Character_t characters[MAX_MONSTERS];
	Character_t *characters = (Character_t *) malloc(MAX_MONSTERS * sizeof(Character_t));
	
	Equipment *equipment = (Equipment *) malloc(MAX_EQUIPMENT * sizeof(Equipment));
	/*
	Array containing every monster in our dungeon
	*/
	
	//Takes the time to randomize our room generation
	srand(time(NULL));
	
	/*
	char *home = getenv("HOME");
	char *directory;
	directory = (char *) malloc(strlen(home) + strlen("/.rlg327/dungeon") + 1);
	strcpy(directory, home);
	strcat(directory, "/.rlg327/dungeon");
	*/	

	/*
	home = getenv("HOME");
	char *monst_file;
	monst_file = (char *) malloc(strlen(home) + strlen("/.rlg327/monster_desc") + 1);
	strcpy(monst_file, home);
	strcat(monst_file, "/.rlg327/monster_desc");	
	*/
	
	/*
	home = getenv("HOME");
	char *obj_file;
	obj_file = (char *) malloc(strlen(home) + strlen("/.rlg327/object_desc") + 1);
	strcpy(obj_file, home);
	strcat(obj_file, "/.rlg327/object_desc");	
	*/
			
	char *directory = (char *) "C:/Users/morro/.rlg327/dungeon.txt";
	char *monst_file = (char *) "C:/Users/morro/.rlg327/monster_desc.txt";
	char *obj_file = (char *) "C:/Users/morro/.rlg327/object_desc.txt";
	// char *directory = "C:/Users/morro/.rlg327/01.rlg327";
	// char *directory = "/home/student/.rlg327/01.rlg327";
	
	int i = 0;
	int j = 0;
	int num_Mon = 1;
	int num_Equip = 0;
	
	// //initiates ncurses capabilities
	init_terminal();
	
	user_action action;
	if(argv[1] != NULL){
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
		}

		switch(action){
			case load:
				loadDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
				generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms, &number_of_rooms, 0, &num_Mon, characters, fogDungeon);
				printDungeon(dungeon, hardness);
				// free(characters);
				// free(equipment);
				// free(directory);
				// free(monst_file);
				// free(obj_file);	
				break;
			case save:
				generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms, &number_of_rooms, 0, &num_Mon, characters, fogDungeon);
				saveDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
				printDungeon(dungeon, hardness);
				// free(characters);
				// free(equipment);
				// free(directory);
				// free(monst_file);
				// free(obj_file);
				break;
			case load_save:
				loadDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
				generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms, &number_of_rooms, 0, &num_Mon, characters, fogDungeon);
				saveDungeon(hardness, directory, PC, &number_of_rooms, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms);
				printDungeon(dungeon, hardness);
				// free(characters);
				// free(equipment);
				// free(directory);
				// free(monst_file);
				// free(obj_file);
				break;
			case num_mon:
				generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms, &number_of_rooms, 0, &num_Mon, characters, fogDungeon);
				simulateGame(characters, &num_Mon, dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms, &number_of_rooms, fogDungeon, &num_Equip, equipment);
				// free(characters);
				// free(equipment);
				// free(directory);
				// free(monst_file);
				// free(obj_file);
				break;
		}
	} else {
		// cout << "Please enter: <--save, --load, --load--save> after './game' to perform an action";
		parseMonstFile(monst_file, characters, &num_Mon);
		parseObjFile(obj_file, equipment, &num_Equip);
		generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms, &number_of_rooms, 0, &num_Mon, characters, fogDungeon);
		simulateGame(characters, &num_Mon, dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, &number_of_upstairs, &number_of_downstairs, rooms, &number_of_rooms, fogDungeon, &num_Equip, equipment);
		// free(characters);
		// free(equipment);
		// free(directory);
		// free(monst_file);
		// free(obj_file);
	}
	return 0;
}