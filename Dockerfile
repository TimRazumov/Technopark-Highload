FROM ubuntu:18.04

RUN apt-get update
RUN apt-get install -y --no-install-recommends apt-utils

RUN apt-get -y install python3-pip
RUN pip3 install cmake
RUN apt-get -y install g++

RUN apt-get -y install wget
RUN apt-get -y install tar
RUN wget -O boost_1_74_0.tar.gz http://sourceforge.net/projects/boost/files/boost/1.74.0/boost_1_74_0.tar.gz/download
RUN tar -xzvf boost_1_74_0.tar.gz && cd boost_1_74_0 && ./bootstrap.sh && ./b2 install

COPY . .

RUN rm -rf build && mkdir build && cd build && cmake .. && make -j4 && cd ..

EXPOSE 80

CMD ./build/bin/static_server --config files/static_server.conf