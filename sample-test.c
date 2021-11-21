#define SCUTEST_DEFINE_MAIN
#define SCUTEST_IMPLEMENTATION
#include "scutest.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

SCUTEST(test_assert) {
    printf("THIS_MESSAGE_SHOULD_NOT_BE_SEEN");
    assert(1);
}

SCUTEST(test_assert_index) {
    assert(_i == 0);
}

static int value;
SCUTEST_ITER(test_fork, 2) {
    assert(value++ == 0);
}

SCUTEST_ERR(test_err, 2) {
    exit(2);
}
SCUTEST(test_large_output) {
    for(int i = 0; i < 100000; i++)
        printf("yes");
}

SCUTEST_SET_FIXTURE_NO_ARGS(NULL, NULL);
SCUTEST_NO_ARGS(test_assert_no_args) {
    assert(1);
}

static void setUp() {
    value = 1;
}
static void tearDown() {
    if(value != 1)
        exit(6);
}
SCUTEST_SET_FIXTURE(setUp, tearDown);

SCUTEST(test_setup_fork) {
    assert(value == 1);
}
SCUTEST_ERR(test_teardown, 6) {
    assert(value++ == 1);
}
static int savedIndex;
static void setUpI(int i) {
    savedIndex = i;
}
static void tearDownI(int i) {
    assert(savedIndex == i);
}

SCUTEST_SET_FIXTURE(setUpI, tearDownI);
SCUTEST(test_setup_index, .iter = 2) {
    assert(_i == savedIndex);
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

SCUTEST(test_timeout_single_test, .exitCode = 9, .timeout = 1) {
    msleep(4000);
    assert(0);
}
SCUTEST_SET_FIXTURE(NULL, NULL, .timeout = 1);
SCUTEST_ITER_ERR(test_timeout, 2, 9) {
    msleep(4000);
    assert(0);
}
