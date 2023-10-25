# FPT’23 Artifact Evaluation

********The below commands are executed sequentially in the order presented (top to bottom)********

## Clone Repo

```bash
# From home directory
cd ~
git clone https://github.com/verilog-to-routing/vtr-verilog-to-routing.git
cd vtr-verilog-to-routing
git fetch -a
git checkout fpt_23_artifact
```

## Tools Setup

```bash
./install_apt_packages.sh
pip3 install -r requirements.txt
make vpr -j4
```