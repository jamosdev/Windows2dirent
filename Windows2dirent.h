/**
@file Windows2dirent.h
@brief brief dirent.h implementation using Windows.h FindFirstFileA() & FindNextFileA() supports seekdir()

we uploaded it to github only because it was better than what you can find elsewhere on the internet
*/
#ifndef Windows2dirent_h
#define Windows2dirent_h 1
#ifdef __cplusplus
extern "C"
{
#endif//__cplusplus
#ifdef _WIN32
#include <ctype.h>//tolower
#include <stdlib.h>//getenv
#include <limits.h>//NAME_MAX,PATH_MAX
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#ifndef WIN32_LEAND_AND_MEAN
#define WIN32_LEAND_AND_MEAN 1
#endif
#ifndef NOSERVICE
#define NOSERVICE 1
#endif
#ifndef NOMCX
#define NOMCX 1
#endif
#ifndef NOIME
#define NOIME 1
#endif
#include <Windows.h>//base functions...
#include <string.h>//memcpy,strlen
#ifndef DT_REG
#define DT_UNKNOWN 0 /**< The type is unknown. Only some filesystems have full support to return the type of the file, others might always return this value. */
#define DT_REG     8 /**< A regular file. */
#define DT_DIR     4 /**< A directory. */
#define DT_FIFO    1 /**< A named pipe, or FIFO. See FIFO Special Files. */
#define DT_SOCK    12/**< A local-domain socket. */
#define DT_CHR     2 /**< A character device. */
#define DT_BLK     6 /**< A block device. */
#define DT_LNK     10/**< A symbolic link.  */
#define DT_WHT     14/**< BSD Union-mount "whiteout" */
#endif//DT_REG
#ifndef d_fileno
/** compatibility BSD + Linux */
#define d_fileno d_ino
#endif
typedef unsigned ino_t;
struct dirent {
    ino_t d_ino;/**< file serial number, windows DOES have a FileID but you have to open each file to get a handle to get the id, screw that! we will just use hashes. */
    unsigned int d_type; /**< only DT_REG, DT_DIR, and DT_LNK are implemented. */
    unsigned char d_namlen;  /**< length of the filename, not including the null. "unsigned char" because that is the appropriate size they said */
    unsigned short d_reclen;    /**< length of this record */
    char d_name[NAME_MAX+1]; /**< at the end because other systems put it at the end to sneaky only allocate as needed for name length and expand as needed (d_reclen is current). well screw all of that. */
};
struct dir_tag {
    HANDLE h;
    WIN32_FIND_DATAA d;
    struct dirent storage;
    unsigned hash_base;
    unsigned counter;
    unsigned errors;
    char reopenbuf[PATH_MAX+1];
};
#ifndef _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_NAMLEN 1
#endif
typedef struct dir_tag DIR;
/*--https://en.wikibooks.org/wiki/C_Programming/POSIX_Reference/dirent.h--*/
static DIR* opendir(const char* dirname) {
    char workbuf[PATH_MAX+1];
    size_t dnlen = strlen(dirname);
    size_t i;
    size_t offset;
    DIR *ret;
    if(!dirname) return NULL;
    if(dirname[0] == '/') {
        const char * SystemDrive = getenv("SystemDrive");
        const char * HOMEDRIVE = getenv("HOMEDRIVE");
        const char * default = "C:";
        const char * chosen = SystemDrive!= NULL ? SystemDrive : HOMEDRIVE != NULL ? HOMEDRIVE : default;
        offset = strlen(chosen);
        memcpy(workbuf,chosen,offset);
    } else offset=0;
    for(i=0;i<strlen(dirname) && i+offset<PATH_MAX;++i) {
        workbuf[i+offset]=dirname[i] == '/' ? '\\' : dirname[i];
    }
    i+=offset;
    if(i+3<PATH_MAX) {
        memcpy(workbuf+i,"\\*",3);//caps it with a NULL terminator
    } else {
        return NULL;//if we go up one dir (if any) and asterix it, then their recursive directory lister will infinite loop, so just return NULL here
    }
    ret = (DIR*)malloc(sizeof(DIR));
    if(ret==NULL) return NULL;
    ret->h = FindFirstFileA((LPCSTR)workbuf, & ret->d);
    if(ret->h == INVALID_HANDLE_VALUE) {
        free(ret);
        return NULL;
    }
    ret->storage.d_reclen = sizeof(struct dirent);
    /*--before we return, lowercase workbuf and hash it for hash_base for consistent but fake ino for performance!!!--*/
    ret->hash_base=5381;//djb2 algorithm
    for(i=0;workbuf[i]!='\0';++i){
        unsigned input = (unsigned char)tolower(workbuf[i]);
        ret->hash_base=((ret->hash_base << 5)+ret->hash_base)+input;
    }
    ret->hash_base &= 0xFFFFE000UL;
    ret->counter = 1;
    ret->errors = 0;
    memcpy(ret->reopenbuf,workbuf,strlen(workbuf));
    return ret;
}
static struct dirent* readdir(DIR* dirp){
    size_t fnlen;
    struct dirent* ret;
    const char *fn;
    WIN32_FIND_DATA *fd;

    if(dirp == NULL) return NULL;
    if(dirp->errors) return NULL;
    ret = & dirp->storage;
    fd = & dirp->d;

    /*--is it a directory (or symbolic link?)--*/
    if(fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ret->d_type = DT_DIR;
    #ifdef FILE_ATTRIBUTE_REPARSE_POINT
    else if(fd->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ret->d_type = DT_LNK;
    #endif
    else fd->d_type = DT_REG;

    /*--filename funcs--*/
    fn = & fd->cFileName[0];
    fnlen = strlen(fn);
    ret->d_namelen = fnlen>254?254:(unsigned char)(fnlen&0xff);
    memcpy(ret->d_name,fd->cFileName,ret->d_namelen);
    ret->d_name[ret->d_namelen]='\0';

    /*--inode faker--*/
    ret->d_ino = dirp->hash_base | dirp->counter;

    /*--perform next iteration--*/
    if(FindNextFileA(dirp->h, fd) != 0) dirp->counter++;
    else dirp->errors = 1;

    return ret;
}
static void rewinddir(DIR* dirp){
    if(dirp==NULL) return;
    FindClose(dirp->h);
    dirp->counter = 1;
    dirp->errors = 0;
    ret->h = FindFirstFileA(dirp->reopenbuf, & dirp->d);
    //if(ret->h == INVALID_HANDLE_VALUE)
}
static void seekdir(DIR* dirp, long int loc){
    if(loc > dirp->counter) {
        /*--good/easy, but not usually the case as telldir not yet--*/
        while(loc > dirp->counter) {
            struct dirent* null_on_errors = readdir(dirp);
            if(null_on_errors == NULL) break;
        }
    } else {
        rewinddir(dirp);
        seekdir(dirp,loc);
    }
}
static long int telldir(DIR* dirp){
    if(dirp == NULL) return 0;
    dirp->last_telldir_pos = dirp->counter;//for a later seekdir
    return dirp->counter;
}
static int closedir(DIR*dirp){
    if(dirp==NULL)return -1;
    FindClose(dirp->h);
    free(dirp);
    return 0;
}
#else
#include <dirent.h>
#endif
#ifdef __cplusplus
}
#endif//__cplusplus
#endif//Windows2dirent_h
