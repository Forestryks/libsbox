FROM ubuntu:19.10

# build with `sudo docker image build -t libsbox .`
# run with `sudo docker run --rm --privileged -i libsbox`

RUN apt-get update && \
    apt-get -y install git build-essential cmake bsdmainutils && \
    rm -rf /var/lib/apt/lists/*

COPY . /libsbox
WORKDIR /libsbox

RUN mkdir -p build && cd build && rm -rf * && \
    cmake .. && \
    make && yes | make install

ENTRYPOINT ["libsboxd"]

