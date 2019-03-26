#!/usr/bin/env python3

import subprocess


def check_clean():
    clean = subprocess.Popen(
        "git status -s".split(),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    stdout, _ = clean.communicate()
    stdout = stdout.decode('utf-8', errors='replace')
    clean.wait()
    return clean.returncode, stdout


def main(args):
    # Check that git tree is currently clean.
    rc, stdout = check_clean()
    if rc != 0 or len(stdout) > 0:
        print("""
{}
Current working tree was not clean! This tool only works on clean checkouts.
""".format(stdout))
        return -1

    # Run the code formatting task
    subprocess.check_call("make format".split())

    # Check that git tree is still currently clean.
    rc, stdout = check_clean()
    if rc != 0 or len(stdout) > 0:
        files = []
        for l in stdout.splitlines():
            l = l.strip()
            if not l:
                continue
            _, fname = l.split(' ', maxsplit=1)
            files.append(fname)
        formatting = {
            "yellow":   "\u001b[33m",
            "red":      "\u001b[31m",
            "reset":    "\u001b[0m",
            "cyan":    "\u001b[36;1m",
            "bold":     "\u001b[1m",
        }
        print("""

{yellow}Code Formatting Check{reset}
==========================================

You {red}{bold}must{reset} make the following changes to match the formatting style;
------------------------------------------

""".format(**formatting))
        subprocess.call("git diff".split())
        print("""
==========================================

The following files {red}{bold}must{reset} be reformatted!

 - {}

Run "{cyan}make format{reset}"

""".format("\n - ".join(files), **formatting))

        subprocess.call("git reset --hard".split())
        return -1



if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv))
