#if defined(SCUTEST_IMPLEMENTATION) || ! defined(SCUTEST_H)
#define _XOPEN_SOURCE 700
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


#ifndef SCUTEST_NO_STDLIB
#include <stdlib.h>
#define SCUTEST_ATOI(nptr) atoi(nptr)
#else
#define SCUTEST_ATOI(nptr) (nptr[0] >= '0' && nptr[0] <= '9' ? nptr[0] - '0' : -1)
#endif

#if ! defined(SCUTEST_NO_STDLIB) && ! defined(SCUTEST_EXIT_FUNC)
#define SCUTEST_EXIT_FUNC(R) exit(R)
#elif defined(SCUTEST_NO_STDLIB)
#define SCUTEST_EXIT_FUNC(R)
#endif

#if ! defined(SCUTEST_NO_STDLIB) && !(defined SCUTEST_GETENV_FUNC)
#define SCUTEST_GETENV_FUNC(R) getenv(R)
#elif defined(SCUTEST_NO_STDLIB)
#define SCUTEST_GETENV_FUNC(R) NULL
#endif

#ifndef SCUTEST_NO_STDIO
#include <stdio.h>
#define SCUTEST_PRINTF(...) printf(__VA_ARGS__)
#define SCUTEST_PERROR(MSG) perror(MSG)
#define SCUTEST_FLUSH() fflush(NULL)
#else
#define SCUTEST_PRINTF(...)
#define SCUTEST_PERROR(MSG)
#define SCUTEST_FLUSH()
#endif

#if ! defined(SCUTEST_NO_BUFFER) && ! defined(SCUTEST_NO_STDLIB)
#include <fcntl.h>
#endif

#ifndef SCUTEST_H
#include "tester.h"
#endif

#ifndef SCUTEST_MAX_NUM_TESTS
#define SCUTEST_MAX_NUM_TESTS 1024
#endif

#ifndef SCUTEST_MAX_NUM_FIXTURES
#define SCUTEST_MAX_NUM_FIXTURES 100
#endif

#ifndef SCUTEST_BUFFER_READ_SIZE
#define SCUTEST_BUFFER_READ_SIZE 255
#endif

#ifndef SCUTEST_DEFAULT_TIMEOUT
#define SCUTEST_DEFAULT_TIMEOUT 5
#endif

int SCUTEST_NUM_TESTS;
int SCUTEST_NUM_FIXTURES;
SCUTEST_FixtureInfo _SCUTEST_fixtures[SCUTEST_MAX_NUM_FIXTURES];

SCUTEST_TestInfo _SCUTEST_tests[SCUTEST_MAX_NUM_TESTS];


struct SCUTEST_FailedTest {SCUTEST_TestInfo* t; int index; int status;};
static struct SCUTEST_FailedTest failedTests[SCUTEST_MAX_NUM_TESTS];
static size_t SCUTEST_PASSED_COUNT = 0;
static int SCUTEST_NUM_FAILED_TESTS;
static int SCUTEST_CHILD_PID;

static void SCUTEST_printResults(int signal) {
    if(signal)
        SCUTEST_PRINTF("aborting\n");
    SCUTEST_PRINTF("....................\n");
    SCUTEST_PRINTF("Passed %ld/%ld tests\n", SCUTEST_PASSED_COUNT, SCUTEST_PASSED_COUNT + SCUTEST_NUM_FAILED_TESTS);
    for(int i = 0; i < SCUTEST_NUM_FAILED_TESTS; i++) {
        SCUTEST_TestInfo* t = failedTests[i].t;
        int index = failedTests[i].index;
        int status = failedTests[i].status;
        SCUTEST_PRINTF("%s:%03d %s.%d (of %d) #%02d failed with status %d\n",
            t->fileName, t->lineNumber, t->name, index, t->iter, t->testNumber,
            status);
    }
    if(signal)
        SCUTEST_EXIT_FUNC(signal);
}

#ifndef SCUTEST_NO_BUFFER
static char* SCUTEST_drainBuffer(int fd, int*bufferSize) {
    static int maxBufferSize = SCUTEST_BUFFER_READ_SIZE;
    char* buffer = malloc(maxBufferSize);
    int result;
    *bufferSize = 0;
    while(result = read(fd, buffer + *bufferSize, SCUTEST_BUFFER_READ_SIZE)) {
        if(result != -1)
            *bufferSize += result;
        else {
            SCUTEST_PERROR("Failed to read");
            break;
        }
        if(*bufferSize + SCUTEST_BUFFER_READ_SIZE > maxBufferSize) {
            maxBufferSize *= 2;
            buffer = realloc(buffer, maxBufferSize);
        }
    }
    close(fd);
    return buffer;
}
static void SCUTEST_dumpBuffer(void* buffer, int bufferSize, int passed) {
    if(!passed) {
        if(bufferSize)
            write(STDOUT_FILENO, buffer, bufferSize);
    }
    free(buffer);
}
#endif

static void SCUTEST_killChild() {
    SCUTEST_PRINTF("Aborting\n");
    if(kill(SCUTEST_CHILD_PID, SIGKILL)) {
        SCUTEST_PERROR("Failed to kill child");
    }
}

