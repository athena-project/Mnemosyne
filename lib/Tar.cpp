#include "Tar.h"


Tar::Tar(){}

bool Tar::untar(char* source, char* location){
	TAR * tar_file;

	if (tar_open(&tar_file, source, NULL, O_RDONLY, 0, TAR_GNU) == -1) {
		fprintf(stderr, "tar_open(): %s\n", strerror(errno));
		throw"";
		return false;
	}

	if (tar_extract_all(tar_file, location) != 0) {
		fprintf(stderr, "tar_extract_all(): %s\n", strerror(errno));
		throw"";
		return false;
	}

	if (tar_close(tar_file) != 0) {
		fprintf(stderr, "tar_close(): %s\n", strerror(errno));
		throw"";
		return false;
	}

	return true;
}

bool untar_file( char* source, char* name, char* location){
	TAR * tar_file;

	if (tar_open(&tar_file, source, NULL, O_RDONLY, 0, TAR_GNU) == -1) {
		fprintf(stderr, "tar_open(): %s\n", strerror(errno));
		throw"";
		return false;
	}

	if (tar_extract_file(tar_file, name) != 0) {
		fprintf(stderr, "tar_extract_all(): %s\n", strerror(errno));
		throw"";
		return false;
	}

	if (tar_close(tar_file) != 0) {
		fprintf(stderr, "tar_close(): %s\n", strerror(errno));
		throw"";
		return false;
	}


	fs::path pathSource = fs::path( string(source)+"/"+string(name) );
	fs::path pathDest = fs::path( location );

	fs::copy_file( pathSource, pathDest );
	remove( (string(source)+"/"+string(name)).c_str() );

	return true;
}

bool Tar::tar_dir(char* location, char* path, char* name){
	TAR * tar_file;

	if (tar_open(&tar_file, location, NULL, O_WRONLY | O_CREAT, 0777, TAR_GNU) == -1) {
		fprintf(stderr, "tar_open(): %s\n", strerror(errno));
		throw"";
		return false;
	}

	if (tar_append_tree(tar_file, path, name) == -1) {
		fprintf(stderr, "tar_append_file(): %s\n", strerror(errno));
		throw"";
		return false;
	}

	if (tar_close(tar_file) != 0) {
		fprintf(stderr, "tar_close(): %s\n", strerror(errno));
		throw"";
		return false;
	}

	return true;
}

