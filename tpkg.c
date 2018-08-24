/* 
##################################################################
############################ TODO ################################
	- use standard C functions to do directory/file manipulation #
##################################################################
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define DBPATH "/home/richard/Pro/LinuxStuff/tpkg/root/usr/bin/tpkg_DB"
#define USRBIN "/home/richard/Pro/LinuxStuff/tpkg/root/usr/bin"
#define URL "127.0.0.1:8080"

struct {
	int ins, rm;
}opt;

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
			sprintf(cmd, "mkdir %s/%s &> /dev/null", DBPATH, pkgList[i]);
			int stat = system(cmd);
			if(WEXITSTATUS(stat) == 1) {
				printf("skipping \"%s\" ...\n", pkgList[i]);
				continue;
			}
			printf("Downloading Package: %s\n", pkgList[i]);
			sprintf(cmd, "wget -P %s %s/%s &> /dev/null", USRBIN, URL, pkgList[i]);
			stat = system(cmd);
			if(WEXITSTATUS(stat) == wget_OK) {
				printf("  Done\n");
			} else if (WEXITSTATUS(stat) == wget_server_err) {
				printf("wget server side error. Does \"%s\" exist?\n", pkgList[i]);
				goto cleanup;
			} else {
				printf("\nwget unknown error, aborting...\n");
				goto cleanup;
			}
			continue;	// good exit

			cleanup:	// package failed, remove directory
			sprintf(cmd, "rmdir %s/%s", DBPATH, pkgList[i]);
			system(cmd);
		}
	}

	if(opt.rm) {
		for(int i=0; i<listTop; i++) {
			int stat = 0;
			char cmd[256];
			sprintf(cmd, "rmdir %s/%s &> /dev/null", DBPATH, pkgList[i]);
			stat = WEXITSTATUS(system(cmd));
			if(stat == 1) {
				fprintf(stderr, "package \"%s\" is not installed...\n", pkgList[i]);
				continue;
			}
			sprintf(cmd, "rm %s/%s &> /dev/null", USRBIN, pkgList[i]);
			system(cmd);
		}
	}
	return 0;
}
