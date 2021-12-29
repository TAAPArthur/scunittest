
# Simple C Unit Tests

# Install
```
make
# make install-header # install single-header
# make install-lib    # install as a library
make install        # install both
```
When install as a library link with `-lscuttest`
When using it a header lib `#define SCUTEST_IMPLEMENTATION` in one file
# Running

Either add `#define SCUTEST_DEFINE_MAIN` before including anything else or manually call runUnitTests like
`int main() { return runUnitTests(); }`

# Features
1. c99 compliant
2. Simple api. There isn't an annoying step of "registering" the test after it has been created
3. ~300 lines
4. Each test runs as its own process so state won't leak between tests.
5. Hide output of passing tests by default
6. Controllable with env vars `NO_FORK` , `NO_BUFFER` `TEST_FILE` `TEST_FUNC`

# Examples
See [sample-test.c](sample-test.c)
Simple test
```
SCUTEST(test_assert) {
    printf("THIS_MESSAGE_SHOULD_NOT_BE_SEEN");
    assert(1);
}
```
Iterations
```
static int value;
// Will run twice and value will be 0 each time
// the iteration number is stored in local variable `_i`
SCUTEST_ITER(test_fork, 2) {
    assert(value++ == 0);
}
```
Fixtures
```
SCUTEST_SET_FIXTURE(setUp, tearDown);
# Now setUp and tearDown will run before/after every test until the next call to SCUTEST_SET_FIXTURE
```
Test timeout and non-zero exit status
```
// When a test times out it will exit with status 9
SCUTEST(test_timeout_single_test, .exitCode = 9, .timeout = 1) {
    msleep(4000);
    assert(0);
}
```
