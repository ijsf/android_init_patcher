#include <android/log.h>

// Verbose logging
#define VERBOSE

// Strings to be patched (string lengths have to match!)
#define PATCH_MATCH "sys.powerctl"
#define PATCH_REPLACE "sys.powerlol"

/*
  This program disables regular Android sys.powerctl shutdowns, e.g.:

    setprop sys.powerctl
    reboot
    etc.

  Shutdowns can now be triggered with e.g.:

    setprop sys.powerlol reboot
*/

// Segment options
#define SCAN_HEAP
//#define SCAN_DATA_SEGMENTS
//#define SCAN_STACK_SEGMENTS

// PID options (init process: likely always pid 1)
#define TARGET_PID 1

// Log tagging
#define LOG_TAG "init_patch"
#define LOG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); printf(LOG_TAG ": "); printf(__VA_ARGS__); printf("\n");
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); fprintf(stderr, LOG_TAG ": "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");
