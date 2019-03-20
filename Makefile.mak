all: game
	
game: game.c
	gcc -g3 game.c -o game
	
clean: 
	rm game
