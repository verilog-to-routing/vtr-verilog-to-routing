FROM gliderlabs/alpine:latest

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
RUN apk add --no-cache git zip flex bison ctags libxml2-dev cairo-dev libxft-dev freetype-dev libx11-dev

#debug tools
RUN apk add --no-cache gdb valgrind

#cloud9 runs on nodejs
RUN apk add --no-cache nodejs nodejs-npm tmux supervisor
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

RUN touch /etc/supervisor.conf

RUN echo '[supervisord]'  >> /etc/supervisor.conf
RUN echo 'nodaemon=true'  >> /etc/supervisor.conf
RUN echo 'pidfile=/var/run/supervisord.pid'  >> /etc/supervisor.conf
RUN echo 'logfile=/var/log/supervisor/supervisord.log'  >> /etc/supervisor.conf

RUN echo '[program:cloud9]' >> /etc/supervisor.conf
RUN echo 'command = node /cloud9/server.js --listen 0.0.0.0 --port 8080 -w /workspace --collab -a admin:logmein' >> /etc/supervisor.conf
RUN echo 'directory = /cloud9' >> /etc/supervisor.conf
RUN echo 'user = root' >> /etc/supervisor.conf
RUN echo 'autostart = true' >> /etc/supervisor.conf
RUN echo 'autorestart = true' >> /etc/supervisor.conf
RUN echo 'stdout_logfile = /var/log/supervisor/cloud9.log' >> /etc/supervisor.conf
RUN echo 'stderr_logfile = /var/log/supervisor/cloud9_errors.log' >> /etc/supervisor.conf
RUN echo 'environment = NODE_ENV="production"' >> /etc/supervisor.conf

# Add volumes
RUN mkdir /workspace && mkdir -p /var/log/supervisor
VOLUME /workspace

EXPOSE 8080

CMD ["supervisord", "-c", "/etc/supervisor.conf"]