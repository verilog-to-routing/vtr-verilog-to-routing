import sys


class termcolor:
    PURPLE = "\033[95m"
    BLUE = "\033[94m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    END = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


def error(*msg, exitcode=-1):
    print(termcolor.RED + "ERROR:", " ".join(str(item) for item in msg), termcolor.END)
    sys.exit(exitcode)
