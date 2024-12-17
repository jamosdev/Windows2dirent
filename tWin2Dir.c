#include <jirent.h>
#include <stdio.h>

int main(int argc,char**argv){
    DIR *d;
    struct dirent * e;
    int i=0;
    const char * os = getenv("OS") ? getenv("OS") : "an Uknown operating system";
    const char * pwsh = getenv("POWERSHELL_DISTRIBUTION_CHANNEL") && strlen(getenv("POWERSHELL_DISTRIBUTION_CHANNEL"))>4? getenv("POWERSHELL_DISTRIBUTION_CHANNEL")+4 : "linux+wine???";
    const char * cores = getenv("NUMBER_OF_PROCESSORS") ? getenv("NUMBER_OF_PROCESSORS") : "???";
    const char * proc = getenv("PROCESSOR_IDENTIFIER") ? getenv("PROCESSOR_IDENTIFIER") : "unknown CPU";
    printf("Coming at you from %s (%s) with\na %s-core %s:\n",os,pwsh,cores,proc);
    d=opendir("/arj");
    e=readdir(d);
    while(e!= NULL) {
        ++i;
        printf("%3d) (%s ($%08x) %s\n",i,e->d_type==DT_DIR?"DIR) ":"FILE)",e->d_fileno,e->d_name);
        e=readdir(d);
    }
    return 0;
}

