/** Jason Luu
 * Test program for logger
 */

#include "log.h"

int main() {
    int x = 10, y = 20;
    float a = 1.5f, b = -2.01f;
    log_print_info("Testing logger\n\n");
    log_print_info("Output separate strings: %s %s\n", "pass", "[PASS]");
    log_print_info("Output two integers: x = %d y = %d\n", x, y);
    log_print_warning(__FILE__, __LINE__, "Test warning on floating point arguments %g %g\n", a, b);
    log_print_error(__FILE__, __LINE__, "Test error on two variables %g %g \n\n", a - x, b + y);

    log_print_info("Test complete\n");
    return 0;
}