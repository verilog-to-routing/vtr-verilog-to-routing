FROM ubuntu:xenial

RUN apt-get update
RUN apt-get upgrade -y
# ------------------------------------------------------------------------------
# Install base
RUN apt-get install -y clang liblist-moreutils-perl time supervisor build-essential g++ zip clang curl libssl-dev apache2-utils git libxml2-dev libx11-dev libxft-dev fontconfig libcairo2-dev gcc automake git cmake flex bison ctags gdb perl valgrind 
RUN sed -i 's/^\(\[supervisord\]\)$/\1\nnodaemon=true/' /etc/supervisor/supervisord.conf

# ------------------------------------------------------------------------------
# set clang as default compiler because it is more verbose and has a static analyser
RUN export CC=clang
RUN export CXX=clang++

# ------------------------------------------------------------------------------
# Install Node.js
RUN curl -L https://raw.githubusercontent.com/c9/install/master/install.sh | bash
RUN apt-get install -y nodejs

# ------------------------------------------------------------------------------
# Install Cloud9
RUN git clone https://github.com/c9/core.git /cloud9
RUN /cloud9/scripts/install-sdk.sh

# Tweak standlone.js conf
RUN sed -i -e 's_127.0.0.1_0.0.0.0_g' /cloud9/configs/standalone.js

RUN touch /etc/supervisor/conf.d/cloud9.conf

RUN echo '[program:cloud9]' >> /etc/supervisor/conf.d/cloud9.conf
RUN echo 'command = nodejs /cloud9/server.js --listen 0.0.0.0 --port 8080 -w /workspace --collab' >> /etc/supervisor/conf.d/cloud9.conf
RUN echo 'directory = /cloud9' >> /etc/supervisor/conf.d/cloud9.conf
RUN echo 'user = root' >> /etc/supervisor/conf.d/cloud9.conf
RUN echo 'autostart = true' >> /etc/supervisor/conf.d/cloud9.conf
RUN echo 'autorestart = true' >> /etc/supervisor/conf.d/cloud9.conf
RUN echo 'stdout_logfile = /var/log/supervisor/cloud9.log' >> /etc/supervisor/conf.d/cloud9.conf
RUN echo 'stderr_logfile = /var/log/supervisor/cloud9_errors.log' >> /etc/supervisor/conf.d/cloud9.conf
RUN echo 'environment = NODE_ENV="production"' >> /etc/supervisor/conf.d/cloud9.conf

# ------------------------------------------------------------------------------
# Add volumes
RUN mkdir /workspace
VOLUME /workspace

# ------------------------------------------------------------------------------
# Clean up APT when done.
RUN apt-get clean && apt-get autoremove && rm -rf /var/lib/apt/lists/*

EXPOSE 22
EXPOSE 8080

CMD ["supervisord", "-c", "/etc/supervisor/supervisord.conf"]
