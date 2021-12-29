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

/**
 * Each test runs as its own process so they each start from the same state
 */
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

/* Clear fixuture */
SCUTEST_SET_FIXTURE(NULL, NULL);
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

/*
 * When tests timeout they exit with status 9
 */
SCUTEST(test_timeout_single_test, .exitCode = 9, .timeout = 1) {
    msleep(4000);
    assert(0);
}
SCUTEST_SET_FIXTURE(NULL, NULL, .timeout = 1);
SCUTEST_ITER_ERR(test_timeout, 2, 9) {
    msleep(4000);
    assert(0);
}


static void saveStatus() {
    SCUTEST_PASSED_COUNT = SCUTEST_NUM_FAILED_TESTS = 0;
}
SCUTEST_SET_FIXTURE(saveStatus, NULL);

SCUTEST(test_exit_status) {
    static nested = 0;
    if (nested) {
        exit(1);
    }
    nested = 1;
    assert(1 == runUnitTests2(__FILE__, "test_exit_status", 0, 0, 0, 0));
}

SCUTEST(test_no_fork) {
    setenv("NO_FORK", "1", 1);
    setenv("TEST_FILE", "__FILE__", 1);
    setenv("TEST_FUNC", "test_no_fork", 1);
    static pid;
    if (pid) {
        assert(pid == getpid());
    }
    pid = getpid();
    assert(!runUnitTests());
}

SCUTEST(test_specific_index) {
    if(_i == 100)
        return;
    setenv("TEST_FUNC", "test_specific_index:100", 1);
    assert(!runUnitTests());
}
