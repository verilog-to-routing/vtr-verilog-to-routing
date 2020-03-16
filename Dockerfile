FROM ubuntu:bionic

ENV WORKSPACE=/workspace

RUN apt-get update && \
 apt-get install --no-install-recommends --yes \
	sudo \
	build-essential \
	flex \
	bison \
	ccache \
	ninja \
	cmake \
	fontconfig \
	libcairo2-dev \
	libfontconfig1-dev \
	libx11-dev \
	libxft-dev \
	libgtk-3-dev \
	perl \
	liblist-moreutils-perl \
	python \
	time \
	git \
	valgrind \
	gdb \
	ctags \
	vim \
	gcc-7 \
	libasan4  && \
 apt-get autoremove -y && \
 apt-get clean && \
 rm -rf /var/lib/apt/lists/*

RUN mkdir -p ${WORKSPACE}
VOLUME ${WORKSPACE}

ENTRYPOINT [ "/bin/bash" ]
