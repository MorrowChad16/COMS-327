all: game
		
game: game.cpp heap.cpp
	g++ -g3 game.cpp heap.cpp -lncurses -o game
	
clean: 
	rm game