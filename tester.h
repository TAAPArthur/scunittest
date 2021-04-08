#ifndef MPX_TESTER_H
#define MPX_TESTER_H

typedef struct SCUTEST {
    void(*testFunc)(int i);
    int lineNumber;
    int testNumber;
    const char* name;
    const char* fileName;
    int status;
    int iter;
    int exitCode;
    int timeout;
} SCUTEST ;

typedef struct {
    void(*setUp)();
    void(*tearDown)();
    int lineNumber;
    const char* fileName;
    int timeout;
} __SCUTEST_SETUP ;

extern SCUTEST __tests[1000];
extern __SCUTEST_SETUP __setup[100];
extern int NUM_TESTS;
extern int NUM_SETUPS;

#define __SCUTEST_CAT(x, y) x ## y
#define _SCUTEST_CAT(x, y) __SCUTEST_CAT(x, y)

#define SCUTEST_ITER(N,END) SCUTEST(N, .iter=END)
#define SCUTEST_ERR(N,ERR) SCUTEST(N, .exitCode=ERR)
#define SCUTEST_ITER_ERR(N,END,ERR) SCUTEST(N, .iter=END, .exitCode=ERR)
#define SCUTEST(N, ARGS...) \
 void N(int i); \
__attribute__((constructor)) static void _SCUTEST_CAT(__scutest_,_SCUTEST_CAT(N, __LINE__))() {\
    __tests[NUM_TESTS++] = (SCUTEST) {N,  __LINE__, NUM_TESTS, # N,  __FILE__, ARGS}; \
}\
void N(int _i __attribute__((unused)))


#define SCUTEST_SET_ENV(setUp, tearDown, ARGS...) \
__attribute__((constructor)) static void _SCUTEST_CAT(setupTearDown,__LINE__)() {\
    __setup[NUM_SETUPS++] = (__SCUTEST_SETUP) {setUp, tearDown, __LINE__, __FILE__, ARGS}; \
}
int runUnitTests();
#ifdef SCUTEST_DEFINE_MAIN
int main() { return runUnitTests(); }
#endif
#endif
