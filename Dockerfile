# This is for Ubuntu

FROM ubuntu:14.04

MAINTAINER Dolfly <dolfly@foxmail.com>

RUN apt-get -qq update 

RUN apt-get -qqy install libssl-dev

RUN apt-get -qqy install git make gcc

RUN git clone https://github.com/dolfly/alilua.git 

RUN [ ! -d /alilua ] && mkdir -p /alilua

COPY ./* /alilua/

RUN cd alilua && make all 

CMD ['/alilua/alilua' '--daemon' '--bind=8080']
