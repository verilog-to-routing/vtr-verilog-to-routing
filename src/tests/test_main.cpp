#include <iostream>
#include "UnitTest++.h"
#include "TestReporterStdout.h"

using namespace UnitTest;

//Basic checks to ensure UnitTest++ works...
SUITE(TestUnitTestPP) {
    TEST(check_true) {
        CHECK(true);
    }

    TEST(check_equal) {
        CHECK_EQUAL(1, 1);
    }

}

int main (int argc, char* argv[]) {

    if(argc > 1) {
        int test_count = 0;
        bool run_suite = (std::string("suite") == argv[1]);

        TestList& all_tests( Test::GetTestList() );
        TestList tests_to_run;
        Test* test;

        std::cout << "Initial Running " << test_count << " Test(s): " << argv[0] << std::endl;
        for(test = tests_to_run.GetHead(); test != nullptr; test = test->next) {
            std::cout << "Suite: " << test->m_details.suiteName << " Test: " <<  test->m_details.testName << std::endl;
        }

        test = all_tests.GetHead();
        while(test) {
            for(int i = 1; i < argc; i++) {
                bool add_test = false;
                if(run_suite) {
                    add_test = (std::string(argv[i]) == test->m_details.suiteName);
                } else {
                    add_test = (std::string(argv[i]) == test->m_details.testName);
                }

                if(add_test) {
                    tests_to_run.Add(test);
                    test_count++;
                }
            }
            Test* prev_test = test;
            //Advance
            test = test->next;
            
            //Must break the prev pointer to avoid including more tests than wanted
            prev_test->next = nullptr;

        }

        std::cout << "Running " << test_count << " Test(s): " << argv[0] << std::endl;
        for(test = tests_to_run.GetHead(); test != nullptr; test = test->next) {
            std::cout << "Suite: " << test->m_details.suiteName << " Test: " <<  test->m_details.testName << std::endl;
        }

        TestReporterStdout reporter;
        TestRunner runner(reporter);
        return runner.RunTestsIf(tests_to_run, 0, True(), 0);

    } else {
        std::cout << "Running All Tests: " << argv[0] << std::endl;
        return RunAllTests();
    }
}
