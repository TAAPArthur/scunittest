#ifndef MPX_TESTER_H
#define MPX_TESTER_H

typedef struct SCUTEST {
    void(*testFunc)(int i);
    int end;
    int exitCode;
    int lineNumber;
    int testNumber;
    const char* name;
    const char* fileName;
    int status;
}SCUTEST ;

typedef struct {
    void(*setUp)();
    void(*tearDown)();
    int timeout;
    int lineNumber;
    const char* fileName;
} __SCUTEST_SETUP ;

extern SCUTEST __tests[100];
extern __SCUTEST_SETUP __setup[100];
extern int NUM_TESTS;
extern int NUM_SETUPS;

#define __SCUTEST_CAT(x, y) x ## y
#define _SCUTEST_CAT(x, y) __SCUTEST_CAT(x, y)

#define SCUTEST(name) SCUTEST_ITER(name,1)
#define SCUTEST_ITER(name,end) SCUTEST_ITER_ERR(name,end,0)
#define SCUTEST_ERR(name,err) SCUTEST_ITER_ERR(name,1,err)
#define SCUTEST_ITER_ERR(N,end,err) \
 void N(int i); \
__attribute__((constructor)) static void _SCUTEST_CAT(N, __LINE__)() {\
    __tests[NUM_TESTS++] = (SCUTEST) {N, end, err, __LINE__, NUM_TESTS, # N,  __FILE__}; \
}\
void N(int _i __attribute__((unused)))


#define SCUTEST_SET_ENV(setUp, tearDown) SCUTEST_SET_ENV_TIMEOUT(setUp, tearDown, 0)

#define SCUTEST_SET_ENV_TIMEOUT(setUp, tearDown, timeout) \
__attribute__((constructor)) static void _SCUTEST_CAT(setupTearDown,__LINE__)() {\
    __setup[NUM_SETUPS++] = (__SCUTEST_SETUP) {setUp, tearDown, timeout, __LINE__, __FILE__}; \
}
int runUnitTests();
#ifdef SCUTEST_DEFINE_MAIN
int main() { return runUnitTests(); }
#endif
#endif
