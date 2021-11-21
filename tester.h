#ifndef SCUTEST_H
#define SCUTEST_H

typedef struct {
    void(*testFunc)(int i);
    int lineNumber;
    int testNumber;
    const char* name;
    const char* fileName;
    char status;
    int iter;
    char exitCode;
    int timeout;
} SCUTEST_TestInfo;

typedef struct {
    void(*setUp)();
    void(*tearDown)();
    int lineNumber;
    const char* fileName;
    int timeout;
} SCUTEST_FixtureInfo;

extern SCUTEST_TestInfo _SCUTEST_tests[];
extern SCUTEST_FixtureInfo _SCUTEST_fixtures[];
extern int SCUTEST_NUM_TESTS;
extern int SCUTEST_NUM_FIXTURES;

#define __SCUTEST_CAT(x, y) x ## y
#define _SCUTEST_CAT(x, y) __SCUTEST_CAT(x, y)

#define SCUTEST_ITER(N,END) SCUTEST(N, .iter=END)
#define SCUTEST_ERR(N,ERR) SCUTEST(N, .exitCode=ERR)
#define SCUTEST_ITER_ERR(N,END,ERR) SCUTEST(N, .iter=END, .exitCode=ERR)
#define SCUTEST_NO_ARGS(N) SCUTEST(N, 0)
#define SCUTEST(N, ...) \
 void N(int i); \
__attribute__((constructor)) static void _SCUTEST_CAT(__scutest_,_SCUTEST_CAT(N, __LINE__))() {\
    _SCUTEST_tests[SCUTEST_NUM_TESTS++] = (SCUTEST_TestInfo) {N,  __LINE__, SCUTEST_NUM_TESTS, # N,  __FILE__, __VA_ARGS__}; \
}\
void N(int _i __attribute__((unused)))


// Legacy macro major
#define SCUTEST_SET_ENV(...) SCUTEST_SET_FIXTURE(__VA_ARGS__)

#define SCUTEST_SET_FIXTURE_NO_ARGS(setUp, tearDown) SCUTEST_SET_FIXTURE(setUp, tearDown, 0)
#define SCUTEST_SET_FIXTURE(setUp, tearDown, ...) \
__attribute__((constructor)) static void _SCUTEST_CAT(setupTearDown,__LINE__)() {\
    _SCUTEST_fixtures[SCUTEST_NUM_FIXTURES++] = (SCUTEST_FixtureInfo) {setUp, tearDown, __LINE__, __FILE__, __VA_ARGS__}; \
}
int runUnitTests();
int runUnitTests2(const char* file, const char* func, int index, int noFork, int noBuffer, int strict);
#ifdef SCUTEST_DEFINE_MAIN
int main() { return runUnitTests(); }
#endif
#endif
