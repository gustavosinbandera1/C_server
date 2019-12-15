# Use an official Debian 9 stable as a parent image
FROM ubuntu:18.04

# Make port 80 available to the world outside this container
EXPOSE 7681


ENV redisPort 6379
ENV redisHost "localhost"
# ENV comlinkPort 2579
# ENV comlinkHost localhost

#Installing server C websockets lwsws
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y vim gcc cmake git libssl-dev libev-dev pkg-config systemd-sysv zlib1g-dev libsqlite3-dev


RUN apt-get install git build-essential cmake libmicrohttpd-dev libcurl4-openssl-dev -y
RUN apt-get install libuv1-dev -y

RUN mkdir -p /usr/local/src
WORKDIR /usr/local/src/


#RUN git clone https://github.com/warmcat/libwebsockets.git libwebsockets
#cp -a ubuntu terminal
COPY libwebsockets /usr/local/src/libwebsockets 

WORKDIR /usr/local/src/libwebsockets
#RUN rm -r build
RUN mkdir build
WORKDIR /usr/local/src/libwebsockets/build

RUN cmake .. -DLWS_WITH_LWSWS=1 -DCMAKE_BUILD_TYPE=DEBUG
RUN make && make install
# install libs in /usr/local/lib ; configured in /etc/ld.so.conf.d/libc.conf
ENV LD_LIBRARY_PATH /usr/local/lib:${LD_LIBRARY_PATH}
RUN ldconfig -v 

RUN mkdir /etc/lwsws
RUN mkdir /etc/lwsws/conf.d
RUN mkdir /var/log/lwsws
RUN mkdir /usr/local/lib/systemd


# Copy the some configs
ADD ./lwsws_conf /etc/lwsws/conf
ADD ./lwsws_my_vhost /etc/lwsws/conf.d/test-server
ADD ./lwsws_systemd.service /usr/local/lib/systemd/lwsws.service
ADD ./lwsws_logrotate /etc/logrotate.d/lwsws
ADD ./check.html /usr/local/share/libwebsockets-test-server/check.html

RUN apt-get install redis-tools -y

#installing some libraries

#installing  C redis client library
################################################################
WORKDIR /usr/local/src
RUN git clone https://github.com/redis/hiredis.git
WORKDIR /usr/local/src/hiredis
RUN make && make install
RUN mkdir /usr/include/hiredis
RUN cp libhiredis.so /usr/lib/
RUN cp hiredis.h /usr/include/hiredis/
RUN cp read.h /usr/include/hiredis/
RUN cp sds.h /usr/include/hiredis/
RUN ldconfig
####################################################################
#compiling C redis client bash application
WORKDIR /usr/local/bin
COPY redisClient.c /usr/local/bin
RUN gcc -o redisClient redisClient.c -lhiredis


#compiling C httpClient.c   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
WORKDIR /usr/local/bin
COPY httpClient.c /usr/local/bin
RUN gcc -o httpClient httpClient.c




# Installing redis server, !!!!just for testing, this need to be removed  or commented
RUN apt-get install redis-server
RUN systemctl enable redis-server.service
RUN redis-server --daemonize yes


#########################compiling plugins##################################################
############################################################################################

#compiling plugin-standalone
WORKDIR /usr/local/src/libwebsockets/plugin-standalone
#RUN rm -r build
RUN mkdir build
WORKDIR /usr/local/src/libwebsockets/plugin-standalone/build
RUN cmake ..
RUN make
RUN make install

#compiling plugin lws_packet
##############################################################
WORKDIR /usr/local/src/libwebsockets/plugins/plugin_lws_packet
#RUN rm -r build
RUN mkdir build
WORKDIR /usr/local/src/libwebsockets/plugins/plugin_lws_packet/build
RUN cmake ..
RUN make
RUN make install
###############################################################

#compiling plugin lws_minimal
##############################################################
WORKDIR /usr/local/src/libwebsockets/plugins/plugin_lws_minimal
#RUN rm -r build
RUN mkdir build
WORKDIR /usr/local/src/libwebsockets/plugins/plugin_lws_minimal/build
RUN cmake ..
RUN make
RUN make install
###############################################################






###################NODEJS JUST FOR TEST AND PLAY WITH JAVASCRIPT CLIENT, BUT REALLY DOESNT NEEDED
#########################WE CAN PLAY WITH C CLIENT AS WELL

#Install nodejs environment
#####################################################################################
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

# Install base dependencies
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y -q --no-install-recommends \
        apt-transport-https \
        build-essential \
        ca-certificates \
        curl \
        git \
        libssl-dev \
        python \
        rsync \
        software-properties-common \
        devscripts \
        autoconf \
        ssl-cert \
    && apt-get clean

# update the repository sources list
# and install dependencies
RUN curl -sL https://deb.nodesource.com/setup_8.x | bash -
RUN apt-get install -y nodejs

# confirm installation
RUN node -v
RUN npm -v

# Use latest npm
RUN npm i npm@latest -g

############################################################################



##add websocketClient js for test
WORKDIR /usr/local/src
COPY nodejsWebSocketClient /usr/local/src/nodejsWebSocketClient
WORKDIR /usr/local/src/nodejsWebSocketClient
COPY /nodejsWebSocketClient/package*.json /usr/local/src/nodejsWebSocketClient
#WORKDIR /usr/local/src/nodejsWebSocketClient
RUN npm install




# RUN systemctl enable /usr/local/lib/systemd/lwsws.service
# RUN systemctl start lwsws.service

# Run webserver when the container launches
#CMD ["lwsws", ""]
CMD /usr/local/bin/shell.sh ; sleep infinity
WORKDIR /usr/local/src




######################HOW TO BUILD############################################
#build
#sudo docker build -t lwsws-container .

#run
# sudo docker run -p 7681:7681 -it lwsws-container

