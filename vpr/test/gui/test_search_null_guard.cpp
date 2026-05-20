/**
 * @file test_search_null_guard.cpp
 * @brief GUI-T-019 — Search-subsystem null-guard symmetry (Layer 4).
 *
 * Closes the symmetric-guard gap recorded in
 * doc/src/dev/vpr_gui_defect_issues_log.rst alongside DEF-005 and DEF-012:
 *
 *   DEF-005 added a null-`app` early-return to `act_on_mouse_move`.
 *   DEF-012 named `act_on_key_press` and `act_on_mouse_press` as siblings
 *           that lack the same defensive guard.
 *
 * The search subsystem entry points are
 *   * search_and_highlight(widget, app)
 *   * enable_autocomplete(app)
 *   * search_type_changed(self, app)
 *
 * Empirical Layer-4 audit (this PR, against the current branch on Linux):
 *
 *   * `enable_autocomplete(nullptr)` — returns cleanly. The `find_widget`
 *     internals iterate `QApplication::allWidgets()` without dereferencing
 *     `this`; on a fresh process with no main.ui loaded the lookup
 *     returns nullptr and the existing `if (!searchBar) return;` guard
 *     catches it. No defect.
 *
 *   * `search_type_changed(combo, nullptr)` — returns cleanly for the same
 *     reason: the `app->find_line_edit("TextInput")` lookup misses and
 *     the existing `if (!searchBar) return;` triggers. No defect.
 *
 *   * `search_and_highlight(nullptr, nullptr)` — Previously SEGVd (DEF-013,
 *     now closed). Null-app and null-widget guards were added to
 *     search_and_highlight() and get_search_type() in search_bar.cpp.
 *     The function now returns cleanly when `app` is nullptr.
 *
 * Per §4 of the test plan we file the failure first and then assert the
 * contract. DEF-013 is now closed — the `[!shouldfail]` annotation has
 * been removed and the test is a positive-contract case.
 *
 * The two non-crashing entry points get positive contract cases (no
 * `[!shouldfail]`) that pin today's behaviour: a future change that adds
 * an unconditional `app->...` deref at the top of either would flip the
 * case to FAIL and surface the regression immediately.
 *
 * Mechanism:
 *   Each contract assertion runs in a child process via fork(); the child
 *   invokes the entry point with a null `ezgl::application*`. A clean
 *   child-exit (status 0) is the contract. A crash (SIGSEGV / SIGABRT) or
 *   any non-zero exit means the guard is missing. Forking isolates the
 *   crash to a child the parent can examine via waitpid(), so a real
 *   regression does not tear down the whole `test_vpr_gui` binary.
 *
 * Platform scope — POSIX only:
 *   The null-guard contracts are codepath-agnostic C++ assertions (no OS-
 *   specific branches inside search_and_highlight / enable_autocomplete /
 *   search_type_changed), so verifying them on a single OS is sufficient.
 *   This file is gated on !_WIN32 because the crash-isolation harness uses
 *   fork() + waitpid(), which have no Win32 equivalent. A Windows port
 *   (VEH + longjmp, or CreateProcess + re-exec self) would add significant
 *   platform-conditional code without adding test coverage — a regression
 *   in any of the three entry points will already fail on the POSIX build.
 *
 * Tag: [layer4][interactive][search][vpr_gui][GUI-T-019]
 */

#include <catch2/catch_test_macros.hpp>

// POSIX-only — see the "Platform scope" note in the file docstring. The
// rest of the file is gated on !_WIN32 so this TU compiles to nothing on
// Windows (Catch2 is fine with TUs that register no test cases).
#if !defined(_WIN32)

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <functional>

#include <QComboBox>

#include <ezgl/application.hpp>

// The three entry points under contract.
#include "search_bar.h"

namespace {

// Result of running a callable in a child process.
struct ChildOutcome {
    bool exited_cleanly = false; // child returned via exit(0)
    int exit_code = -1;
    int signal_no = 0; // non-zero if killed by a signal
};

// Run `body` in a child process. The child exit-codes back to the parent;
// any signal-induced termination is reported via signal_no. Catches the
// "guard returned cleanly" outcome (child runs to end, exits 0) and the
// "guard missing" outcome (signal or non-zero exit) without taking down
// the host test binary.
ChildOutcome run_in_child(std::function<void()> body) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child. Reset SIGABRT/SIGSEGV to default so any crash is observable
        // as a signal rather than swallowed by an inherited handler.
        std::signal(SIGABRT, SIG_DFL);
        std::signal(SIGSEGV, SIG_DFL);
        body();
        std::_Exit(0); // body returned -> guard worked
    }
    REQUIRE(pid > 0);

    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    REQUIRE(w == pid);

    ChildOutcome o{};
    if (WIFEXITED(status)) {
        o.exited_cleanly = (WEXITSTATUS(status) == 0);
        o.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        o.signal_no = WTERMSIG(status);
    }
    return o;
}

} // namespace

