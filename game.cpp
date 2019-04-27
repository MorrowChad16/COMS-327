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
#define DEF_HEALTH 200
#define NUM_STORAGE 10


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
	bool GREEN;
	bool YELLOW;
	bool BLUE;
	bool RED;
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
		bool trap;
};

class Equipment{
	public:
		char name[78]; //holds the name of the equipment
		char description[1024]; //holds the description of the monster
		Items equipment;
		Color color;
		int HitBonus; //hit bonus
		Dice DamageBonus; //damage bonus
		int DodgeBonus; //dodge bonus
		int DefenseBonus; //defense bonus
		int weight;
		int SpeedBonus; //speed bonus
		int SpecialAtr; //special attribute
		int value;
		bool status;
		int rarity;
		char lv; //holds the dungeon character
		bool a; //tells whether the equipment is alive or not/can be placed or not
		int pos[2]; //position of equipment
		char c; //holds the equipment character
		bool isHeld;
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
		int storage[NUM_STORAGE]; //stores items index. Can include anything, but should be used for equipment
		int equiped[NUM_STORAGE + 2]; //1 slot for each type of item for the character to use
		int weapons[4]; //sword, offhand, ranged and wand
		int spells[5]; //temporary boosts to 
		int armor[5]; //armor, helmet, cloak, gloves, shoes
		int ammunition[3]; //arrows, traps and throwable lights
		int dodge; //dodge ability of PC
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
int deleteMonster(char dungeon[WINDOW_Y][WINDOW_X], Character_t characters[MAX_MONSTERS], int oldXpos, int *moveX, int oldYpos, int *moveY, int *num_Mon, bool isPC, Equipment equipment[MAX_EQUIPMENT]){
	int i, j, index, tempX, tempY, randDodge, charLoc;
	int newYpos = oldYpos + *moveY;
	int newXpos = oldXpos + *moveX;
	bool isPlaced = false;
	char tempChar;
	int loopThrough = 0;
	int randLoc;
	int randLocArr[8][2]; //X, Y
	randLocArr[0][0] = 1;
	randLocArr[0][1] = 0;
	
	randLocArr[1][0] = 1;
	randLocArr[1][1] = 1;
	
	randLocArr[2][0] = 0;
	randLocArr[2][1] = 1;
	
	randLocArr[3][0] = -1;
	randLocArr[3][1] = 1;
	
	randLocArr[4][0] = -1;
	randLocArr[4][1] = 0;
	
	randLocArr[5][0] = -1;
	randLocArr[5][1] = -1;
	
	randLocArr[6][0] = 0;
	randLocArr[6][1] = -1;
	
	randLocArr[7][0] = 1;
	randLocArr[7][1] = -1;
	
	if(isPC){ //if the char is the PC then check if the new spot is a monster
		for(i = 0; i < MAX_MONSTERS; i++){ //loops through all our characters
			if(newYpos == characters[i].pos[Y_LOC] && newXpos == characters[i].pos[X_LOC] && characters[i].a){ //checks if the new position is occupied by a monster
				characters[i].health -= characters[0].attackDamage.base;
				for(int m = 0; m < characters[0].attackDamage.dice; m++){
					randDodge = 1 + rand() % 1000;
					if(randDodge > characters[i].dodge){ //If randDodge is above the characters dodge ability then the attack hits
						characters[i].health -= 1 + rand() % characters[0].attackDamage.sides;
					}
				} 
				if(characters[i].health < 0){
					dungeon[newYpos][newXpos] = characters[i].lv; //Replace the pos with the dungeon char
					characters[i].a = false;
					if(characters[i].b){
						*num_Mon = 0;
					}
					characters[i].pos[X_LOC] = 0;
					characters[i].pos[Y_LOC] = 0;
					return 1;
				}
				*moveX = 0;
				*moveY = 0;
			}
		}
		return 0;
	} else { //if the char is an NPC then check if the spot is a monster
	
		for(int b = 0; b < MAX_MONSTERS; b++){ //find monster we're attacking with
			if(oldXpos == characters[b].pos[X_LOC] && oldYpos == characters[b].pos[Y_LOC]){
				index = b;
			}
		}
		
		for(i = 0; i < MAX_MONSTERS; i++){ //loops through all our characters
			if(newYpos == characters[i].pos[Y_LOC] && newXpos == characters[i].pos[X_LOC] && characters[i].a){ //checks if the new position is occupied by a monster
				
				if(i == 0){ //reduce PC health
					characters[0].health -= characters[index].attackDamage.base;
					for(int b = 0; b < characters[index].attackDamage.dice; b++){
						randDodge = 1 + rand() % 1000;
						if(randDodge > characters[0].dodge){ //if randDodge is above the PC's dodgeBonus then the attack hits
							characters[0].health -= 1 + rand() % characters[index].attackDamage.sides;
						}
					}
					*moveX = 0;
					*moveY = 0;
				}
				
				// if new location is occupied in that pos then move to a surrounding opening cell
				while(!isPlaced && loopThrough < 16){
					randLoc = rand() % 8;
					if(dungeon[newYpos + randLocArr[randLoc][1]][newXpos + randLocArr[randLoc][0]] == '.' || dungeon[newYpos + randLocArr[randLoc][1]][newXpos + randLocArr[randLoc][0]] == ' ' || dungeon[newYpos + randLocArr[randLoc][1]][newXpos + randLocArr[randLoc][0]] == '#'){ //Right
						*moveX += randLocArr[randLoc][0];
						*moveY += randLocArr[randLoc][1];
						isPlaced = true;
					}
					loopThrough++;
				}
				
				if(!isPlaced) { //no new positions swap locations
					*moveY = 0;
					*moveX = 0;
					// store the cell were about to override
					tempX = characters[index].pos[X_LOC];
					tempY = characters[index].pos[Y_LOC];
					tempChar = characters[index].lv;
					// first swap
					characters[index].pos[X_LOC] = characters[i].pos[X_LOC];
					characters[index].pos[Y_LOC] = characters[i].pos[Y_LOC];
					characters[index].pos[Y_LOC] = characters[i].lv;
					// second swap
					characters[i].pos[X_LOC] = tempX;
					characters[i].pos[Y_LOC] = tempY;
					characters[i].lv = tempChar;
				}
				
				if(characters[0].health < 0){
					characters[0].a = false;
					return 1;
				}
			}
		}
		
		for(int x = 0; x < MAX_MONSTERS; x++){
			if(equipment[x].pos[X_LOC] == newXpos && equipment[x].pos[Y_LOC] == newYpos && equipment[x].a){
				charLoc = x;
			}
		}
			
		if(dungeon[newYpos][newXpos] == 'T' && equipment[charLoc].color.RED){
			characters[index].health -= equipment[charLoc].DamageBonus.base;
			for(int b = 0; b < equipment[charLoc].DamageBonus.dice; b++){
				characters[index].health -= 1 + rand() % equipment[charLoc].DamageBonus.sides;
			}
			dungeon[newYpos][newXpos] = equipment[charLoc].lv;
			equipment[charLoc].pos[X_LOC] = 0;
			equipment[charLoc].pos[Y_LOC] = 0;
			equipment[charLoc].a = false;
		}
		
		if(characters[index].health < 0){
			dungeon[oldYpos][oldXpos] = characters[index].lv;
			characters[index].a = false;
			characters[index].pos[X_LOC] = 0;
			characters[index].pos[Y_LOC] = 0;
			return 1;
		}
		
		return 0;
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
	int row, col, i, cmnd, randX, randY, j, maxLoop, input, shootingMessage, charIndex, monIndex, randDodge, tempCurs;
	int wpKey, arKey, amKey, spKey, stKey;
	int message = -1;
	int key, eKey, iKey, temp;
	int isError = 0;
	bool isStored = false;
	int cursLoc = 0;
	bool takenOff = false;
	int moveX, moveY, tempX, tempY;
	char tempChar;
	bool isDropped = false;
	bool isSwapped = false;
	int randLoc;
	int randLocArr[8][2]; //X, Y
	bool onMonster = false;
	randLocArr[0][0] = 1;
	randLocArr[0][1] = 0;
	
	randLocArr[1][0] = 1;
	randLocArr[1][1] = 1;
	
	randLocArr[2][0] = 0;
	randLocArr[2][1] = 1;
	
	randLocArr[3][0] = -1;
	randLocArr[3][1] = 1;
	
	randLocArr[4][0] = -1;
	randLocArr[4][1] = 0;
	
	randLocArr[5][0] = -1;
	randLocArr[5][1] = -1;
	
	randLocArr[6][0] = 0;
	randLocArr[6][1] = -1;
	
	randLocArr[7][0] = 1;
	randLocArr[7][1] = -1;
	
	if(message == 100){
		CMD:;
		clear();
		mvprintw(0, 0, "Commands:\nUP_KEY: Move PC Up\nRIGHT KEY: Move PC Right\nDOWN_KEY: Move PC Down\nLEFT_KEY: Move PC Left\n>: Go Down Stairs\n<: Go Up Stairs\n.: Rest PC\nm: Display Monsters\ni: Inventory\ne: Equiped\nQ: Quit\n\nPress any key to exit");
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
	for(int row = 0; row < GAME_HEIGHT; row++){
		for(int col = 0; col < GAME_WIDTH; col++){
			if(dungeon[row][col] == '+'){ //if its a fire source 
				for(int k = row - 1; k < row + 2; k++){
					for(int m = col - 1; m < col + 2; m++){
						fogDungeon[k][m] = dungeon[k][m];
					}
				}
			}
			if(dungeon[row][col] == '@'){ //if its the pc add the 5x5 range around it
				for(int h = row - 2; h < row + 3; h++){
					for(int b = col - 2; b < col + 3; b++){
						fogDungeon[h][b] = dungeon[h][b];
					}
				}
			}
		}
	}
	
	//check if the new location is over an item
	if(characters[0].lv != '.' || characters[0].lv != '#'){ //location isn't a room or corridor so it must be an equipment item
		for(int k = 0; k < NUM_STORAGE; k++){ //loop through PC storage to look for open slot to place the new item
			if(characters[0].storage[k] == -1 && !isStored){ //character has an empty slot for a new item
				for(int l = 0; l < *num_Equip; l++){ //loop through equipment to find the right one
					if(characters[0].pos[Y_LOC] == equipment[l].pos[Y_LOC] && characters[0].pos[X_LOC] == equipment[l].pos[X_LOC] && !isStored){ //look for equipment with same position as PC
						if(equipment[l].equipment.trap && equipment[l].color.RED){
							characters[0].health -= equipment[l].DamageBonus.base;
							for(int b = 0; b < equipment[l].DamageBonus.dice; b++){
								characters[0].health -= 1 + rand() % equipment[l].DamageBonus.sides;
							}
							characters[0].lv = equipment[l].lv;
							equipment[l].pos[X_LOC] = 0;
							equipment[l].pos[Y_LOC] = 0;
							equipment[l].a = false;
						} else {
							characters[0].storage[k] = l;
							characters[0].lv = equipment[l].lv; //put equipment lv into the characters lv
							isStored = true;
							equipment[l].isHeld = true;
							equipment[l].pos[X_LOC] = 0;
							equipment[l].pos[Y_LOC] = 0;
						}
					}
				}
			}
		}
		isStored = false;
	}
	
	//Pastes the fog dungeon
	for(row = 0; row < GAME_HEIGHT; row++){
		for(col = 0; col < GAME_WIDTH; col++){
			if(fogDungeon[row][col] == '.' || fogDungeon[row][col] == '#' || fogDungeon[row][col] == ' ' || fogDungeon[row][col] == '|' 
				|| fogDungeon[row][col] == '-' || fogDungeon[row][col] == '@' || fogDungeon[row][col] == '<' || fogDungeon[row][col] == '>' 
				|| fogDungeon[row][col] == '+' || fogDungeon[row][col] == '`'){ //always white
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

	printw("\nPC HP: %d	PC Speed: %d	PC DAMAGE: %d+%dd%d", characters[0].health, characters[0].s, characters[0].attackDamage.base, characters[0].attackDamage.dice, characters[0].attackDamage.sides);
	
	ER:;
	if(message == 0){
		mvprintw(MESSAGES, 0, "YOU ARE RUNNING INTO A WALL! GO IN A NEW DIRECTION!\n");
	} else if(message == 1){
		mvprintw(MESSAGES, 0, "YOU ARE NOT ON A DOWN STAIR CASE!\n");
	} else if(message == 2){
		mvprintw(MESSAGES, 0, "YOU ARE NOT ON AN UP STAIR CASE!\n");
	} else if(message == 3){
		mvprintw(MESSAGES, 0, "YOU ARE NOT ON DEBRIS!\n");
	} else if(message == 4){
		mvprintw(MESSAGES, 0, "YOU DO NOT HAVE A RANGED WEAPON!\n");
	}
	
	key = getch();
	switch(key) {
		case KEY_UP: //UP
			if(!hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC]]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				moveY = -1;
				moveX = 0;
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC], &moveX, characters[0].pos[Y_LOC], &moveY, num_Mon, true, equipment);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[Y_LOC] += moveY; //Update Y
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				message = 0;
				goto ER;
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case KEY_RIGHT: //RIGHT
			if(!hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] + 1]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				moveY = 0;
				moveX = 1;
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC], &moveX, characters[0].pos[Y_LOC], &moveY, num_Mon, true, equipment);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[X_LOC] += moveX; //Update X
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon

			} else {
				message = 0;
				goto ER;
				
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case KEY_DOWN: //DOWN
			if(!hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC]]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				moveY = 1;
				moveX = 0;
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC], &moveX, characters[0].pos[Y_LOC], &moveY, num_Mon, true, equipment);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[Y_LOC] += moveY; //Update Y
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				message = 0;
				goto ER;
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case KEY_LEFT: //LEFT
			if(!hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] - 1]){ //checks new space is empty space
				//Checks if space is occupied by a monster
				moveY = 0;
				moveX = -1;
				deleteMonster(dungeon, characters, characters[0].pos[X_LOC], &moveX, characters[0].pos[Y_LOC], &moveY, num_Mon, true, equipment);
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv; //update dungeon with covered character by PC
				characters[0].pos[X_LOC] += moveX; //Update X
				characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //Update LV
				dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update dungeon
			} else {
				message = 0;
				goto ER;
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case '.': //REST
			R:;
			//PC doesn't do anything
			break;
		//-------------------------------------------------------------------------------------------------------------------------------	
		case '>': //attempts to go downstairs
			if(characters[0].lv == '>'){
				generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, number_of_upstairs, number_of_downstairs, rooms, number_of_rooms, 1, num_Mon, characters, fogDungeon);
				*gameStatus = 2;
			} else {
				message = 1;
				goto ER;
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case '<': //attempts to go upstairs
			if(characters[0].lv == '<'){
				generateNewFloor(dungeon, hardness, roomMap, wholeMap, PC, upwardCases, downwardCases, number_of_upstairs, number_of_downstairs, rooms, number_of_rooms, 1, num_Mon, characters, fogDungeon);
				*gameStatus = 2;
			} else {
				message = 2;
				goto ER;
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case 'l': //lights debris on fire
			if(characters[0].lv == '`'){
				characters[0].lv = '+';
			} else {
				message = 3;
				goto ER;
			}
		break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case 's': //shoot if you have a ranged weapon
			tempX = characters[0].pos[X_LOC];
			tempY = characters[0].pos[Y_LOC]; 
			shootingMessage = 0;
			if(characters[0].equiped[2] != -1){
				WINDOW *w;
				w = newwin(24, 80, 0, 0);
				
				RESET:;
				wrefresh(w);
				clear();
				for(int row = characters[0].pos[Y_LOC] - 3; row < characters[0].pos[Y_LOC] + 4; row++){
					for(int col = characters[0].pos[X_LOC] - 3; col < characters[0].pos[X_LOC] + 4; col++){
						if(row == tempY && col == tempX){ //always white
							mvaddch(row, col, '*');
						} else if(dungeon[row][col] == '.' || dungeon[row][col] == '#' || dungeon[row][col] == ' ' || dungeon[row][col] == '|' 
							|| dungeon[row][col] == '-' || dungeon[row][col] == '@' || dungeon[row][col] == '<' || dungeon[row][col] == '>' 
							|| dungeon[row][col] == '+' || dungeon[row][col] == '`'){
							mvaddch(row, col, dungeon[row][col]);
						}else {
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
				
				FAIL:;
				if(shootingMessage == 1){
					mvprintw(MESSAGES, 0, "You can't move any further\n");
				} else if(shootingMessage == 2){
					mvprintw(MESSAGES, 0, "You aren't shooting a monster location\n");
				}
				shootingMessage = 0;
				input = getch();
				switch(input){
					case KEY_UP:
						if(tempY > characters[0].pos[Y_LOC] - 3 && hardness[tempY - 1][tempX] != 255){
							tempY--;
							goto RESET;
						} else {
							shootingMessage = 1;
							goto FAIL;
						}
						break;
					case KEY_DOWN:
						if(tempY < characters[0].pos[Y_LOC] + 3 && hardness[tempY + 1][tempX] != 255){
							tempY++;
							goto RESET;
						} else {
							shootingMessage = 1;
							goto FAIL;
						}
						break;
					case KEY_RIGHT:
						if(tempX < characters[0].pos[X_LOC] + 3 && hardness[tempY][tempX + 1] != 255){
							tempX++;
							goto RESET;
						} else {
							shootingMessage = 1;
							goto FAIL;
						}
						break;
					case KEY_LEFT:
						if(tempX > characters[0].pos[X_LOC] - 3 && hardness[tempY][tempX - 1] != 255){
							tempX--;
							goto RESET;
						} else {
							shootingMessage = 1;
							goto FAIL;
						}
						break;
					case 's': //shoot the location if it's a monster
						while(monIndex < *num_Mon){
							if(characters[monIndex].pos[Y_LOC] == tempY && characters[monIndex].pos[X_LOC] == tempX){
								charIndex = monIndex;
								onMonster = true;
							}
							monIndex++;
						}
						
						if(onMonster){
							characters[charIndex].health -= equipment[characters[0].equiped[2]].DamageBonus.base;
							for(j = 0; j < equipment[characters[0].equiped[2]].DamageBonus.dice; j++){
								randDodge = 1 + rand() % 1000;
								if(randDodge > characters[charIndex].dodge){ //If randDodge is above the characters dodge ability then the attack hits
									characters[charIndex].health -= 1 + rand() % equipment[characters[0].equiped[2]].DamageBonus.sides;
								}
							} 
							// mvwprintw(w, MESSAGES, 0, "You shot %c with %c\n", characters[charIndex].c, equipment[characters[0].equiped[2]].c);
							// sleep(2);
							onMonster = false;
						} else {
							shootingMessage = 2;
							goto FAIL;
						}
						break;
					case 27:
						break;
					default:
						goto FAIL;
						break;
				}
				endwin();
			} else {
				message = 4;
				goto ER;
			}
			goto DFT;
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
						|| dungeon[row][col] == '-' || dungeon[row][col] == '@' || dungeon[row][col] == '<' || dungeon[row][col] == '>' 
						|| dungeon[row][col] == '+' || dungeon[row][col] == '`'){ //always white
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
		//-------------------------------------------------------------------------------------------------------------------------------
		case 'v':
			WINDOW *tempWin;
			tempWin = newwin(24, 80, 0, 0);
			cursLoc = 0;
			
			REFRESH:;
			wrefresh(tempWin);
			clear();
			printw("###INVENTORY###\n\n");
			for(int k = 0; k < 5; k++){
				if(k == 0){
					printw("Weapons");
				} else if(k == 1){
					printw("Armor");
				} else if(k == 2){
					printw("Ammunition");
				} else if(k == 3){
					printw("Spells");
				} else if(k == 4){
					printw("Storage");
				}
				
				if(k == cursLoc){
					printw(" <<");
				}
				printw("\n");
			}
			printw("\nPress ESC to return");
			iKey = getch();
			switch(iKey){
				case KEY_UP:
					if(cursLoc - 1 >= 0){
						cursLoc--;
					}
					goto REFRESH;	
					break;
				case KEY_DOWN:
					if(cursLoc + 1 < 5){
						cursLoc++;
					}
					goto REFRESH;
					break;
				case ' ':
					if(cursLoc == 0){ //weapons
						tempCurs = 0;
						WINDOW *wp;
						wp = newwin(24, 80, 0, 0);
						
						wpRefresh:;
						
						wrefresh(wp);
						clear();
						printw("###WEAPONS###\n\n");
						for(int k = 0; k < 4; k++){
							if(k == 0){
								printw("Sword: %c", equipment[characters[0].weapons[k]].c);
							} else if(k == 1){
								printw("Offhand: %c", equipment[characters[0].weapons[k]].c);
							} else if(k == 2){
								printw("Ranged: %c", equipment[characters[0].weapons[k]].c);
							} else if(k == 3){
								printw("Wand: %c", equipment[characters[0].weapons[k]].c);
							}
							if(k == tempCurs){
								printw(" <<");
							}
							printw("\n");
						}
						printw("\n\nPress ESC to return");
						wpKey = getch();
						switch(wpKey){
							case KEY_UP:
								if(tempCurs - 1 >= 0){
									tempCurs--;
								}
								goto wpRefresh;	
								break;
							case KEY_DOWN:
								if(tempCurs + 1 < 4){
									tempCurs++;
								}
								goto wpRefresh;
								break;
							case 27:
								delwin(wp);
								goto REFRESH;
							default:
								goto wpRefresh;
						}
					} else if(cursLoc == 1){ //armor
						tempCurs = 0;
						WINDOW *ar;
						ar = newwin(24, 80, 0, 0);
						
						arRefresh:;
						
						wrefresh(ar);
						clear();
						printw("###ARMOR###\n\n");
						for(int k = 0; k < 5; k++){ //armor, helmet, cloak, gloves, shoes
							if(k == 0){
								printw("Armor: %c", equipment[characters[0].armor[k]].c);
							} else if(k == 1){
								printw("Helmet: %c", equipment[characters[0].weapons[k]].c);
							} else if(k == 2){
								printw("Cloak: %c", equipment[characters[0].weapons[k]].c);
							} else if(k == 3){
								printw("Gloves: %c", equipment[characters[0].weapons[k]].c);
							} else if(k == 4){
								printw("Shoes: %c", equipment[characters[0].weapons[k]].c);
							}
							if(k == tempCurs){
								printw(" <<");
							}
							printw("\n");
						}
						printw("\n\nPress ESC to return");
						arKey = getch();
						switch(arKey){
							case KEY_UP:
								if(tempCurs - 1 >= 0){
									tempCurs--;
								}
								goto arRefresh;	
								break;
							case KEY_DOWN:
								if(tempCurs + 1 < 4){
									tempCurs++;
								}
								goto arRefresh;
								break;
							case 27:
								delwin(ar);
								goto REFRESH;
							default:
								goto arRefresh;
						}
					} else if(cursLoc == 2){ //ammuntion
						tempCurs = 0;
						WINDOW *am;
						am = newwin(24, 80, 0, 0);
						
						amRefresh:;
						
						wrefresh(am);
						clear();
						printw("###AMMUNITION###\n\n");
						for(int k = 0; k < 3; k++){
							if(k == 0){
								printw("Arrows: %c", equipment[characters[0].ammunition[k]].c);
							} else if(k == 1){
								printw("??: %c", equipment[characters[0].ammunition[k]].c);
							} else if(k == 2){
								printw("??: %c", equipment[characters[0].ammunition[k]].c);
							}
							if(k == tempCurs){
								printw(" <<");
							}
							printw("\n");
						}
						printw("\n\nPress ESC to return");
						amKey = getch();
						switch(amKey){
							case KEY_UP:
								if(tempCurs - 1 >= 0){
									tempCurs--;
								}
								goto amRefresh;	
								break;
							case KEY_DOWN:
								if(tempCurs + 1 < 4){
									tempCurs++;
								}
								goto amRefresh;
								break;
							case 27:
								delwin(am);
								goto REFRESH;
							default:
								goto amRefresh;
						}
					} else if(cursLoc == 3){ //spells
						tempCurs = 0;
						WINDOW *sp;
						sp = newwin(24, 80, 0, 0);
						
						spRefresh:;
						
						wrefresh(sp);
						clear();
						printw("###SPELLS###\n\n");
						for(int k = 0; k < 5; k++){
							printw("%d: %c", k + 1, equipment[characters[0].spells[k]].c);
							if(k == tempCurs){
								printw(" <<");
							}
							printw("\n");
						}
						printw("\n\nPress ESC to return");
						spKey = getch();
						switch(spKey){
							case KEY_UP:
								if(tempCurs - 1 >= 0){
									tempCurs--;
								}
								goto spRefresh;	
								break;
							case KEY_DOWN:
								if(tempCurs + 1 < 4){
									tempCurs++;
								}
								goto spRefresh;
								break;
							case 27:
								delwin(sp);
								goto REFRESH;
							default:
								goto spRefresh;
						}
					} else if(cursLoc == 4){ //storage
					// ge[NUM_STORAGE]; //stores items index. Can include anything, but should be used for equipment
						tempCurs = 0;
						WINDOW *st;
						st = newwin(24, 80, 0, 0);
						
						stRefresh:;
						
						wrefresh(st);
						clear();
						printw("###STORAGE###\n\n");
						for(int k = 0; k < NUM_STORAGE; k++){
							printw("%d: %c", k + 1, equipment[characters[0].weapons[k]].c);
							if(k == tempCurs){
								printw(" <<");
							}
							printw("\n");
						}
						printw("\n\nPress ESC to return");
						stKey = getch();
						switch(stKey){
							case KEY_UP:
								if(tempCurs - 1 >= 0){
									tempCurs--;
								}
								goto stRefresh;	
								break;
							case KEY_DOWN:
								if(tempCurs + 1 < 4){
									tempCurs++;
								}
								goto stRefresh;
								break;
							case 27:
								delwin(st);
								goto REFRESH;
							default:
								goto stRefresh;
						}
					}
					goto REFRESH;
				case 27:
					goto DFT;
					break;
				default:
					goto REFRESH;
			}
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case 'i': //lists PC inventory/storage
			WINDOW *i;
			i = newwin(24, 80, 0, 0);
			cursLoc = 0;
			
			INV:;
			wrefresh(i);
			clear();
			printw("###INVENTORY###\n\n");
			for(int k = 0; k < NUM_STORAGE; k++){
				printw("<%d>: %c", k, equipment[characters[0].storage[k]].c);
				if(k == cursLoc){
					printw(" <<");
				}
				printw("\n");
			}
			printw("\nPress ESC to return");
			iKey = getch();
			switch(iKey){
				case KEY_UP:
					if(cursLoc - 1 >= 0){
						cursLoc--;
					}
					goto INV;	
					break;
				case KEY_DOWN:
					if(cursLoc + 1 < NUM_STORAGE){
						cursLoc++;
					}
					goto INV;
					break;
				case 27:
					break;
				case 'w': //wear the item
					if(equipment[characters[0].storage[cursLoc]].equipment.weapon){
						if(characters[0].equiped[0] == -1){ //if its empty then place it there
							characters[0].equiped[0] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[0]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[0]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[0]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[0]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[0]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[0]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[0];
							characters[0].s -= equipment[characters[0].equiped[0]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[0]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[0]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[0]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[0]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[0]].DodgeBonus;
							characters[0].equiped[0] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[0]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[0]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[0]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[0]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[0]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[0]].DodgeBonus;
						}
						// printw("You equiped the weapon: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.offhand){
						if(characters[0].equiped[1] == -1){ //if its empty then place it there
							characters[0].equiped[1] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[1]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[1]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[1]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[1]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[1]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[1]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[1];
							characters[0].s -= equipment[characters[0].equiped[1]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[1]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[1]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[1]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[1]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[1]].DodgeBonus;
							characters[0].equiped[1] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[1]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[1]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[1]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[1]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[1]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[1]].DodgeBonus;
						}
						// printw("You equiped the offhand: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.ranged){
						if(characters[0].equiped[2] == -1){ //if its empty then place it there
							characters[0].equiped[2] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[2]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[2]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[2]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[2]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[2]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[2]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[2];
							characters[0].s -= equipment[characters[0].equiped[2]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[2]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[2]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[2]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[2]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[2]].DodgeBonus;
							characters[0].equiped[2] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[2]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[2]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[2]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[2]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[2]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[2]].DodgeBonus;
						}
						// printw("You equiped the ranged weapon: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.armor){
						if(characters[0].equiped[3] == -1){ //if its empty then place it there
							characters[0].equiped[3] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[3]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[3]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[3]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[3]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[3]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[3]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[3];
							characters[0].s -= equipment[characters[0].equiped[3]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[3]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[3]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[3]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[3]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[3]].DodgeBonus;
							characters[0].equiped[3] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[3]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[3]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[3]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[3]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[3]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[3]].DodgeBonus;
						}
						// printw("You equiped the armor: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.helmet){
						if(characters[0].equiped[4] == -1){ //if its empty then place it there
							characters[0].equiped[4] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[4]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[4]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[4]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[4]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[4]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[4]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[4];
							characters[0].s -= equipment[characters[0].equiped[4]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[4]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[4]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[4]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[4]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[4]].DodgeBonus;
							characters[0].equiped[4] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[4]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[4]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[4]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[4]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[4]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[4]].DodgeBonus;
						}
						// printw("You equiped the helmet: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.cloak){
						if(characters[0].equiped[5] == -1){ //if its empty then place it there
							characters[0].equiped[5] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[5]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[5]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[5]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[5]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[5]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[5]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[5];
							characters[0].s -= equipment[characters[0].equiped[5]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[5]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[5]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[5]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[5]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[5]].DodgeBonus;
							characters[0].equiped[5] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[5]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[5]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[5]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[5]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[5]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[5]].DodgeBonus;
						}
						// printw("You equiped the cloak: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.gloves){
						if(characters[0].equiped[6] == -1){ //if its empty then place it there
							characters[0].equiped[6] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[6]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[6]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[6]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[6]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[6]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[6]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[6];
							characters[0].s -= equipment[characters[0].equiped[6]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[6]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[6]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[6]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[6]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[6]].DodgeBonus;
							characters[0].equiped[6] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[6]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[6]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[6]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[6]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[6]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[6]].DodgeBonus;
						}
						// printw("You equiped the gloves: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.boots){
						if(characters[0].equiped[7] == -1){ //if its empty then place it there
							characters[0].equiped[7] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[7]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[7]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[7]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[7]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[7]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[7]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[7];
							characters[0].s -= equipment[characters[0].equiped[7]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[7]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[7]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[7]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[7]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[7]].DodgeBonus;
							characters[0].equiped[7] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[7]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[7]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[7]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[7]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[7]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[7]].DodgeBonus;
						}
						//printw("You equiped the boots: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.amulet){
						if(characters[0].equiped[8] == -1){ //if its empty then place it there
							characters[0].equiped[8] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[8]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[8]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[8]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[8]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[8]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[8]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[8];
							characters[0].s -= equipment[characters[0].equiped[8]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[8]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[8]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[8]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[8]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[8]].DodgeBonus;
							characters[0].equiped[8] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[8]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[8]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[8]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[8]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[8]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[8]].DodgeBonus;
						}
						//printw("You equiped the amulet: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.light){
						if(characters[0].equiped[9] == -1){ //if its empty then place it there
							characters[0].equiped[9] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[9]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[9]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[9]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[9]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[9]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[9]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[9];
							characters[0].s -= equipment[characters[0].equiped[9]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[9]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[9]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[9]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[9]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[9]].DodgeBonus;
							characters[0].equiped[9] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[9]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[9]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[9]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[9]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[9]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[9]].DodgeBonus;
						}
						//printw("You equiped the light: %c\n", equipment[characters[0].storage[cursLoc]]);
					} else if(equipment[characters[0].storage[cursLoc]].equipment.ring){
						if(characters[0].equiped[10] == -1){ //if its empty then place it there
							characters[0].equiped[10] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[10]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[10]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[10]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[10]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[10]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[10]].DodgeBonus;
						} else if(characters[0].equiped[11] == -1){ //if its empty then place it there
							characters[0].equiped[11] = characters[0].storage[cursLoc]; //assign item to equiped
							characters[0].storage[cursLoc] = -1; //remove from inventory
							//apply boosts to stats
							characters[0].s += equipment[characters[0].equiped[11]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[11]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[11]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[11]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[11]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[11]].DodgeBonus;
						} else { //flip the character index locations from the storage and equiped if its occupied
							temp = characters[0].equiped[10];
							characters[0].s -= equipment[characters[0].equiped[10]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[10]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[10]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[10]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[10]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[10]].DodgeBonus;
							characters[0].equiped[10] = characters[0].storage[cursLoc];
							characters[0].storage[cursLoc] = temp;
							characters[0].s += equipment[characters[0].equiped[10]].SpeedBonus;
							characters[0].health += equipment[characters[0].equiped[10]].DefenseBonus;
							characters[0].attackDamage.base += equipment[characters[0].equiped[10]].DamageBonus.base;
							characters[0].attackDamage.dice += equipment[characters[0].equiped[10]].DamageBonus.dice;
							characters[0].attackDamage.sides += equipment[characters[0].equiped[10]].DamageBonus.sides;
							characters[0].dodge += equipment[characters[0].equiped[10]].DodgeBonus;
						}
						//printw("You equiped the armor: %c\n", equipment[characters[0].storage[cursLoc]]);
					}
					// if(!isSwapped){
						// characters[0].s += equipment[characters[0].storage[cursLoc]].SpeedBonus;
						// characters[0].health += equipment[characters[0].storage[cursLoc]].DefenseBonus;
						// characters[0].attackDamage.base += equipment[characters[0].storage[cursLoc]].DamageBonus.base;
						// characters[0].attackDamage.dice += equipment[characters[0].storage[cursLoc]].DamageBonus.dice;
						// characters[0].attackDamage.sides += equipment[characters[0].storage[cursLoc]].DamageBonus.sides;
						// characters[0].storage[cursLoc] = -1;
					// }
					// isSwapped = false;
					goto INV;
					break;
				case 'd': //drop the item
					equipment[characters[0].storage[cursLoc]].isHeld = false; //item is no longer held by the PC
					while(!isDropped){
						randLoc = rand() % 8;
						if(hardness[characters[0].pos[Y_LOC] + randLocArr[randLoc][1]][characters[0].pos[X_LOC] + randLocArr[randLoc][0]] == 0){ //Right
							//if the space is open then don't worry about updating info
							if(dungeon[characters[0].pos[Y_LOC] + randLocArr[randLoc][1]][characters[0].pos[X_LOC] + randLocArr[randLoc][0]] == '.' || dungeon[characters[0].pos[Y_LOC] + randLocArr[randLoc][1]][characters[0].pos[X_LOC] +  + randLocArr[randLoc][0]] == '#'){
								equipment[characters[0].storage[cursLoc]].lv = dungeon[characters[0].pos[Y_LOC]  + randLocArr[randLoc][1]][characters[0].pos[X_LOC] + randLocArr[randLoc][0]];
								dungeon[characters[0].pos[Y_LOC]  + randLocArr[randLoc][1]][characters[0].pos[X_LOC] + randLocArr[randLoc][0]] = equipment[characters[0].storage[cursLoc]].c;
								equipment[characters[0].storage[cursLoc]].pos[Y_LOC] = characters[0].pos[Y_LOC] + randLocArr[randLoc][1];
								equipment[characters[0].storage[cursLoc]].pos[X_LOC] = characters[0].pos[X_LOC] + randLocArr[randLoc][0];
								if(equipment[characters[0].storage[cursLoc]].equipment.trap){
									equipment[characters[0].storage[cursLoc]].color.WHITE = false;
									equipment[characters[0].storage[cursLoc]].color.RED = true;
								}
								isDropped = true;
							} else { //space is occupied
								for(int h = 0; h < *num_Mon; h++){ //find monster in space
									if(characters[0].pos[Y_LOC] + randLocArr[randLoc][1] == characters[h].pos[Y_LOC] && characters[0].pos[X_LOC] + randLocArr[randLoc][0] == characters[h].pos[X_LOC]){
										if(characters[h].lv == '.' || characters[h].lv == '#'){ //monster doesn't have an object as its lv already
											characters[h].lv = equipment[characters[0].storage[cursLoc]].c;
											equipment[characters[0].storage[cursLoc]].pos[Y_LOC] = characters[0].pos[Y_LOC] + randLocArr[randLoc][1];
											equipment[characters[0].storage[cursLoc]].pos[X_LOC] = characters[0].pos[X_LOC] + randLocArr[randLoc][0];
											isDropped = true;
										}
									}
								}
							}
						}
					}
					isDropped = false;
					printw("You dropped the item: %c\n", equipment[characters[0].storage[cursLoc]]);
					equipment[characters[0].storage[cursLoc]].isHeld = false;
					characters[0].storage[cursLoc] = -1; //the storage slot on the pc is emptied
					goto INV;
					break;
				case 'x': //delete the item
					equipment[characters[0].storage[cursLoc]].a = false; //item is destroyed when expunged
					equipment[characters[0].storage[cursLoc]].isHeld = false; //item is destroyed when expunged
					printw("You deleted the item: %c\n", equipment[characters[0].storage[cursLoc]]);
					characters[0].storage[cursLoc] = -1; //the storage slot on the pc is emptied
					goto INV;
					break;
				case 'I': //examine the item
					wrefresh(i);
					clear();
					printw("%c\n", equipment[characters[0].storage[cursLoc]].c);
					printw("NAME: ");
					for(int k = 0; k < 78; k++){
						printw("%c", equipment[characters[0].storage[cursLoc]].name[k]);
					}
					printw("\n");
					printw("DESCRIPTION: ");
					for(int k = 0; k < 1024; k++){
						printw("%c", equipment[characters[0].storage[cursLoc]].description[k]);
					}
					printw("\n");
					printw("EQUIPMENT TYPE: ");
					if(equipment[characters[0].storage[cursLoc]].equipment.weapon){
						printw("Weapon\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.offhand){
						printw("Offhand\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.ranged){
						printw("Ranged\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.armor){
						printw("Armor\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.helmet){
						printw("Helmet\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.cloak){
						printw("Cloak\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.gloves){
						printw("Gloves\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.boots){
						printw("Boots\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.ring){
						printw("Ring\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.amulet){
						printw("Amulet\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.light){
						printw("Light\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.scroll){
						printw("Scroll\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.book){
						printw("Book\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.flask){
						printw("Flask\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.gold){
						printw("Gold\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.ammunition){
						printw("Ammunition\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.food){
						printw("Food\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.wand){
						printw("Wand\n");
					} else if(equipment[characters[0].storage[cursLoc]].equipment.container){
						printw("Container\n");
					}
					printw("COLOR: ");
					if(equipment[characters[0].storage[cursLoc]].color.RED){
						printw("Red\n");
					} else if(equipment[characters[0].storage[cursLoc]].color.GREEN){
						printw("Green\n");
					} else if(equipment[characters[0].storage[cursLoc]].color.YELLOW){
						printw("Yellow\n");
					} else if(equipment[characters[0].storage[cursLoc]].color.BLUE){
						printw("Blue\n");
					} else if(equipment[characters[0].storage[cursLoc]].color.MAGENTA){
						printw("Magenta\n");
					} else if(equipment[characters[0].storage[cursLoc]].color.CYAN){
						printw("Cyan\n");
					} else if(equipment[characters[0].storage[cursLoc]].color.WHITE){
						printw("White\n");
					} 
					printw("HIT BONUS: %d\n", equipment[characters[0].storage[cursLoc]].HitBonus);
					printw("DAMAGE BONUS: %d+%dd%d\n", equipment[characters[0].storage[cursLoc]].DamageBonus.base, equipment[characters[0].storage[cursLoc]].DamageBonus.dice, equipment[characters[0].storage[cursLoc]].DamageBonus.sides);
					printw("DODGE BONUS: %d\n", equipment[characters[0].storage[cursLoc]].DodgeBonus);
					printw("DEFENSE BONUS: %d\n", equipment[characters[0].storage[cursLoc]].DefenseBonus);
					printw("WEIGHT: %d\n", equipment[characters[0].storage[cursLoc]].weight);
					printw("SPEED BONUS: %d\n", equipment[characters[0].storage[cursLoc]].SpeedBonus);
					printw("VALUE: %d\n", equipment[characters[0].storage[cursLoc]].value);
					printw("STATUS: %b\n", equipment[characters[0].storage[cursLoc]].status);
					printw("RARITY: %d\n", equipment[characters[0].storage[cursLoc]].rarity);
					printw("Y: %d X: %d\n", equipment[characters[0].storage[cursLoc]].pos[Y_LOC], equipment[characters[0].storage[cursLoc]].pos[X_LOC]);
					printw("SPECIAL ATTRIBUTE: %b\n", equipment[characters[0].storage[cursLoc]].SpecialAtr);
					printw("\nPRESS ANY KEY TO EXIT");
					iKey = getch();
					switch(iKey){
						default:
							break;
					}
					goto INV;
					break;
				default:
					goto INV;
					break;
			}
			delwin(i);
			goto DFT;
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case 'e': //lists PC equiped items
			WINDOW *e;
			e = newwin(24, 80, 0, 0);
			cursLoc = 0;
			
			EQP:;
			wrefresh(e);
			clear();
			printw("###EQUIPED###\n\n");
			for(int k = 0; k < NUM_STORAGE + 2; k++){
				if(k == 0){
					printw("<a>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 1){
					printw("<b>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 2){
					printw("<c>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 3){
					printw("<d>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 4){
					printw("<e>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 5){
					printw("<f>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 6){
					printw("<g>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 7){
					printw("<h>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 8){
					printw("<i>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 9){
					printw("<j>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 10){
					printw("<k>: %c", equipment[characters[0].equiped[k]].c);
				} else if(k == 11){
					printw("<l>: %c", equipment[characters[0].equiped[k]].c);
				} 
				if(k == cursLoc){
					printw("<<");
				}
				printw("\n");
			}
			
			printw("\nPress ESC to return\n");
			eKey = getch();
			switch(eKey){
				case KEY_UP:
					if(cursLoc - 1 >= 0){
						cursLoc--;
					}
					goto EQP;	
					break;
				case KEY_DOWN:
					if(cursLoc + 1 < NUM_STORAGE + 2){
						cursLoc++;
					}
					goto EQP;
					break;
				case 27:
					break;
				case 'd': //drop the item
					equipment[characters[0].equiped[cursLoc]].isHeld = false; //item is destroyed when dropped but not expunged
					characters[0].s -= equipment[characters[0].equiped[cursLoc]].SpeedBonus;
					characters[0].health -= equipment[characters[0].equiped[cursLoc]].DefenseBonus;
					characters[0].attackDamage.base -= equipment[characters[0].equiped[cursLoc]].DamageBonus.base;
					characters[0].attackDamage.dice -= equipment[characters[0].equiped[cursLoc]].DamageBonus.dice;
					characters[0].attackDamage.sides -= equipment[characters[0].equiped[cursLoc]].DamageBonus.sides;
					characters[0].dodge -= equipment[characters[0].equiped[cursLoc]].DodgeBonus;
					while(!isDropped){
						randLoc = rand() % 8;
						if(hardness[characters[0].pos[Y_LOC] + randLocArr[randLoc][1]][characters[0].pos[X_LOC] + randLocArr[randLoc][0]] == 0){ //Right
							//if the space is open then don't worry about updating info
							if(dungeon[characters[0].pos[Y_LOC] + randLocArr[randLoc][1]][characters[0].pos[X_LOC] + randLocArr[randLoc][0]] == '.' || dungeon[characters[0].pos[Y_LOC] + randLocArr[randLoc][1]][characters[0].pos[X_LOC] +  + randLocArr[randLoc][0]] == '#'){
								equipment[characters[0].equiped[cursLoc]].lv = dungeon[characters[0].pos[Y_LOC]  + randLocArr[randLoc][1]][characters[0].pos[X_LOC] + randLocArr[randLoc][0]];
								dungeon[characters[0].pos[Y_LOC]  + randLocArr[randLoc][1]][characters[0].pos[X_LOC] + randLocArr[randLoc][0]] = equipment[characters[0].equiped[cursLoc]].c;
								equipment[characters[0].equiped[cursLoc]].pos[Y_LOC] = characters[0].pos[Y_LOC] + randLocArr[randLoc][1];
								equipment[characters[0].equiped[cursLoc]].pos[X_LOC] = characters[0].pos[X_LOC] + randLocArr[randLoc][0];
								if(equipment[characters[0].equiped[cursLoc]].equipment.trap){
									equipment[characters[0].equiped[cursLoc]].color.WHITE = false;
									equipment[characters[0].equiped[cursLoc]].color.RED = true;
								}
								isDropped = true;
							} else { //space is occupied
								for(int h = 0; h < *num_Mon; h++){ //find monster in space
									if(characters[0].pos[Y_LOC] + randLocArr[randLoc][1] == characters[h].pos[Y_LOC] && characters[0].pos[X_LOC] + randLocArr[randLoc][0] == characters[h].pos[X_LOC]){
										if(characters[h].lv == '.' || characters[h].lv == '#'){ //monster doesn't have an object as its lv already
											characters[h].lv = equipment[characters[0].equiped[cursLoc]].c;
											equipment[characters[0].equiped[cursLoc]].pos[Y_LOC] = characters[0].pos[Y_LOC] + randLocArr[randLoc][1];
											equipment[characters[0].equiped[cursLoc]].pos[X_LOC] = characters[0].pos[X_LOC] + randLocArr[randLoc][0];
											isDropped = true;
										}
									}
								}
							}
						}
					}
					isDropped = false;
					// printw("You dropped the item: %c\n", equipment[characters[0].equiped[cursLoc]]);
					equipment[characters[0].equiped[cursLoc]].isHeld = false;
					characters[0].equiped[cursLoc] = -1; //the equiped slot on the pc is emptied
					goto EQP;
					break;
				case 'x': //delete the item
					equipment[characters[0].equiped[cursLoc]].a = false; //item is destroyed when dropped
					equipment[characters[0].equiped[cursLoc]].isHeld = false; //item is destroyed when expunged
					characters[0].s -= equipment[characters[0].equiped[cursLoc]].SpeedBonus;
					characters[0].health -= equipment[characters[0].equiped[cursLoc]].DefenseBonus;
					characters[0].attackDamage.base -= equipment[characters[0].equiped[cursLoc]].DamageBonus.base;
					characters[0].attackDamage.dice -= equipment[characters[0].equiped[cursLoc]].DamageBonus.dice;
					characters[0].attackDamage.sides -= equipment[characters[0].equiped[cursLoc]].DamageBonus.sides;
					characters[0].dodge -= equipment[characters[0].equiped[cursLoc]].DodgeBonus;
					characters[0].equiped[cursLoc] = -1; //the equiped slot on the pc is emptied
					// printw("You deleted the item: %c\n", equipment[characters[0].equiped[cursLoc]]);
					goto EQP;
					break;
				case 't': //take off item
					for(int m = 0; m < NUM_STORAGE; m++){
						if(characters[0].storage[m] == -1){
							//decrement boosts to stats
							characters[0].storage[m] = characters[0].equiped[cursLoc];
							// printw("You took off the item: %c\n", equipment[characters[0].equiped[cursLoc]]);
							characters[0].s -= equipment[characters[0].equiped[cursLoc]].SpeedBonus;
							characters[0].health -= equipment[characters[0].equiped[cursLoc]].DefenseBonus;
							characters[0].attackDamage.base -= equipment[characters[0].equiped[cursLoc]].DamageBonus.base;
							characters[0].attackDamage.dice -= equipment[characters[0].equiped[cursLoc]].DamageBonus.dice;
							characters[0].attackDamage.sides -= equipment[characters[0].equiped[cursLoc]].DamageBonus.sides;
							characters[0].dodge -= equipment[characters[0].equiped[cursLoc]].DodgeBonus;
							characters[0].equiped[cursLoc] = -1;
							takenOff = true;
						} else {
							//message??
						}
					}
					if(!takenOff){
						printw("Unable to take off item, no storage available\n");
					}
					goto EQP;
					break;
				case 'I': //examine the item
					wrefresh(e);
					clear();
					printw("%c\n", equipment[characters[0].equiped[cursLoc]].c);
					printw("NAME: ");
					for(int k = 0; k < 78; k++){
						printw("%c", equipment[characters[0].equiped[cursLoc]].name[k]);
					}
					printw("\n");
					printw("DESCRIPTION: ");
					for(int k = 0; k < 1024; k++){
						printw("%c", equipment[characters[0].equiped[cursLoc]].description[k]);
					}
					printw("\n");
					printw("EQUIPMENT TYPE: ");
					if(equipment[characters[0].equiped[cursLoc]].equipment.weapon){
						printw("Weapon\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.offhand){
						printw("Offhand\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.ranged){
						printw("Ranged\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.armor){
						printw("Armor\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.helmet){
						printw("Helmet\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.cloak){
						printw("Cloak\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.gloves){
						printw("Gloves\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.boots){
						printw("Boots\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.ring){
						printw("Ring\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.amulet){
						printw("Amulet\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.light){
						printw("Light\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.scroll){
						printw("Scroll\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.book){
						printw("Book\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.flask){
						printw("Flask\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.gold){
						printw("Gold\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.ammunition){
						printw("Ammunition\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.food){
						printw("Food\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.wand){
						printw("Wand\n");
					} else if(equipment[characters[0].equiped[cursLoc]].equipment.container){
						printw("Container\n");
					}
					printw("COLOR: ");
					if(equipment[characters[0].equiped[cursLoc]].color.RED){
						printw("Red\n");
					} else if(equipment[characters[0].equiped[cursLoc]].color.GREEN){
						printw("Green\n");
					} else if(equipment[characters[0].equiped[cursLoc]].color.YELLOW){
						printw("Yellow\n");
					} else if(equipment[characters[0].equiped[cursLoc]].color.BLUE){
						printw("Blue\n");
					} else if(equipment[characters[0].equiped[cursLoc]].color.MAGENTA){
						printw("Magenta\n");
					} else if(equipment[characters[0].equiped[cursLoc]].color.CYAN){
						printw("Cyan\n");
					} else if(equipment[characters[0].equiped[cursLoc]].color.WHITE){
						printw("White\n");
					} 
					printw("HIT BONUS: %d\n", equipment[characters[0].equiped[cursLoc]].HitBonus);
					printw("DAMAGE BONUS: %d+%dd%d\n", equipment[characters[0].equiped[cursLoc]].DamageBonus.base, equipment[characters[0].equiped[cursLoc]].DamageBonus.dice, equipment[characters[0].equiped[cursLoc]].DamageBonus.sides);
					printw("DODGE BONUS: %d\n", equipment[characters[0].equiped[cursLoc]].DodgeBonus);
					printw("DEFENSE BONUS: %d\n", equipment[characters[0].equiped[cursLoc]].DefenseBonus);
					printw("WEIGHT: %d\n", equipment[characters[0].equiped[cursLoc]].weight);
					printw("SPEED BONUS: %d\n", equipment[characters[0].equiped[cursLoc]].SpeedBonus);
					printw("VALUE: %d\n", equipment[characters[0].equiped[cursLoc]].value);
					printw("STATUS: %b\n", equipment[characters[0].equiped[cursLoc]].status);
					printw("RARITY: %d\n", equipment[characters[0].equiped[cursLoc]].rarity);
					printw("Y: %d X: %d\n", equipment[characters[0].equiped[cursLoc]].pos[Y_LOC], equipment[characters[0].equiped[cursLoc]].pos[X_LOC]);
					printw("SPECIAL ATTRIBUTE: %b\n", equipment[characters[0].equiped[cursLoc]].SpecialAtr);
					printw("\nPRESS ANY KEY TO EXIT");
					eKey = getch();
					switch(eKey){
						default:
							break;
					}
					goto EQP;
					break;
				default:
					goto EQP;
					break;
			}
			delwin(e);
			goto DFT;
			break;
		//-------------------------------------------------------------------------------------------------------------------------------
		case 'L':
			//change the PC character to the dungeon character, update the specs later after user has chosen position to move
			dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv;
			fogDungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].lv;
			tempX = characters[0].pos[X_LOC];
			tempY = characters[0].pos[Y_LOC];
			LCS:;
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
				OOPS:;
				clear();
				mvprintw(0, 0, "Commands:\n<7 or y>: Move Cursor Up-Left\n<8 or k>: Move Cursor Up\n<9 or u>: Move Cursor Up-Right\n<6 or l>: Move Cursor Right\n<3 or n>: Move Cursor Down-Right\n<2 or j>: Move Cursor Down\n<1 or b>: Move Cursor Down-Left\n<4 or h>: Move Cursor Left\n<t>: Look at object info\n\nPress ESC to exit");
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
				// case 'y': //UP-LEFT
					// HM:;
					// //Update '*' position
					// if(hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC] - 1] != 255){
						// characters[0].pos[Y_LOC] -= 1;
						// characters[0].pos[X_LOC] -= 1;
					// }
					// goto LCS;
					// break;
				// case '7': //UP-LEFT
					// goto HM; //Goes to default UP-LEFT case
					// break;
				// case KEY_HOME: //UP-LEFT
					// goto HM; //Goes to default UP-LEFT case
					// break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case KEY_UP: //UP
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC]] != 255){
						characters[0].pos[Y_LOC] -= 1;
					}
					goto LCS;
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case KEY_RIGHT: //RIGHT
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] + 1] != 255){
						characters[0].pos[X_LOC] += 1;
					}
					goto LCS;
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case KEY_DOWN: //DOWN
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC]] != 255){
						characters[0].pos[Y_LOC] += 1;
					}
					goto LCS;
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case KEY_LEFT: //LEFT
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] - 1] != 255){
						characters[0].pos[X_LOC] -= 1;
					}
					goto LCS;
					break;
				case 't': //Look at item or monster info
					if(characters[0].pos[X_LOC] != ' ' || characters[0].pos[X_LOC] != '.' || characters[0].pos[X_LOC] != '#'){
						WINDOW *t;
						t = newwin(24, 80, 0, 0);
						for(int d = 1; d < *num_Mon; d++){
							if(characters[0].pos[Y_LOC] == characters[d].pos[Y_LOC] && characters[0].pos[X_LOC] == characters[d].pos[X_LOC]){
								wrefresh(i);
								clear();
								printw("%c\n", characters[d].c);
								printw("NAME: ");
								for(int k = 0; k < 78; k++){
									printw("%c", characters[d].name[k]);
								}
								printw("\n");
								printw("DESCRIPTION: ");
								for(int k = 0; k < 1024; k++){
									printw("%c", characters[0].description[k]);
								}
								printw("\n");
								printw("COLOR: ");
								if(characters[d].color.RED){
									printw("Red\n");
								} else if(characters[d].color.GREEN){
									printw("Green\n");
								} else if(characters[d].color.YELLOW){
									printw("Yellow\n");
								} else if(characters[d].color.BLUE){
									printw("Blue\n");
								} else if(characters[d].color.MAGENTA){
									printw("Magenta\n");
								} else if(characters[d].color.CYAN){
									printw("Cyan\n");
								} else if(characters[d].color.WHITE){
									printw("White\n");
								} 
								printw("SPEED: %d\n", characters[d].s);
								printw("ABILITIES:\n");
								if(characters[d].i){
									printw("INTELLIGENT\n");
								}
								if(characters[d].t){
									printw("TELEPATHIC\n");
								}
								if(characters[d].tu){
									printw("TUNNEL\n");
								}
								if(characters[d].e){
									printw("ERRATIC\n");
								}
								if(characters[d].p){
									printw("PASS\n");
								}
								if(characters[d].pu){
									printw("PICK UP\n");
								}
								if(characters[d].d){
									printw("DESTROY\n");
								}
								if(characters[d].u){
									printw("UNIQUE\n");
								}
								if(characters[d].b){
									printw("FINAL BOSS\n");
								}
								printw("HITPOINTS: %d\n", characters[d].health);
								printw("DAMAGE: %d+%dd%d\n", characters[d].attackDamage.base, characters[d].attackDamage.dice, characters[d].attackDamage.sides);
								printw("RARITY: %d\n", characters[d].rarity);
								printw("Y: %d X: %d\n", characters[d].pos[Y_LOC], characters[d].pos[X_LOC]);
								printw("\nPRESS ANY KEY TO EXIT");
								iKey = getch();
								switch(iKey){
									default:
										break;
								}
							}
						}
						goto LCS;
						break;
					case 27:
						//reset all the info
						characters[0].pos[Y_LOC] = tempY;
						characters[0].pos[X_LOC] = tempX;
						dungeon[tempY][tempX] = characters[0].c;
						fogDungeon[tempY][tempX] = characters[0].c;
						goto DFT;
						break;
					default:
						goto OOPS;
						break;
					}
			}
			break;
		case 't':
			//teleportPC(dungeon, fogDungeon, hardness, characters, num_Mon);
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
						|| dungeon[row][col] == '-' || dungeon[row][col] == '@' || dungeon[row][col] == '<' || dungeon[row][col] == '>' ||
						dungeon[row][col] == '+' || dungeon[row][col] == '`'){ //always white
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
				case KEY_UP: //UP
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] - 1][characters[0].pos[X_LOC]] != 255){
						characters[0].pos[Y_LOC] -= 1;
					}
					goto BSC;
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case KEY_RIGHT: //RIGHT
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] + 1] != 255){
						characters[0].pos[X_LOC] += 1;
					}
					goto BSC;
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case KEY_DOWN: //DOWN
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC] + 1][characters[0].pos[X_LOC]] != 255){
						characters[0].pos[Y_LOC] += 1;
					}
					goto BSC;
					break;
				//-------------------------------------------------------------------------------------------------------------------------------	
				case KEY_LEFT: //LEFT
					//Update '*' position
					if(hardness[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC] - 1] != 255){
						characters[0].pos[X_LOC] -= 1;
					}
					goto BSC;
					break;
				case 'r': //random location
					//with these random values we can never reach the immutable rock
					randY = 2 + rand() % (GAME_HEIGHT - 3);
					randX = 2 + rand() % (GAME_WIDTH - 3);
					if(dungeon[randY][randX] == '.' || dungeon[randY][randX] == ' ' || dungeon[randY][randX] == '#'){
						characters[0].pos[Y_LOC] = randY; //apply new pos to the PC
						characters[0].pos[X_LOC] = randX; //apply new pos to the PC
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
					} else {
						goto BSC;
					}
					break;
				case 't': //user chosen position
					if(dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] == '.' || dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] == ' ' || dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] == '#'){
						characters[0].lv = dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]]; //update PC's lv
						dungeon[characters[0].pos[Y_LOC]][characters[0].pos[X_LOC]] = characters[0].c; //update the dungeon with the new PC location
						//updates fog map for user
						for(row = characters[0].pos[Y_LOC] - 2; row < characters[0].pos[Y_LOC] + 3; row++){
							for(col = characters[0].pos[X_LOC] - 2; col < characters[0].pos[X_LOC] + 3; col++){
								fogDungeon[row][col] = dungeon[row][col];
							}
						}
					} else {
						goto BSC;
					}
					break;
				default:
					isError = 1;
					goto OOP;
					break;
			}
			break;
		default:
			message = 100;
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
	int randomX, randomY, randLight, randDebris, randJewels;
	int numLight = 0;
	int numDebris = 0;
	bool equipPlaced = false;
	while(i <= *num_Equip && numEquip < minEquip){ //generate all the equipment and place them if rarity allows
		int rarityNum = rand() % 100;
		if(rarityNum < equipment[i].rarity && equipment[i].a && !equipment[i].isHeld){ //if the rarity num is less than the monsters rarity and its alive
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
					equipment[i].isHeld = false;
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
	
	//'+': light
	//'`': debris
	//randomly adds debris and light to the dungeon, unable to pick up
	randLight = 1 + rand() % 5;
	randDebris = 1 + rand() % 5;
	
	while(numLight < randLight){
		randomY = 1 + rand() % GAME_HEIGHT - 1;
		randomX = 1 + rand() % GAME_WIDTH - 2;
		if(dungeon[randomY][randomX] == '.'){
			dungeon[randomY][randomX] = '+';
			numLight++;
		}
	}
	
	while(numDebris < randDebris){
		randomY = 1 + rand() % GAME_HEIGHT - 1;
		randomX = 1 + rand() % GAME_WIDTH - 2;
		if(dungeon[randomY][randomX] == '.'){
			dungeon[randomY][randomX] = '`';
			numDebris++;
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
	int oldX, oldY;
	int isErratic = FALSE;
	int gameStatus = 0;
	bool gameOver = true;
	
	generateMonsters(dungeon, hardness, PC, num_Mon, characters, rooms, number_of_rooms); //generate the PC and the monsters
	characters[0].health = DEF_HEALTH;
	characters[0].attackDamage.base = 0;
	characters[0].attackDamage.dice = 1;
	characters[0].attackDamage.sides = 4;
	for(int m = 0; m < NUM_STORAGE; m++){ //sets the default arrays to be -1, which signifies an empty slot
		characters[0].storage[m] = -1;
		characters[0].equiped[m] = -1;
		// characters[0].weapons[m] = -1;
		// characters[0].spells[m] = -1; 
		// characters[0].armor[m] = -1;
		// characters[0].ammunition[m] = -1;
	}
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
				if(gameOver || !*num_Mon){
					gameStatus = 3;
				}
				gameOver = true;
				if(gameStatus == 2){
					for(i = 0; i < MAX_MONSTERS; i++){
						heap_remove_min(&h); //remove all of the old monsters from the heap
						if(!characters[i].u && !characters[i].a){ //if the monsters aren't unique then bring them back to life for the next level
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
				oldX = p->pos[X_LOC];
				oldY = p->pos[Y_LOC];
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
					deleteMonster(dungeon, characters, p->pos[X_LOC], &moveXdir, p->pos[Y_LOC], &moveYdir, num_Mon, false, equipment);
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
	int i = 0;
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
					} else if(!s.compare("TRAP")){
						equipment[i].equipment.trap = true;
						equipment[i].c = 'T';
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
							equipment[i].HitBonus = stoi(s1);
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
						equipment[i].HitBonus += 1 + rand() % tempSides;
					}
					// cout << equipment[i].HitBonus << endl;
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
							equipment[i].DamageBonus.base = stoi(s1);
							s1 = "";
						} else if(!s2.compare("d")){
							s1.erase(s1.length()-1);
							equipment[i].DamageBonus.dice = stoi(s1);
							s1 = "";
						} else if(!s2.compare("\n")){
							s.erase(s.length()-1);
							s.erase(s.length()-1);
							s1.erase(s1.length()-1);
							s1.erase(s1.length()-1);
							equipment[i].DamageBonus.sides = stoi(s1);
							readingInfo = false;
							compDam = true;
						}
					}
					// cout << equipment[i].DamageBonus.base << "+" << equipment[i].DamageBonus.dice << "d" << equipment[i].DamageBonus.sides << endl;
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
							equipment[i].SpecialAtr = stoi(s1);
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
						equipment[i].SpecialAtr += 1 + rand() % tempSides;
					}
					// cout << equipment[i].SpecialAtr << endl;
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
							equipment[i].DodgeBonus = stoi(s1);
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
						equipment[i].DodgeBonus += 1 + rand() % tempSides;
					}
					// cout << equipment[i].DodgeBonus << endl;
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
							equipment[i].DefenseBonus = stoi(s1);
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
						equipment[i].DefenseBonus += 1 + rand() % tempSides;
					}
					// cout << equipment[i].DefenseBonus << endl;
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
							equipment[i].SpeedBonus = stoi(s1);
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
						equipment[i].SpeedBonus += 1 + rand() % tempSides;
					}
					// cout << equipment[i].SpeedBonus << endl;
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
		int input;
		WINDOW *w;
		w = newwin(40, 80, 0, 0);
		wrefresh(w);
		clear();
		printw("Welcome to the Chad's Dungeon Game!\n\n");
		printw("The game is simple. You are lonely PC in a dungeon surrounded by monsters\nand equipment\n");
		printw("You can pick up the equipment for increased PC specs. \nYou can attack the monsters by attacking their characters position\n");
		printw("You can move up and down the dungeon floors when on top of staircases (< or >).\n");
		printw("You can teleport, but you may not teleport on top of a monster.\n");
		printw("To win the game you must kill every monster on that level\n");
		printw("\nPress any key to view the commands\n");
		input = getch();
		switch(input){
			default:
				break;
		}
		wrefresh(w);
		clear();
		printw("Basic PC Commands:\nKEY_UP: Move Up\nKEY_RIGHT: Move Right\nKEY_DOWN: Move Down\nKEY_LEFT: Move Left\n>: Go Down Stairs\n<: Go Up Stairs\n.: Rest PC\nm: Display Monsters\nQ: Quit\ni: Inventory\n");
		printw("t: Teleport\n	Key Pads: Move to teleport destination\n	t: Teleport to that space\n 	r: Teleport to a random location\n");
		printw("l: Light debris on fire\n");
		printw("s: Shoot a monster with your ranged weapon\n");
		printw("f: View entire dungeon without fog\n");
		printw("i: View PC inventory\n	KEY_UP: Move up list of equipment\n	KEY_DOWN: Move down list of equipment\n	w: Wear the item\n	d: Drop the item\n	x: Delete the item\n	I: View item information\n	Esc: Return to the game\n");
		printw("e: View PC inventory\n	KEY_UP: Move up list of equipment\n	KEY_DOWN: Move down list of equipment\n	t: Take off the item\n	d: Drop the item\n	x: Delete the item\n	I: View item information\n	Esc: Return to the game\n");
		printw("L: Enter viewing window\n	NUM_KEYS: Move location\n	t: View monster information\n	Esc: Return to game\n");
		printw("Q: Quit game\n");
		printw("\nPress any key to enter the game\n");
		input = getch();
		switch(input){
			default:
				break;
		}
		endwin();
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