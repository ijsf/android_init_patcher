#ifndef __PATCH_H__
#define __PATCH_H__

#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#ifdef ENVIRONMENT64
#define ADDR_LEN "16"
#else
#define ADDR_LEN "8"
#endif

#define WORD sizeof(long)
#define BUFLEN 4096
#define MAX_RECORDS 4096 
#define DUMP_DIR "%s/"
#define ADDR_FMT 
#define MAPI_FMT "%0"ADDR_LEN"lx-%0"ADDR_LEN"lx %c%c%c%c %0"ADDR_LEN"llx %02x:%02x %lu %255[^\n]s"
#define MAPO_FMT "%0"ADDR_LEN"lx-%0"ADDR_LEN"lx %c%c%c%c %0"ADDR_LEN"llx %02x:%02x %lu %s\n"
#define DUMP_FMT DUMP_DIR "%0"ADDR_LEN"lx:%0"ADDR_LEN"lx:%c%c%c%c:%lld:%s"

typedef struct __procmap_record {
    long begin;
    long end;
    unsigned char read;
    unsigned char write;
    unsigned char exec;
    unsigned char shared;
    long long offset;
    unsigned int dev_major;
    unsigned int dev_minor;
    unsigned long inode;
    char info[256];
} procmap_record;

typedef struct __procmap {
    procmap_record records[MAX_RECORDS];
    size_t count;
    pid_t pid;
} procmap;

int procmap_init(procmap *map, pid_t pid);
int read_maps(procmap *map, FILE *ifile);
int patch_memory_strings(pid_t pid, const void *addr_start, const size_t len, const char *match, const char *replace);

#endif

