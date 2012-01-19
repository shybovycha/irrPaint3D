all: irrPaint3d.cpp
#    g++ irrPaint3d.cpp -g -lIrrlicht -I/usr/include/irrlicht/ -o irrPaint3d.elf
    cl.exe irrPaint3d.cpp -ID:/Programming/Gamedev/irrlicht-1.7.2/include
	
clean:
	rm irrPaint3d.elf
