all: hello
	
sobel: sobel.c
	gcc -Wall -Werror -g3 sobel.c -o sobel
	
clean: 
	rm sobel
	
