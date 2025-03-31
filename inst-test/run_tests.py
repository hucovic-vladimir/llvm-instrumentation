from simple import simple
from ccsds import ccsds
from testlib import print_test_results

if(__name__ == "__main__"):
    number_passed = 0
    number_failed = 0
    runs = [simple.test_simple(), ccsds.test_ccsds()]
    for run in runs:
        if not run:
            number_passed += 1;
        else:
            number_failed += 1;

    print_test_results(number_failed, number_passed)
