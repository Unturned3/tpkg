#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ROOT "/path/to/root"
#define URL "https://raw.githubusercontent.com/Unturned3/unturned3.github.io/master/repo"

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
	if(argc == 1) {
		disp_usage();
		return 0;	// exit if no options provided
	}

	// is tpkg running as root?
	if(getuid() != 0) {
		fprintf(stderr, "tpkg must run as root!\n");
		return 0;
	}

	// change the working directory to ROOT
	if(chdir(ROOT) == -1) {
		fprintf(stderr, "cannot set working directory. Does ROOT exist?\n");
		return 0;
	}

	// create usr/bin/tpkg_DB if it doesn't exist
	if(access("usr/bin/tpkg_DB", F_OK) != 0) {
		fprintf(stderr, "warning: /ROOT/usr/bin/tpkg_DB does not exist! Creating a new one...\n");
		system("mkdir -p usr/bin/tpkg_DB");
		system("chmod 755 usr/bin/tpkg_DB");
	}

	// use getopt() to parse command line options
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
	
	// read the list of package names provided
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
	
	// install the requested packages
	if(opt.ins) {
		for(int i=0; i<listTop; i++) {
			char cmd[256];
			sprintf(cmd, "mkdir usr/bin/tpkg_DB/%s &> /dev/null", pkgList[i]);
			int stat = system(cmd);

			// skip to next package if current package is already installed
			if(WEXITSTATUS(stat) == 1) {
				printf("skipping \"%s\" ...\n", pkgList[i]);
				continue;
			}

			printf("Downloading Package: %s\n", pkgList[i]);
			sprintf(cmd, "wget -P usr/bin %s/%s &> /dev/null", URL, pkgList[i]);
			stat = system(cmd);
			if(WEXITSTATUS(stat) == wget_OK) {
				// modify file permission
				sprintf(cmd, "chmod 755 usr/bin/%s", pkgList[i]);
				system(cmd);
				printf("  Done\n");
			} else if (WEXITSTATUS(stat) == wget_server_err) {
				// server returned error, such as 404, etc.
				printf("wget server side error. Does \"%s\" exist?\n", pkgList[i]);
				goto cleanup;
			} else {
				printf("\nwget unknown error, aborting...\n");
				goto cleanup;
			}
			continue;	// normal exit

			cleanup:	// fetching failed, remove the corresponding tpkg_DB entry
			sprintf(cmd, "rmdir usr/bin/tpkg_DB/%s", pkgList[i]);
			system(cmd);
		}
	}

	// remove the requested packages
	if(opt.rm) {
		for(int i=0; i<listTop; i++) {
			int stat = 0;
			char cmd[256];
			sprintf(cmd, "rmdir usr/bin/tpkg_DB/%s &> /dev/null", pkgList[i]);
			stat = WEXITSTATUS(system(cmd));
			if(stat == 1) {
				fprintf(stderr, "package \"%s\" is not installed...\n", pkgList[i]);
				continue;
			}
			sprintf(cmd, "rm usr/bin/%s &> /dev/null", pkgList[i]);
			system(cmd);
		}
	}
	return 0;
}
