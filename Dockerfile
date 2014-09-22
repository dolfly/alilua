# This is for Ubuntu

FROM ubuntu:14.04

MAINTAINER Dolfly <dolfly@foxmail.com>

RUN apt-get -qq update 

RUN apt-get -qqy install libssl-dev

RUN git clone https://github.com/dolfly/alilua.git 

RUN cd alilua

RUN make install clean

RUN alilua --daemon --bind=8080
