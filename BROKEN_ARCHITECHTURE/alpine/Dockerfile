FROM alpine:latest

# ------------------------------------------------------------------------------
# Install base

RUN apk update && \ 
    apk upgrade

#fetching tools    
RUN apk add --no-cache curl wget ca-certificates openssl-dev apache2-utils

#compile tool
RUN apk add --no-cache build-base bash make cmake automake gcc g++ clang llvm libgcc libstdc++

#perl tool
RUN apk add --no-cache perl-dev perl-list-moreutils 

#vtr- requirement
RUN apk add --no-cache git zip flex bison ctags libxml2-dev

#debug tools
RUN apk add --no-cache gdb valgrind

#cloud9 runs on nodejs
RUN apk add --no-cache nodejs nodejs-npm tmux
# ------------------------------------------------------------------------------
# set clang as default compiler because it is more verbose and has a static analyser
RUN export CC=clang
RUN export CXX=clang++

# ------------------------------------------------------------------------------
# Install Cloud9
RUN git clone https://github.com/c9/core.git /cloud9
RUN curl -s -L https://raw.githubusercontent.com/c9/install/master/link.sh | bash
RUN /cloud9/scripts/install-sdk.sh

# Tweak standlone.js conf
RUN sed -i -e 's_127.0.0.1_0.0.0.0_g' /cloud9/configs/standalone.js

# Add volumes
RUN mkdir /workspace
VOLUME /workspace

EXPOSE 8080

CMD ["node", "/cloud9/server.js", "--listen", "0.0.0.0", "--port", "8080", "-w", "/workspace", "--collab", "-a", "admin:logmein"]
