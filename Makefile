all: irrPaint3d.cpp
	g++ irrPaint3d.cpp -g -lIrrlicht -I/usr/include/irrlicht/ -o irrPaint3d.elf
	
clean:
	rm irrPaint3d.elf
