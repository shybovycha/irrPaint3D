all: irrPaint3d.cpp
	g++ irrPaint3d.cpp -g -lIrrlicht -I/usr/include/irrlicht/ -o irrPaint3d

clean:
	rm irrPaint3d
