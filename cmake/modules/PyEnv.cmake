# Create a python virtual environment using venv and install all necessary packages
# wheel should be installed before the requirements.txt list to prevent errors.
add_custom_target(env
    COMMAND python3 -m venv ../.venv
    COMMAND . ../.venv/bin/activate && pip3 install wheel
    COMMAND . ../.venv/bin/activate && pip3 install -r ../requirements.txt
)