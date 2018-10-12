#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>	// DIR struct

#define ROOT "/home/richard/root"	// fake root directory to operate on (for safety)
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
const char* optString = "ir";	// accepted options for tpkg

int listTop = 0;
char pkgList[32][128];

int installed(char *name)	// check if a package is installed already
{
	char cmd[256];
	sprintf(cmd, "usr/bin/tpkg_DB/%s", name);
	if(access(cmd, F_OK) == 0)
		return 1;	// file exists
	return 0;
}

int chdb(int action, char *name)
{
	char cmd[256];
	if(action == 1)	// creates an entry in tpkg_DB
		sprintf(cmd, "mkdir usr/bin/tpkg_DB/%s &> /dev/null", name);
	if(action == -1) {	// removes an entry in tpkg_DB
		sprintf(cmd, "usr/bin/tpkg_DB/%s", name);
		int n = 0;
		DIR *d = opendir(cmd);
		if(d == NULL)
			return 1;	// directory does not exist (pkg not installed)
		while(readdir(d) != NULL) {
			n++;	// count how many files are present inside directory
			if(n > 2)
				break;
		}
		closedir(d);
		if(n <= 2)	// directory is empty
			sprintf(cmd, "rmdir usr/bin/tpkg_DB/%s &> /dev/null", name);
		else
			return 2;	// directory is NOT empty (pkg required by others)
	}
	int s = WEXITSTATUS(system(cmd));	// rmdir & mkdir returns 1 if failed
	return s;
}

int install(char name[128], char rqb[128])
{
	// printf("Installing package \"%s\"\n", name);
	if(chdb(1, name) == 1) {
		printf("\"%s\" is already installed\n", name);
		return 0;
	}
	
	char cmd[256];
	int stat1 = 0, stat2 = 0;

	// download the pkg and its .dep file
	sprintf(cmd, "wget -c -P usr/bin %s/%s", URL, name);
	stat1 = WEXITSTATUS(system(cmd));
	sprintf(cmd, "wget -c -P usr/bin/tpkg_DB %s/%s.dep", URL, name);
	stat2 = WEXITSTATUS(system(cmd));
	if(stat1 == wget_OK && stat2 == wget_OK) {
		// modify file permission
		sprintf(cmd, "chmod 755 usr/bin/%s", name);
		system(cmd);
	} else if (stat1 == wget_server_err || stat2 == wget_server_err) {
		// server returned error, such as 404, etc.
		printf("wget error. Does \"%s\" exist?\n", name);
		goto PKG_FAILED;
	} else {
		printf("\nwget unknown error, aborting...\n");
		goto PKG_FAILED;
	}

	// add package "rqb" into current package's tpkg_DB directory
	// meaning: package "rqb" requires current package as a dependency
	// "null": nothing depends on the current packge, so skip this
	if(strcmp(rqb, "null") != 0) {
		sprintf(cmd, "touch usr/bin/tpkg_DB/%s/%s", name, rqb);
		system(cmd);
	}

	printf("Resolving dependencies...\n");

	char depaddr[256];
	int depSize = 0;
	char depList[32][128];

	// read dependencies from .dep file
	sprintf(depaddr, "usr/bin/tpkg_DB/%s.dep", name);
	FILE *depfp = fopen(depaddr, "r");
	while(fscanf(depfp, "%128s", depList[depSize]) != EOF)
		depSize++;
	fclose(depfp);
	
	// download dependencies
	for(int i=0; i<depSize; i++) {
		if(installed(depList[i]))
			continue;
		if(install(depList[i], name) == -1) {
			fprintf(stderr, "Error fetching dependency \"%s\"\n", depList[i]);
			goto PKG_FAILED;
		}
	}
	return 0;	// normal exit point

PKG_FAILED:
	sprintf(cmd, "rm -r usr/bin/%s &> /dev/null", name);
	system(cmd);	// remove pkg in usr/bin
	chdb(-1, name);	// remove record in tpkg_DB
	chdb(-1, strcat(name, ".dep"));	// remove its corresponding .dep file
	return -1; 
}

int main(int argc, char** argv)
{
	if(argc == 1) {
		disp_usage();
		return 1;	// exit if no options provided
	}

	// is tpkg running as root?
	if(getuid() != 0) {
		fprintf(stderr, "tpkg must run as root!\n");
		return 1;
	}

	// change the working directory to ROOT
	if(chdir(ROOT) == -1) {
		fprintf(stderr, "cannot set working directory. Does ROOT exist?\n");
		return 1;
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
			int status = install(pkgList[i], "null");	// DFS-install pkg
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
			char cmd[256] = "usr/bin/tpkg_DB/";
			stat = chdb(-1, pkgList[i]);	// remove record in tpkg_DB
			if(stat == 1) {
				fprintf(stderr, "package \"%s\" is not installed...\n", pkgList[i]);
				continue;
			}
			if(stat == 2) {
				fprintf(stderr, "cannot remove package \"%s\". Required by: ", pkgList[i]);
				sprintf(cmd, "ls usr/bin/tpkg_DB/%s", pkgList[i]);
				system(cmd);
				continue;
			}
			sprintf(cmd, "rm -r usr/bin/%s &> /dev/null", pkgList[i]);
			system(cmd);	// remove pkg installed in usr/bin
			// remove its dependency requirement from other packages
			chdb(-1, strcat(pkgList[i], ".dep"));	// remove .dep file in tpkg_DB
		}
	}
	return 0;
}
