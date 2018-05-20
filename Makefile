build: towerfs.exe

towerfs.exe: towerfs.cpp
	g++ towerfs.cpp -O2 -o towerfs.exe `pkg-config fuse --cflags --libs`

run: towerfs.exe
	( mkdir tower 2>/dev/null ) ; ./towerfs.exe tower

stop:
	fusermount -u tower

