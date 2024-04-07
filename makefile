all: main libcore

main: ./src/main.c
	cc -Wall -Wextra -ggdb -L./raylib-5.0_linux_amd64/lib -o ./bin/main.out ./src/main.c -lraylib

run: ./bin/main.out
	LD_PRELOAD=./raylib-5.0_linux_amd64/lib/libraylib.so ./bin/main.out

libcore: ./src/libcore.c
	cc -Wextra -ggdb -fPIC -shared -L./raylib-5.0_linux_amd64/lib -o ./bin/libcore.so ./src/libcore.c
