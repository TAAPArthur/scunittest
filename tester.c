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

SCUTEST __tests[1000];


typedef SCUTEST  Test;
struct FailedTest {Test* t; int index; int status;};
struct FailedTest failedTests[1000];
static size_t passedCount = 0;
static int NUM_FAILED_TESTS;
static bool noFork;
static bool noBuffer;
static int childPid;
static int TIMEOUT = 5;

void (*preTestFunc)() = NULL;

static void printResults(int signal) {
    if(signal)
        printf("aborting\n");
    printf("....................\n");
    printf("Passed %ld/%ld tests\n", passedCount, passedCount + NUM_FAILED_TESTS);
    for(int i = 0; i < NUM_FAILED_TESTS; i++) {
        Test* t = failedTests[i].t;
        int index = failedTests[i].index;
        int status = failedTests[i].status;
        printf("%s:%03d %s.%d (of %d) #%02d failed with status %d\n",
            t->fileName, t->lineNumber, t->name, index, t->iter, t->testNumber,
            status);
    }
    if(signal)
        exit(signal);
}

#include <fcntl.h>
static int fds[2];
static char* buffer;
static int bufferSize;
#define readSize 255
static void drainBuffer() {
    static int maxBufferSize = readSize * 10;
    buffer = malloc(maxBufferSize);
    int result;
    bufferSize = 0;
    while(result = read(fds[0], buffer + bufferSize, readSize)) {
        if(result != -1)
            bufferSize += result;
        else {
            perror("Failed to read");
            break;
        }
        if(bufferSize + readSize > maxBufferSize) {
            maxBufferSize *= 2;
            buffer = realloc(buffer, maxBufferSize);
        }
    }
    close(fds[0]);
}
static void dumpBuffer(int passed) {
    if(!passed) {
        if(bufferSize)
            write(STDOUT_FILENO, buffer, bufferSize);
    }
    free(buffer);
}

static void killChild() {
    printf("Aborting\n");
    if(kill(childPid, SIGKILL)) {
        perror("Failed to kill child");
    }
}

static int createSigAction(int sig, void(*callback)(int)) {
    struct sigaction sa;
    sa.sa_handler = callback;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NODEFER;
    return sigaction(sig, &sa, NULL);
}
static int runTest(Test* test, int i) {
    __SCUTEST_SETUP* setUpTearDown = NULL;
    for(int i = NUM_SETUPS - 1; i >= 0 ; i--)
        if(__setup[i].lineNumber < test->lineNumber && strcmp(test->fileName, __setup[i].fileName) == 0) {
            setUpTearDown = &__setup[i];
            break;
        }
    printf("%s:%03d %s.%d...", test->fileName, test->lineNumber, test->name, i);
    fflush(NULL);
    if(!noBuffer) {
        pipe(fds);
    }
    if(noFork || !(childPid = fork())) {
        if(!noFork)
            signal(SIGINT, NULL);
        if(!noBuffer) {
            fcntl(fds[0], F_SETFD, O_CLOEXEC);
            dup2(fds[1], STDOUT_FILENO);
            dup2(fds[1], STDERR_FILENO);
            close(fds[1]);
            close(fds[0]);
        }
        createSigAction(SIGALRM, SIG_DFL);
        if(setUpTearDown && setUpTearDown->setUp)
            setUpTearDown->setUp(i);
        if(preTestFunc)
            preTestFunc();
        test->testFunc(i);
        if(setUpTearDown && setUpTearDown->tearDown)
            setUpTearDown->tearDown(i);
        if(!noFork)
            exit(0);
    }
    int exitStatus = -1;
    if(!noFork) {
        if(!noBuffer) {
            close(fds[1]);
        }
        if(test->timeout)
            alarm(test->timeout);
        else
            alarm(setUpTearDown && setUpTearDown->timeout ? setUpTearDown->timeout : TIMEOUT);
    }
    if(!noBuffer)
        drainBuffer();
    if(!noFork) {
        int status = -1;
        if(-1 == waitpid(childPid, &status, 0)) {
            perror("Failed to wait on child");
        }
        exitStatus = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
        alarm(0);
    }
    bool passed = exitStatus == test->exitCode;
    if(!noBuffer)
        dumpBuffer(passed);
    printf("%s\n", passed ? "passed" : "failed");
    if(!passed) {
        failedTests[NUM_FAILED_TESTS++] = (struct FailedTest) {test, i, exitStatus};
    }
    passedCount += passed;
    return passed;
}

int runUnitTests() {
    createSigAction(SIGALRM, killChild);
    createSigAction(SIGINT, printResults);
    assert(NUM_TESTS);
    noFork = getenv("NO_FORK");
    noBuffer = getenv("NO_BUFFER");
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
    for(int i = 0; i < NUM_TESTS; i++) {
        Test* t = __tests + i;
        if((!file || strcmp(file, t->fileName) == 0) && (!func || strcmp(func, t->name) == 0)) {
            if(index)
                runTest(t, atoi(index));
            else
                for(int i = 0; i < (t->iter ? t->iter : 1); i++)
                    if(!runTest(t, i) && veryStrict)
                        break;
            if(strict && NUM_FAILED_TESTS)
                break;
        }
    }
    printResults(0);
    return NUM_FAILED_TESTS ? 1 : 0;
}
