The raytracer is completed in c++ and outputs a ppm file using the save_imageP6.
The files outputted are visually similar if not exact to the testing ones.
Also ensures that there only up to 15 spheers and 10 lights as the instructions said only up to 14 more or 9 more. As well checks ths length of output is <= 20 characters long.


It won't work unless you add the ppm.cpp because I make a call "ppm::save_imageP6(Width, Height, parameters.output.c_str(), pixels);" and it needs
ppm for it work.

To compile:
	g++ raytracer.cpp ppm.cpp -o raytracer.exe

To run:
	./raytracer.exe [filename.txt]

