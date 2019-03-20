Sobel.c is designed to apply a sobel filter to a .pgm input and output the new file. 

The read_pgm function reads in the file and outputs the pixel info into an X by X range. In 
this case it was 1024x1024.

The main function applies the sobel filter to the X and then the Y seperately. 
Combined to create the new overall image.

The write_pgm function then takes the out array created after applying the sobel filter
to a new .pgm image.