FROM ubuntu:16.04

RUN apt-get update && \
    apt-get install -y \
            build-essential \
            autoconf \
            libtool \
            checkinstall \
            cmake \
            pkg-config \
            yasm \
            git \
            libjasper-dev \
            libpng12-dev \
            libtiff5-dev\
            libavcodec-dev \
            libavformat-dev \
            libswscale-dev \
            libdc1394-22-dev \
            libxine2-dev \
            libv4l-dev \
            libgstreamer0.10-dev \
            libgstreamer-plugins-base0.10-dev \
            qt5-default \
            libgtk2.0-dev \
            libatlas-base-dev \
            libmp3lame-dev \
            libtheora-dev \
            libvorbis-dev \
            libxvidcore-dev \
            libopencore-amrnb-dev \
            libopencore-amrwb-dev \
            libmysqlcppconn-dev \
            libmysqlclient-dev \
            libboost-all-dev \
            libcurl4-openssl-dev \
            x264 v4l-utils

WORKDIR /
RUN git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc && \
    cd grpc && \
    git submodule update --init && \
    make && \
    make install

RUN cd grpc/third_party/protobuf && \
    make install

RUN git clone https://github.com/opencv/opencv.git && \
    cd opencv && \
    git checkout 3.3.1

RUN git clone https://github.com/opencv/opencv_contrib.git && \
    cd opencv_contrib && \
    git checkout 3.3.1

RUN cd opencv && \
    mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=RELEASE \
        -D CMAKE_INSTALL_PREFIX=/usr/local \
        -D INSTALL_C_EXAMPLES=ON \
        -D INSTALL_PYTHON_EXAMPLES=ON \
        -D WITH_TBB=ON \
        -D WITH_V4L=ON \
        -D WITH_QT=ON \
        -D WITH_OPENGL=ON \
        -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
        -D BUILD_EXAMPLES=ON .. && \
    make -j4 && \
    make install && \
    sh -c 'echo "/usr/local/lib" >> /etc/ld.so.conf.d/opencv.conf' && \
    ldconfig

RUN rm -rf opencv && \
    rm -rf opencv && \
    rm -rf opencv_contrib && \
    rm -rf grpc

COPY . /app
WORKDIR /app

RUN make build

CMD ["make","run"]