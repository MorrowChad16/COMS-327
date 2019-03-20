all: game
		
game: game.c heap.c
	gcc -g3 game.c -o game
	
clean: 
	rm game
