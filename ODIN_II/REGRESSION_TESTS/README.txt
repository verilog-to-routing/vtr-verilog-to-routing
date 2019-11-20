This describes the regression testing facilities.

This regresses over the basic MICROBENCHMARKS to check that they're still working.
- to checkin regression results
NOT SUPPORTED YET

- to check regression
SCRIPTS/regression_test_no_args.pl yes LISTS/big_bmarks.list BENCHMARKS/FULL_BENCHMARKS/ BASIC_TESTS/ BASIC_TESTS/GOLDEN_VERSION/

SCRIPTS/regression_test_no_args.pl yes LISTS/regression_micro_working.list BENCHMARKS/MICROBENCHMARKS/ BASIC_TESTS/ BASIC_TESTS/GOLDEN_VERSION/

SCRIPTS/regression_test_no_args.pl yes LISTS/big_bmarks.list BENCHMARKS/FULL_BENCHMARKS/ BASIC_TESTS/ BASIC_TESTS/GOLDEN_VERSION/

# NOTE!!!
# The tests below on the configuration file include path sensitive data. You
# must run this regression test from the directory containing this README
# file!!!

	SCRIPTS/regression_test_arch.pl yes LISTS/regression_arch_working.list BENCHMARKS/ARCH_BENCHMARKS BASIC_TESTS/ BASIC_TESTS/GOLDEN_VERSION/

	SCRIPTS/regression_test_arch.pl yes LISTS/regression_full_working.list BENCHMARKS/FULL_BENCHMARKS BASIC_TESTS/ BASIC_TESTS/GOLDEN_VERSION/

	SCRIPTS/regression_test_arch.pl yes LISTS/regression_micro_working.list BENCHMARKS/MICROBENCHMARKS BASIC_TESTS/ BASIC_TESTS/GOLDEN_VERSION/
