#!/usr/bin/env python3

import argparse
import datetime
import getpass
import subprocess
import sys
import tempfile



if __name__ == "__main__":
    parser = argparse.ArgumentParser("Automated script for applying code reformatting. Automatically commits reformatting changes as 'VTR Robot' and adds them to .git-blame-ignore-revs (so they can be skipped with git hyper-blame). Should be run from the root of the source tree.")
    parser.add_argument("--skip_commit", action='store_true', default=False, help="Don't commit the autoformat changes. Default: %(default)s")
    args = parser.parse_args()

    git_commit_args = [
        '-a',
        '--no-verify',
        '--author="VTR Robot <robot@verilogtorouting.org>"',
    ]

    # Run `make format`
    subprocess.check_call("make format", shell=True)

    if not args.skip_commit:
        # Commit the changes caused by `make format`
        with tempfile.NamedTemporaryFile("w") as msg:
            msg.write("""\
🤖 - Automated code reformat.

I'm an auto code reformatting bot. Beep boop...
            """)
            msg.flush()

            subprocess.check_call("git commit {} --file={msg}".format(' '.join(git_commit_args), msg=msg.name),
                                  shell=True)

        # Append the format change to the .git-blame-ignore-revs file.
        format_hash = subprocess.check_output(
            "git rev-parse --verify HEAD", shell=True)
        format_hash = format_hash.decode("utf-8").strip()

        now = datetime.datetime.utcnow().isoformat()

        with open(".git-blame-ignore-revs", "a+") as bf:
            bf.write("""\
# Autoformat run on {}
{}
            """.format(now, format_hash))

        subprocess.check_call("git add .git-blame-ignore-revs", shell=True)

        with tempfile.NamedTemporaryFile("w") as msg:
            msg.write("""\
🤖 - Ignore automated code reformat in blame.

Adding the reformatting change
{}
(which I just committed!) to the ignore list.

I'm an auto code reformatting bot. Beep boop...
            """.format(format_hash))
            msg.flush()

            subprocess.check_call("git commit {} --file={msg}".format(' '.join(git_commit_args), msg=msg.name),
                                  shell=True)

