#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tester.h"

void(*TESTS[100])(void);
int NUM_TESTS;

__SCUTEST_SETUP __setup[100];
int NUM_SETUPS;

SCUTEST __tests[100];


typedef SCUTEST  Test;
struct FailedTest {Test* t; int index; int status;};
struct FailedTest failedTests[1000];
static size_t passedCount = 0;
static int NUM_FAILED_TESTS;
static bool noFork;
static int childPid;
static int TIMEOUT = 5;

void (*preTestFunc)() = NULL;

static void printResults(int signal) {
    if(signal)
        printf("aborting\n");
    printf("....................\n");
    printf("Passed %ld/%ld tests\n", passedCount, passedCount + NUM_FAILED_TESTS );
    for(int i = 0; i < NUM_FAILED_TESTS; i++) {

        Test*t = failedTests[i].t;
        int index = failedTests[i].index;
        int status = failedTests[i].status;

        printf("%s:%03d %s.%d (%d of %d) #%02d failed with status %d\n",
                t->fileName, t->lineNumber, t->name, index, index, t->end, t->testNumber,
            status);
    }
    if(signal)
        exit(signal);
}

static void killChild() {
    if(kill(childPid, SIGKILL)) {
        perror("Failed to kill child");
    }
}
static int runTest(Test*test, int i) {
    assert(test->end);
    __SCUTEST_SETUP* setUpTearDown = NULL;
    for(int i = NUM_SETUPS - 1; i>=0 ; i--)
        if(__setup[i].lineNumber < test->lineNumber && strcmp(test->fileName, __setup[i].fileName) == 0){
            setUpTearDown = &__setup[i];
            break;
        }
    if(noFork || !(childPid = fork())) {
        printf("%s:%03d %s.%d...", test->fileName, test->lineNumber, test->name, i);
        if(!noFork)
            signal(SIGINT, NULL);
        if(setUpTearDown && setUpTearDown->setUp)
            setUpTearDown->setUp();
        if(preTestFunc)
            preTestFunc();
        test->testFunc(i);
        if(setUpTearDown && setUpTearDown->tearDown)
            setUpTearDown->tearDown();
        if(!noFork)
            exit(0);
    }
    int exitStatus;
    if(!noFork) {
        alarm(setUpTearDown && setUpTearDown->timeout? setUpTearDown->timeout: TIMEOUT);
        int status = 0;
        waitpid(childPid , &status, 0);
        exitStatus = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
        alarm(0);
    }
    bool passed = exitStatus == test->exitCode;
    printf("%s\n", passed?"passed": "failed");
    if(!passed) {
        failedTests[NUM_FAILED_TESTS++]=(struct FailedTest ){test, i, exitStatus};
    }
    passedCount+=passed;
    return passed;
}

int runUnitTests() {
    assert(NUM_TESTS);
    noFork = getenv("NO_FORK");
    char* file = getenv("TEST_FILE");
    char* func = getenv("TEST_FUNC");
    char* index = func ? strchr(func, '.') : NULL;
    if(index) {
        *index = 0;
        index++;
    }
    char* strictStr = getenv("STRICT");;
    bool strict = strictStr ? strcmp(strictStr, "0") : 0;
    bool veryStrict = strict && strictStr ? strcmp(strictStr, "2") : 0;
    signal(SIGINT, printResults);
    signal(SIGALRM, killChild);
    for(int i = 0; i < NUM_TESTS; i++) {
        Test* t = __tests + i;
        if((!file || strcmp(file, t->fileName) == 0) && (!func || strcmp(func, t->name) == 0)) {
            if(index)
                runTest(t, atoi(index));
            else
                for(int i = 0; i < t->end; i++)
                    if(!runTest(t, i) && veryStrict)
                        break;
            if(strict && NUM_FAILED_TESTS)
                break;
        }
    }
    printResults(0);
    return NUM_FAILED_TESTS?1:0;
}
