#define SCUTEST_DEFINE_MAIN
#include "tester.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

SCUTEST(test_assert){
    assert(1);
}

SCUTEST(test_assert_index){
    assert(_i==0);
}

static int value;
SCUTEST_ITER(test_fork, 2){
    assert(value++==0);
}

SCUTEST_ERR(test_err, 2){
    exit(2);
}

static void setUp() {
    value = 1;
}
static void tearDown() {
    if(value != 1)
        exit(6);
}
SCUTEST_SET_ENV(setUp, tearDown);

SCUTEST(test_setup_fork){
    assert(value==1);
}
SCUTEST_ERR(test_teardown, 6){
    assert(value++==1);
}
/**
 * Sleep of mil milliseconds
 * @param ms number of milliseconds to sleep
 */
static inline void msleep(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    while(nanosleep(&ts, &ts));
}
SCUTEST_SET_ENV_TIMEOUT(NULL, NULL, 1);
SCUTEST_ERR(test_timeout, 9){
    msleep(40000);
    assert(0);
}
