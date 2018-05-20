#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

static int do_getattr(const char *path, struct stat *st) {
	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_atime = time(NULL);
	st->st_mtime = time(NULL);
	
	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0555;
	}
	
	if (strcmp(path, "/test") == 0) {
		st->st_mode = S_IFREG | 0555;
		st->st_size = 15;
	}
	
	return 0;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);
	
	//filler(buffer, "test", NULL, 0);
	
	return 0;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	static const char *content = "abcdefghijklmn\n";
	int len = strlen(content);
	if (offset < 0 || offset >= size) return 0;
	if (len - offset < size) {
		size = len - offset;
	}
	memcpy(buffer, content, size);
	return size;
}

static struct fuse_operations operations;

int main(int argc, char **argv) {
	operations.getattr = do_getattr;
	operations.readdir = do_readdir;
	operations.read = do_read;
	return fuse_main(argc, argv, &operations, NULL);
}
