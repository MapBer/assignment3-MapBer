
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
	if (argc != 3) {
		syslog(LOG_ERR, "Usage: %s with incorrect parameters\n", argv[0]);
		return 1;
	}
    
    
	FILE *fp = fopen(argv[1], "w");
	if (fp == NULL) {
        syslog(LOG_ERR, "%s: File does not exist\n", argv[1]);
        perror("Error opening file");
		return 1;
	}
    
    openlog("writer", LOG_PID|LOG_CONS, LOG_USER);
    syslog(LOG_DEBUG, "Writing %s to %s", argv[1], argv[2]);

    fprintf(fp, "%s\n", argv[2]);
	fclose(fp);
	return 0;
}
