#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define DBPATH "/home/richard/Pro/LinuxStuff/tpkg/root/usr/bin/tpkg_DB/"
#define USRBIN "/home/richard/Pro/LinuxStuff/tpkg/root/usr/bin/"
#define URL "127.0.0.1:8080/"

struct {
	int ins, rm;
}opt;

enum touchErr {
	mkdir_success,
	mkdir_fail
};

enum wgetErr {
	wget_OK, wget_generic_err, wget_parse_err,
	wget_file_err, wget_net_err, wget_ssl_err,
	wget_auth_err, wget_proto_err, wget_server_err
};

void disp_usage() { printf("error: incorrect/missing arguments\n"); }
const char* optString = "ir";

int listTop = 0;
char* pkgList[32];

int main(int argc, char ** argv)
{
	if(argc == 1)
		return 0;	// exit if no options provided

	char c;
	while ((c = getopt(argc, argv, optString)) != -1)
		switch (c) {
			case 'i':
				opt.ins = 1; break;
			case 'r':
				opt.rm = 1; break;
			case '?':
				disp_usage(); return 0;
			default: break;
		}

	if(opt.ins && opt.rm) {
		fprintf(stderr, "error: cannot install and remove packages at the same time\n");
		return 0;
	}
	
	if(opt.ins || opt.rm) {
		for(int i=optind; i<argc; i++) {
			pkgList[listTop] = argv[i];
			listTop++;
		}
		if(listTop == 0) {
			disp_usage();
			return 0;
		}
	}

	if(opt.ins) {
		for(int i=0; i<listTop; i++) {
			// skip if package exists
			char cmd[256] = "mkdir ";
			strcat(cmd, DBPATH);
			strcat(cmd, pkgList[i]);
			strcat(cmd, " &> /dev/null");
			int stat = system(cmd);
			if(WEXITSTATUS(stat) == 1) {
				printf("skipping \"%s\" ...\n", pkgList[i]);
				continue;
			}
			printf("Downloading Package: %s\n", pkgList[i]);
			strcpy(cmd, "wget -P ");
			strcat(cmd, USRBIN);
			strcat(cmd, " ");
			strcat(cmd, URL);
			strcat(cmd, pkgList[i]);
			strcat(cmd, " > /dev/null");
			stat = system(cmd);
			if(WEXITSTATUS(stat) == wget_OK) {
				printf("  Done\n");
			} else if (WEXITSTATUS(stat) == wget_server_err) {
				printf("  wget server side error. Does the package exist?\n");
				printf("\npackage downloading failed, aborting...\n");
				goto cleanup;
			} else {
				printf("\nwget unknown error, aborting...\n");
				goto cleanup;
			}
			continue;	// good exit

			cleanup:	// package failed, remove directory
			strcpy(cmd, "rmdir ");
			strcat(cmd, DBPATH);
			strcat(cmd, pkgList[i]);
			system(cmd);
		}
	}

	if(opt.rm) {
		for(int i=0; i<listTop; i++) {

		}
	}
	return 0;
}