// ---------------------------------------------------------------------------
// search_and_highlight — DEF-013 (CLOSED). Null-app and null-widget guards
// were added to search_and_highlight() and get_search_type() so the
// function returns cleanly when app is nullptr. This is now a positive-
// contract test identical to the enable_autocomplete and search_type_changed
// cases below.
// ---------------------------------------------------------------------------

TEST_CASE("Search: search_and_highlight(nullptr) returns cleanly "
          "[symmetric-guard contract — DEF-013]",
          "[layer4][interactive][search][vpr_gui][GUI-T-019]") {
    // DEF-013 — fixed: null guards added to search_bar.cpp.
    ChildOutcome o = run_in_child([]() {
        search_and_highlight(/*widget=*/nullptr, /*app=*/nullptr);
    });
    INFO("Child exit_code=" << o.exit_code << " signal=" << o.signal_no);
    REQUIRE(o.exited_cleanly);
    REQUIRE(o.signal_no == 0);
}

// ---------------------------------------------------------------------------
// enable_autocomplete — positive contract (no [!shouldfail]). Today returns
// cleanly because `app->find_line_edit(name)` iterates QApplication's widget
// list without dereferencing `this`, returns nullptr when the name is
// absent, and the existing `if (!searchBar) return;` catches it. The case
// pins this behaviour: a future change that adds an unconditional `app->`
// deref above the existing guard will flip this to FAIL.
// ---------------------------------------------------------------------------

TEST_CASE("Search: enable_autocomplete(nullptr) returns cleanly "
          "[symmetric-guard contract]",
          "[layer4][interactive][search][vpr_gui][GUI-T-019]") {
    ChildOutcome o = run_in_child([]() {
        enable_autocomplete(/*app=*/nullptr);
    });
    INFO("Child exit_code=" << o.exit_code << " signal=" << o.signal_no);
    REQUIRE(o.exited_cleanly);
    REQUIRE(o.signal_no == 0);
}

// ---------------------------------------------------------------------------
// search_type_changed — positive contract. Same reasoning as above; passes
// a real combo so the existing `if (!self) return;` and empty-text early-
// returns are bypassed and the function reaches the `app->find_line_edit`
// call that the contract is about.
// ---------------------------------------------------------------------------

TEST_CASE("Search: search_type_changed(combo, nullptr) returns cleanly "
          "[symmetric-guard contract]",
          "[layer4][interactive][search][vpr_gui][GUI-T-019]") {
    ChildOutcome o = run_in_child([]() {
        QComboBox combo;
        combo.addItem("Block Name");
        combo.setCurrentIndex(0);
        search_type_changed(&combo, /*app=*/nullptr);
    });
    INFO("Child exit_code=" << o.exit_code << " signal=" << o.signal_no);
    REQUIRE(o.exited_cleanly);
    REQUIRE(o.signal_no == 0);
}

// ---------------------------------------------------------------------------
// Positive-control case: confirm the fork harness itself works — a callable
// that returns cleanly must report exited_cleanly=true. Without this, a
// broken harness could mask a real guard regression by always reporting
// "crashed".
// ---------------------------------------------------------------------------

TEST_CASE("Search: fork harness reports clean exit for a no-op body "
          "[positive control]",
          "[layer4][interactive][search][vpr_gui][GUI-T-019]") {
    // The body intentionally constructs a std::string so gcc-13 cannot
    // prove the lambda is nothrow: a provably-nothrow lambda fed into
    // std::function<void()> trips -Werror=noexcept inside libstdc++'s
    // is_nothrow_invocable_r_v SFINAE chain. The std::string ctor
    // (allocator-throwing) breaks that inference and the assertion
    // below still validates the positive control.
    ChildOutcome o = run_in_child([]() { volatile std::string s("ok"); (void)s; });
    REQUIRE(o.exited_cleanly);
    REQUIRE(o.signal_no == 0);
    REQUIRE(o.exit_code == 0);
}

#endif // !_WIN32
