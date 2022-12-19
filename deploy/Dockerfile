FROM ubuntu:22.04

RUN apt-get update && apt-get install -y xz-utils python-is-python3 gcc g++ wget git libpq-dev libmysqlclient-dev

RUN git clone https://github.com/sazikov-ad/networks /sources

RUN wget https://github.com/bazelbuild/bazel/releases/download/4.0.0/bazel-4.0.0-linux-x86_64 -O /bazel_binary && \
    chmod +x /bazel_binary

RUN cd /sources && \
    /bazel_binary build //...

RUN mkdir /out && \
    cp /sources/bazel-bin/pkg/backend.tar.xz /out/backend.tar.xz

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y xz-utils libpq-dev libmysqlclient-dev
WORKDIR /backend
COPY --from=0 /out/backend.tar.xz ./
ADD pg_config.ini .
ADD server_config.ini .
RUN tar -xf backend.tar.xz && rm backend.tar.xz
RUN mkdir -p /backend/work/logs && ln -sf /dev/stderr /backend/work/logs/server.log
CMD ["/backend/usr/bin/chat.server", "--config", "/backend/server_config.ini"]
