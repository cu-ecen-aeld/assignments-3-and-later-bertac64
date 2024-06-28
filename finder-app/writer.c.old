/**
* writer.c - software for assignment 2
* Author: Claudio Bertacchini
*/

/* POSIX.1 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>

int main(int argc, char *argv[]){
    int fd;

    openlog("writer", LOG_PID| LOG_CONS, LOG_USER);

    if(argc != 3){
        syslog(LOG_ERR, "Error: arguments number should be 2!");
        syslog(LOG_INFO,"Usage: %s <writefile> <writestr>",argv[0]);
        closelog();
        return 1;
    }

    char *writefile = argv[1];
    char *writestr = argv[2];
    
    // Make a copy of writefile to use with dirname
    char dir_copy[1024];
    strncpy(dir_copy, writefile, 1024);
    dir_copy[1023] = '\0';
    
    char *dir = dirname(dir_copy);

    struct stat statbuf;
    if(stat(dir, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)){
        syslog(LOG_ERR, "Error: %s is not a directory or cannot be accessed!", dir);
 //       printf("Error: %s is not a directory or cannot be accessed!\n", dir);
        closelog();
        return 1;
    }

    fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd == -1){
        syslog(LOG_ERR, "Error: cannot create file! %s", strerror(errno));
//        printf("Error: cannot create file! %s\n", strerror(errno));
        closelog();
        return 1;
    }

    ssize_t bw = write(fd, writestr, strlen(writestr));
    if (bw == -1){
        syslog(LOG_ERR, "Error: cannot write into the file! %s", strerror(errno));
//        printf("Error: cannot write into the file! %s\n", strerror(errno));
        close(fd);
        closelog();
        return 1;
    }

    if (close(fd) == -1){
        syslog(LOG_ERR, "Error closing file! %s", strerror(errno));
        closelog();
        return -1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    printf("Writing %s to %s\n", writestr, writefile);

    fd = open(writefile, O_RDONLY);
    if(fd == -1){
        syslog(LOG_ERR, "Error: cannot read file! %s", strerror(errno));
        printf("Error: cannot read file! %s\n", strerror(errno));
        closelog();
        return 1;
    }

    size_t len = strlen(writestr);
    char buf[1024];
    //zero out the buffer
    memset(buf,0,sizeof(buf));
    
    ssize_t rb = read(fd, buf, len);
    if(rb == -1){
        syslog(LOG_ERR, "Error reading data! %s", strerror(errno));
        printf("Error reading data! %s\n", strerror(errno));
        close(fd);
        closelog();
        return 1;
    }
    //Null-terminate string
    buf[rb]='\0';

    if(strcmp(buf, writestr) != 0){
        syslog(LOG_ERR, "Error: content does not match with %s", writestr);
        printf("Error: content does not match with %s\n", writestr);
        closelog();
        close(fd);
        return 1;
    }

    close(fd);
    closelog();
    return 0;
}

