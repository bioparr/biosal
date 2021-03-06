
#include "debugger.h"

#include "timer.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

void core_debugger_examine(void *pointer, int bytes)
{
    int position;
    char *array;
    char byte_value;

    array = pointer;

    for (position = 0; position < bytes; position++) {
        byte_value = array[position];
        printf(" %x", byte_value & 0xff);
    }

    printf("\n");
}

void core_debugger_jitter_detection_start(struct core_timer *timer)
{
    core_timer_init(timer);
    core_timer_start(timer);
}

void core_debugger_jitter_detection_end(struct core_timer *timer, const char *name, uint64_t actor_time)
{
    uint64_t elapsed_time;
    uint64_t threshold;

    core_timer_stop(timer);
    elapsed_time = core_timer_get_elapsed_nanoseconds(timer);
    core_timer_destroy(timer);

    elapsed_time -= actor_time;

    /*
     * An iteration should not take more than
     * 30 us.
     */
    threshold = 10 * 1000;

    if (elapsed_time >= threshold) {
        printf("core_debugger: Warning, detected jitter (%" PRIu64 " ns) at \"%s\"\n",
                        elapsed_time, name);
    }
}

void core_exit_with_error()
{
    int error_status;

    /*
     * From ALCF support:
     *
     * According to Tom Gooding from IBM, the default behaviour is
     * this:
     *
     *                by default  job completes when
     *                             ---------------------------------
     *                             exit(0)       all tasks have exited
     *                             exit(1)       immediately
     *                             exit(2)       all taks have exited
     *                             exit(30)     all tasks have exited
     */
#ifdef __bgq__
    error_status = 2;
#else
    error_status = 1;
#endif

    exit(error_status);
}
