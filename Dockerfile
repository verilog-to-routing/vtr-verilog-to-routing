FROM fedora:latest

RUN dnf -y update '*' --refresh
# ------------------------------------------------------------------------------
# Install base
RUN dnf install -y glibc-static tmux python libevent-devel ncurses-devel clang perl-List-MoreUtils time supervisor @development-tools zip clang curl git libxml++-devel libX11-devel libXft-devel fontconfig cairo-devel automake cmake flex bison ctags gdb perl valgrind
RUN sed -i 's/^\(\[supervisord\]\)$/\1\nnodaemon=true/' /etc/supervisord.conf

# ------------------------------------------------------------------------------
# Install Node.js
RUN curl -L https://raw.githubusercontent.com/c9/install/master/install.sh | bash
RUN dnf install -y nodejs

# ------------------------------------------------------------------------------
# Install Cloud9
RUN git clone https://github.com/c9/core.git /cloud9
RUN /cloud9/scripts/install-sdk.sh

# Tweak standlone.js conf
RUN sed -i -e 's_127.0.0.1_0.0.0.0_g' /cloud9/configs/standalone.js

# ------------------------------------------------------------------------------
# Add volumes
RUN mkdir /workspace
VOLUME /workspace

RUN dnf clean all

RUN ln -s /lib64 /usr/lib/x86_64-linux-gnu

RUN echo "#!/bin/sh" >> /startup.sh
RUN echo "export CC=clang" >> /startup.sh
RUN echo "export CXX=clang++" >> /startup.sh
RUN echo "if [ -z \"\$PASS\" ]; then" >> /startup.sh
RUN echo "  node /cloud9/server.js --listen 0.0.0.0 --port 8080 -w /workspace --collab -a root:letmein" >> /startup.sh
RUN echo "else" >> /startup.sh
RUN echo "  node /cloud9/server.js --listen 0.0.0.0 --port 8080 -w /workspace --collab -a root:\$PASS" >> /startup.sh
RUN echo "fi" >> /startup.sh

RUN chmod +x /startup.sh

EXPOSE 22
EXPOSE 8080


CMD ["/bin/sh","/startup.sh"]
