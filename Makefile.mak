all: game
		
game: game.c heap.c
	gcc -g3 game.c heap.c -lncurses -o game
	
clean: 
	rm game