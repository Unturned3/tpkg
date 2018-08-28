#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ROOT "/home/richard/root"
#define URL "https://raw.githubusercontent.com/Unturned3/unturned3.github.io/master/repo"
// #define URL "127.0.0.1:8080"

struct {
	int ins, rm;
}opt;

enum wgetErr {
	wget_OK, wget_generic_err, wget_parse_err,
	wget_file_err, wget_net_err, wget_ssl_err,
	wget_auth_err, wget_proto_err, wget_server_err
};

// set the permission for all directoreis to 755
mode_t dirPerm = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

void disp_usage() { printf("error: incorrect/missing arguments\n"); }
const char* optString = "ir";

int listTop = 0;
char pkgList[32][128];

int installed(char *name)
{
	char cmd[256];
	sprintf(cmd, "usr/bin/tpkg_DB/%s", name);
	if(access(cmd, F_OK) == 0)
		return 1;
	return 0;
}

int chdb(int action, char *name)
{
	char cmd[256];
	if(action == 1)
		sprintf(cmd, "mkdir usr/bin/tpkg_DB/%s &> /dev/null", name);
	if(action == -1)
		sprintf(cmd, "rm -r usr/bin/tpkg_DB/%s &> /dev/null", name);
	int s = WEXITSTATUS(system(cmd));
	return s;
}

int install(char name[128])
{
	printf("Installing package \"%s\"\n", name);
	if(chdb(1, name) == 1) {
		printf("  already installed\n", name);
		return 0;
	}
	
	char cmd[256];
	int stat1 = 0, stat2 = 0;

	// download the pkg and its .dep file
	sprintf(cmd, "wget -cq -P usr/bin %s/%s", URL, name);
	stat1 = WEXITSTATUS(system(cmd));
	sprintf(cmd, "wget -cq -P usr/bin/tpkg_DB %s/%s.dep", URL, name);
	stat2 = WEXITSTATUS(system(cmd));
	if(stat1 == wget_OK && stat2 == wget_OK) {
		// modify file permission
		sprintf(cmd, "chmod 755 usr/bin/%s", name);
		system(cmd);
	} else if (stat1 == wget_server_err || stat2 == wget_server_err) {
		// server returned error, such as 404, etc.
		printf("  wget server side error. Does \"%s\" exist?\n", name);
		goto PKG_FAILED;
	} else {
		printf("\n  wget unknown error, aborting...\n");
		goto PKG_FAILED;
	}

	printf("  Resolving dependencies...\n");

	char depaddr[256];
	int depSize = 0;
	char depList[32][128];

	// read dependencies from .dep file
	sprintf(depaddr, "usr/bin/tpkg_DB/%s.dep", name);
	FILE *depfp = fopen(depaddr, "r");
	while(fscanf(depfp, "%128s", depList[depSize]) != EOF)
		depSize++;
	
	// download dependencies
	for(int i=0; i<depSize; i++) {
		if(installed(depList[i]))
			continue;
		if(install(depList[i]) == -1) {
			fprintf(stderr, "  error fetching dependency \"%s\"\n", depList[i]);
			goto PKG_FAILED;
		}
	}
	return 0;	// normal exit point

PKG_FAILED:
	sprintf(cmd, "rm -r usr/bin/%s &> /dev/null", name);
	system(cmd);	// remove pkg in usr/bin
	chdb(-1, name);	// remove record in tpkg_DB
	chdb(-1, strcat(name, ".dep"));
	return -1; 
}

int main(int argc, char** argv)
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

	// reset umask for correct permissions
	// umask(0);

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
	
	// check for conflicting options
	if(opt.ins && opt.rm) {
		fprintf(stderr, "error: cannot install and remove packages at the same time\n");
		return 0;
	}
	
	// read the list of package names provided
	if(opt.ins || opt.rm) {
		for(int i=optind; i<argc; i++) {
			strcpy(pkgList[listTop], argv[i]);
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
			int status = install(pkgList[i]);	// DFS-install pkg
			if(status == -1) {
				printf("error occured. aborting...\n");
				return 0;
			}
		}
	}

	// remove the requested packages
	if(opt.rm) {
		for(int i=0; i<listTop; i++) {
			int stat = 0;
			char cmd[256];
			stat = chdb(-1, pkgList[i]);	// remove record in tpkg_DB
			if(stat == 1) {
				fprintf(stderr, "package \"%s\" is not installed...\n", pkgList[i]);
				continue;
			}
			sprintf(cmd, "rm -r usr/bin/%s &> /dev/null", pkgList[i]);
			system(cmd);	// remove pkg installed in usr/bin
			chdb(-1, strcat(pkgList[i], ".dep"));	// remove .dep file in tpkg_DB
		}
	}
	return 0;
}
