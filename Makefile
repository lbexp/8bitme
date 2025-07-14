compile:
	gcc main.c -o build/main.o -c
	gcc file.c -o build/file.o -c
	gcc png.c -o build/png.o -c
link:
	gcc -o build/8bitme build/main.o build/file.o build/png.o -lz
run:
	Make compile && Make link && ./build/8bitme
