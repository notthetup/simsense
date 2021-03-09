
// Usage : sensorsim -n /dev/probe -d 1 -p scr -d manta.txt

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_LENGTH 128

int ptym_open(char *pts_name_s , int pts_namesz) {
    char    *ptr;
    int     fdm;

    fdm = posix_openpt(O_RDWR | O_NONBLOCK);
    if (fdm < 0) return(-1);
    if (grantpt(fdm) < 0)  {
        close(fdm);
        return(-2);
    }
    if (unlockpt(fdm) < 0)  {
        close(fdm);
        return(-3);
    }
    if ((ptr = ptsname(fdm)) == NULL)  {
        close(fdm);
        return(-4);
    }
    
    strncpy(pts_name_s, ptr, pts_namesz);
    pts_name_s[pts_namesz - 1] = '\0';

    return(fdm);  
}

int conf_ser(int serialDev) {

    int rc;
    struct termios params;

    // Get terminal atributes
    rc = tcgetattr(serialDev, &params);

    // Modify terminal attributes
    cfmakeraw(&params);

    rc = cfsetispeed(&params, B9600);

    rc = cfsetospeed(&params, B9600);

    // CREAD - Enable port to read data
    // CLOCAL - Ignore modem control lines
    params.c_cflag |= (B9600 |CS8 | CLOCAL | CREAD | INLCR | OCRNL);

    // Make Read Blocking
    //fcntl(serialDev, F_SETFL, 0);

    // Set serial attributes
    rc = tcsetattr(serialDev, TCSANOW, &params);

    tcflush(serialDev, TCIOFLUSH);
  
    return EXIT_SUCCESS;
}

int main (int argc, char** argv){
    int delays = 1;
    int ptyfd; 
    FILE *datafp;
    char * devName = "/dev/gps";
    char * dataName = "nmea.txt";
    char * prompt = "scr";
    char * rv;
    char tmp[MAX_LENGTH] = {0};
    char buf[MAX_LENGTH] = {0};
    char ptyname[128] = {0};

    printf("hello world\n");

    ptyfd=ptym_open(ptyname,128);
    // if (symlink(ptyname, devName) < 0) {
    //   fprintf(stderr, "Cannot create: %s . Using %s instead.\n", devName, ptyname);
    // } else{
    //   printf("Created simulated sensor at %s", ptyname);
    // }
    printf("Created simulated sensor at %s\n", ptyname);
    conf_ser(ptyfd);

    datafp = fopen(dataName , "r");

    bool promptFound = false;
    int bptr = 0;
    while (promptFound != true ){
        int ret = read(ptyfd, tmp, MAX_LENGTH);
        if (ret > 0) {
            if (bptr >= MAX_LENGTH-1){
                bptr = 0;
                memset(buf,0,MAX_LENGTH);
            }
            for (int i=0; i < ret; i++) buf[bptr++] = tmp[i];
            if (strstr(buf, prompt) != NULL){
                promptFound = true;
                break;
            }
            char * pch=strrchr(buf,'\r');
            if (pch != NULL){
                for (int i = 0; i < MAX_LENGTH; i++){
                    if (pch-buf+i < MAX_LENGTH) {
                        buf[i] = buf[pch-buf+i];
                    }else{
                        buf[i] = 0;
                    }
                }
                bptr = 0;
            }
       }
       usleep(100000);
    }
    
    rv = fgets (tmp, MAX_LENGTH, datafp);
    tmp[MAX_LENGTH-1] = '\0';
    while (rv != NULL){
        write(ptyfd, tmp, strlen(tmp));
        sleep(delays);
        rv = fgets (tmp, MAX_LENGTH, datafp);
        tmp[MAX_LENGTH-1] = '\0';
    }

    close(ptyfd);

    return EXIT_SUCCESS;
}