static int SCUTEST_createSigAction(int sig, void(*callback)(int)) {
    struct sigaction sa;
    sa.sa_handler = callback;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NODEFER;
    return sigaction(sig, &sa, NULL);
}
static int SCUTEST_runTest(SCUTEST_TestInfo* test, int i, int noFork, int noBuffer) {
    SCUTEST_FixtureInfo * setUpTearDown = NULL;

    static int fds[2];
    static int bufferSize;
    for(int i = SCUTEST_NUM_FIXTURES - 1; i >= 0 ; i--)
        if(_SCUTEST_fixtures[i].lineNumber < test->lineNumber && strcmp(test->fileName, _SCUTEST_fixtures[i].fileName) == 0) {
            setUpTearDown = &_SCUTEST_fixtures[i];
            break;
        }
    SCUTEST_PRINTF("%s:%03d %s.%d...", test->fileName, test->lineNumber, test->name, i);
    SCUTEST_FLUSH();

#ifndef SCUTEST_NO_BUFFER
    if(!noBuffer) {
        pipe(fds);
    }
#endif
    if(noFork || !(SCUTEST_CHILD_PID = fork())) {
        if(!noFork)
            signal(SIGINT, NULL);
#ifndef SCUTEST_NO_BUFFER
        if(!noBuffer) {
            dup2(fds[1], STDOUT_FILENO);
            dup2(fds[1], STDERR_FILENO);
            close(fds[1]);
            close(fds[0]);
        }
#endif
        SCUTEST_createSigAction(SIGALRM, SIG_DFL);
        if(setUpTearDown && setUpTearDown->setUp)
            setUpTearDown->setUp(i);
        test->testFunc(i);
        if(setUpTearDown && setUpTearDown->tearDown)
            setUpTearDown->tearDown(i);
        if(!noFork)
            SCUTEST_EXIT_FUNC(0);
    }
    int exitStatus = -1;
    if(!noFork) {
#ifndef SCUTEST_NO_BUFFER
        if(!noBuffer) {
            close(fds[1]);
        }
#endif
        if(test->timeout)
            alarm(test->timeout);
        else
            alarm(setUpTearDown && setUpTearDown->timeout ? setUpTearDown->timeout : SCUTEST_DEFAULT_TIMEOUT);
    }
#ifndef SCUTEST_NO_BUFFER
    static char* buffer;
    if(!noBuffer)
        buffer = SCUTEST_drainBuffer(fds[0], &bufferSize);
#endif
    if(!noFork) {
        int status = -1;
        if(-1 == waitpid(SCUTEST_CHILD_PID, &status, 0)) {
            SCUTEST_PERROR("Failed to wait on child");
        }
        exitStatus = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
        alarm(0);
    }
    int passed = exitStatus == test->exitCode;
#ifndef SCUTEST_NO_BUFFER
    if(!noBuffer)
        SCUTEST_dumpBuffer(buffer, bufferSize, passed);
#endif
    SCUTEST_PRINTF("%s\n", passed ? "passed" : "failed");
    if(!passed) {
        failedTests[SCUTEST_NUM_FAILED_TESTS++] = (struct SCUTEST_FailedTest) {test, i, exitStatus};
    }
    SCUTEST_PASSED_COUNT += passed;
    return passed;
}

int runUnitTests2(const char* file, const char* func, int index, int noFork, int noBuffer, int strict) {
    for(int i = 0; i < SCUTEST_NUM_TESTS; i++) {
        SCUTEST_TestInfo* t = _SCUTEST_tests + i;
        if((!file || strcmp(file, t->fileName) == 0) && (!func || strcmp(func, t->name) == 0)) {
            if(index >= 0)
                SCUTEST_runTest(t, index, noFork, noBuffer);
            else
                for(int i = 0; i < (t->iter ? t->iter : 1); i++)
                    if(!SCUTEST_runTest(t, i, noFork, noBuffer) && strict == 2)
                        break;
            if(strict && SCUTEST_NUM_FAILED_TESTS)
                break;
        }
    }
    SCUTEST_printResults(0);
    return SCUTEST_NUM_FAILED_TESTS ? 1 : 0;
}

int runUnitTests() {
    SCUTEST_createSigAction(SIGALRM, SCUTEST_killChild);
    SCUTEST_createSigAction(SIGINT, SCUTEST_printResults);
    int noFork = !!SCUTEST_GETENV_FUNC("NO_FORK");
    int noBuffer = !!SCUTEST_GETENV_FUNC("NO_BUFFER");
    char* file = SCUTEST_GETENV_FUNC("TEST_FILE");
    char* func = SCUTEST_GETENV_FUNC("TEST_FUNC");
    char* index = func ? strchr(func, '.') : NULL;
    if(index) {
        *index = 0;
        index++;
    }
    char* strictStr = SCUTEST_GETENV_FUNC("STRICT");
    int strict = strictStr ? strcmp(strictStr, "0") : 0;
    int veryStrict = strict && strictStr ? strcmp(strictStr, "2") : 0;
    return runUnitTests2(file, func, index ? SCUTEST_ATOI(index) : -1, noFork, noBuffer, strictStr ? SCUTEST_ATOI(strictStr ) : 0);
}
#endif
