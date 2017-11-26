#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include "patch.h"
#include "main.h"

static void cleanup() {
  if (ptrace(PTRACE_DETACH, TARGET_PID, NULL, NULL)) {
    perror("PTRACE_DETACH");
    exit(errno);
  }
}

static int filter_maps(const procmap *imap, procmap *omap) {
  const char *blacklist[] = {"(deleted)", NULL};
  bool doit;
  size_t i, j;

  // Debug logging
#ifdef VERBOSE
#ifdef SCAN_DATA_SEGMENTS
    LOG("scanning data segments");
#endif
#ifdef SCAN_STACK_SEGMENTS
    LOG("scanning stack segments");
#endif
#ifdef SCAN_HEAP
    LOG("scanning heap");
#endif
#endif

  for (i=0; i<imap->count; ++i) {
    doit = false;
#ifdef SCAN_DATA_SEGMENTS
    doit |= (!strcmp(imap->records[i].info, imap->records[i].info));
#endif
#ifdef SCAN_STACK_SEGMENTS
    doit |= (strstr(imap->records[i].info, "[stack"));
#endif
#ifdef SCAN_HEAP
      doit |= strstr(imap->records[i].info, "[heap") || strstr(imap->records[i].info, "[anon:libc_malloc");
#endif

    j = 0;
    while (doit && blacklist[j] != NULL) {
      doit &= (strstr(imap->records[i].info, blacklist[j]) == NULL);
      j++;
    }

    if (doit) {
      memcpy(&omap->records[omap->count], &imap->records[i], 
      sizeof(procmap_record));
      omap->count++;
    }
  }
  return 0;
}

size_t patch_init(const procmap *map, const char *match, const char *replace) {
  size_t i;
  const procmap_record *it;
  size_t segment_size;
  size_t patches = 0;

  for (i=0; i<map->count; i++) {
    it = &map->records[i];

    if (it->read == 'r' && it->end > it->begin) {
      segment_size = it->end - it->begin;
      patches += patch_memory_strings(map->pid, (void *)it->begin, segment_size, match, replace);
    }
  }

  return patches;
}

int main(int argc, const char *argv[], char *envp[]) {
  procmap map, filtered_map;

  // Check match and replace sizes
  if(sizeof(PATCH_MATCH) != sizeof(PATCH_REPLACE)) {
    LOGE("invalid patch parameters");
  }

  if (ptrace(PTRACE_ATTACH, TARGET_PID, NULL, NULL)) {
    perror("PTRACE_ATTACH");
    exit(errno);
  }

  procmap_init(&map, TARGET_PID);
  procmap_init(&filtered_map, TARGET_PID);

  int patches = 0;
  if (read_maps(&map, NULL) == 0) {
    if (filter_maps(&map, &filtered_map) == 0) {
      patches = patch_init(&filtered_map, PATCH_MATCH, PATCH_REPLACE);
      if (patches > 0) {
        LOG("successful (%u patches)", patches);
      }
      else if (patches < 0) {
#ifdef VERBOSE
        LOGE("found matches, but failed to patch");
#endif
      }
      else {
#ifdef VERBOSE
        LOGE("failed to find any matches");
#endif
      }
    }
    else {
#ifdef VERBOSE
      LOGE("could not filter proc maps");
#endif
    }
  }
  else {
#ifdef VERBOSE
    LOGE("could not read proc maps");
#endif
  }

  cleanup();

  return patches > 0 ? 0 : 1;
}
