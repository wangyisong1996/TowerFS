# TowerFS
Yet another filesystem-based adventure game inspired by magic tower.

## Dependencies

This project is based on FUSE and C++.

```bash
$ sudo apt install libfuse-dev
$ sudo apt install g++
```

## Building and Running

To build and run the game:

```bash
$ make build
$ make run
```

Then the game will be mounted on `tower` directory.

To play the game:

```bash
$ cd tower
# Do what the fun you want to do
```

To stop the running game, please make sure that no one has a working diretory inside the game, and then:

```bash
$ make stop
```


