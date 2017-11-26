#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/ptrace.h>
#include <inttypes.h>
#include "patch.h"
#include "main.h"

int procmap_init(procmap *map, pid_t pid) {
  size_t i;
  map->pid = pid;
  map->count = 0;

  for (i=0; i<MAX_RECORDS; ++i) {
    map->records[i].info[0] = '\0';
  }

  return 0;
}

int read_maps(procmap *map, FILE *ifile) {
  char map_fn[64];
  char buf[BUFLEN];
  procmap_record *it;

  // try to open /prod/<pid>/maps if file is not supplied
  if (!ifile) {
    snprintf(map_fn, sizeof(map_fn), "/proc/%d/maps", map->pid);
    ifile = fopen(map_fn, "r");
    if (!ifile) {
#ifdef VERBOSE
      LOGE("could not open %s", map_fn);
#endif
      return -1;
    }
  }

  // read all the records from maps and parse them
  map->count = 0;
  while (fgets(buf, sizeof(buf), ifile) && map->count < MAX_RECORDS)
  {
    it = &map->records[map->count];
    memset(it, 0, sizeof(procmap_record));
    buf[strlen(buf)-1] = 0;
    sscanf((const char *)buf, MAPI_FMT, &it->begin, &it->end, &it->read,
    &it->write, &it->exec, &it->shared, &it->offset, &it->dev_major,
    &it->dev_minor, &it->inode, it->info);
    char* newline = strchr(it->info, '\n');
    if (newline)
      *newline = 0;
    memset(buf, 0, sizeof(buf));
    map->count++;
  }

  fclose(ifile);

  if (map->count == MAX_RECORDS) {
#ifdef VERBOSE
    LOGE("max segments exceeded");
#endif
  }

  return 0;
}

bool patch(pid_t pid, const void *addr_start, const char *payload, const size_t payload_length) {
  // PTRACE_PEEKTEXT and PTRACE_POKETEXT do not have to be word-aligned
  
  // Cut payload_length up in words since PTRACE_PEEKTEXT and PTRACE_POKETEXT work on word sizes only
  const size_t payload_words = payload_length / WORD;

  // In-place patch
  size_t patched_bytes = 0;
  for(size_t offset_words = 0; offset_words < payload_words; ++offset_words) {
    const void *addr = ((uint8_t*)addr_start + (offset_words * WORD));

    // Peek data
    const long word = ptrace(PTRACE_PEEKTEXT, pid, (void *)addr, NULL);
    char *bytes = (char *)&word;
    if (errno) {
      return false;
    }
    
    // Patch using payload
    size_t patch_remainder = (payload_length - patched_bytes);
    if (patch_remainder > WORD) {
      patch_remainder = WORD;
    }
    memcpy((void *)bytes, (void *)&payload[patched_bytes], patch_remainder);
    patched_bytes += patch_remainder;

    // Poke data
    const long result = ptrace(PTRACE_POKETEXT, pid, (void *)(addr), (void *)(word));
    if (result != 0 || errno != 0) {
      return false;
    }
  }
  return true;
}

int patch_memory_strings(pid_t pid, const void *addr_start, const size_t len, const char *match, const char *replace) {
  // caller must make sure this does not overflow
  // caller must make sure strlen(replace) == strlen(match)

  errno = 0;

  const size_t match_length = strlen(match);
  const size_t replace_length = strlen(replace);
  void * first_match_addr = NULL;
  size_t offset_in_match = 0;
  int patches = 0;

  for (size_t offset = 0; offset < len; offset += WORD) {
    const void *addr = ((uint8_t*)addr_start + offset);

    // Peek data
    const long word = ptrace(PTRACE_PEEKTEXT, pid, (void *)(addr), NULL);
    const char *bytes = (char*)&word;
    if (errno != 0) {
      return -1;
    }
    // Search for match
    for (size_t m = 0; m < WORD; ++m) {
      // Check each of WORD bytes for character match
      if (bytes[m] == match[offset_in_match]) {
        // Match
        if (offset_in_match == 0) {
          // First match, store byte offset of first match
          first_match_addr = (void *)((uint8_t*)addr + m);
        }
        ++offset_in_match;

        // Check for complete match
        if (offset_in_match == match_length) {
#ifdef VERBOSE
          LOG("patching complete match at address 0x%" PRIx64, (uint64_t)first_match_addr);
#endif
          if (patch(pid, first_match_addr, (const char*)replace, replace_length)) {
            ++patches;
          }
          else {
            // Fail immediately
            return -1;
          }
          // No (more) match, reset variables
          offset_in_match = 0;
        }
      }
      else {
        // No (more) match, reset variables
        offset_in_match = 0;
      }
    }
  }
  return patches;
}
