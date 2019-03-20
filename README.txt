This combined program creates an 80x21 dungeon with random room location and sizes, corridors connecting them, and stairs.

The save_dungeon function will load a new dungeon, print the dungeon, and then save the contents of the dungeon with the a file marker, file size, and file type. 
It saves only the "shell" of the key pieces of rooms and staircase locations.

load_dungeon will go to the path assigned and read the binary info within the file and save the contents to the necessary 
variables. It will then print the dungeon and exit.

A switch case with check for --save, --load, and --load--save DO NOT ADD A SPACE IN BETWEEN LOAD AND SAVE!!!

YOU MUST ENTER AT LEAST ONE CHARACTER TO RECEIVE AN ERROR MESSAGE OR RUN A COMMAND! IF NOT YOU WILL RECIEVE A SEGEMENTATION ERROR!