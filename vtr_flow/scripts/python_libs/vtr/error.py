"""
    Module to handle VTR flow erroring.
"""


class VtrError(Exception):
    """
    Base class for VTR related exceptions

    Attributes:
        msg -- An explanation of the error
    """

    def __init__(self, *msg):
        self.msg = " ".join(str(item) for item in msg)
        super().__init__()


class CommandError(VtrError):
    """
    Raised when an external command failed.

    Attributes:
        returncode -- The return code from the command
        cmd == The command run
    """

    def __init__(self, *msg, cmd, returncode, log=None):
        super().__init__(*msg)
        self.returncode = returncode
        self.cmd = cmd
        self.log = log


class InspectError(VtrError):
    """
    Raised when some query (inspection) result is not found.
    """

    def __init__(self, *msg, filename=None):
        super().__init__(*msg)
        self.filename = filename
