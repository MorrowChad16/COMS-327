gameGenerator creates a dungeon with 6 or more randomly generated rooms that are connected by at least one corridor.
A random up and down stairs is added throughout the playable dungeon

generateDungeon creates the 80x24 window with a border for the playable area

generateRooms creates and adds 6 or more rooms to the dungeon area. It returns the number of rooms for generateCorridors

generateCorridors connects the rooms through Euclidean formula

generateStairs searches randomly through the area to add stairs

printDungeon will finally print the finished dungeon

