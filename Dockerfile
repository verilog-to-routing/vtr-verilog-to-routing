FROM fedora:latest
# ------------------------------------------------------------------------------
# Install base
RUN dnf -y update '*' --refresh && \
dnf install -y glibc-static tmux python libevent-devel ncurses-devel clang perl-List-MoreUtils \
time @development-tools zip clang curl git libxml++-devel libX11-devel libXft-devel fontconfig \
cairo-devel automake cmake flex bison ctags gdb perl valgrind

RUN ln -s /lib64 /usr/lib/x86_64-linux-gnu

# Install Node.js
RUN curl -L https://raw.githubusercontent.com/c9/install/master/install.sh | bash
RUN dnf install -y nodejs

# Install Cloud9
RUN git clone https://github.com/c9/core.git /cloud9
RUN /cloud9/scripts/install-sdk.sh
RUN sed -i -e 's_127.0.0.1_0.0.0.0_g' /cloud9/configs/standalone.js

#cleanup
RUN dnf clean all

# boot script
RUN echo "#!/bin/sh" >> /startup.sh
RUN echo "export CC=clang" >> /startup.sh
RUN echo "export CXX=clang++" >> /startup.sh
RUN echo "node /cloud9/server.js --listen 0.0.0.0 --port 8080 -w /workspace --collab -a root:letmein" >> /startup.sh
RUN chmod +x /startup.sh

# expose port and directory
RUN mkdir /workspace
VOLUME /workspace
EXPOSE 8080

ENTRYPOINT ["/bin/sh","/startup.sh"]
