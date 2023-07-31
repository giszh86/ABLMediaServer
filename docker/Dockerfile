FROM ubuntu:20.04

WORKDIR /app
ADD ./bin /app
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends libssl-dev libasound2 libgtk-3-0 libxtst6 libpulse0 avahi-utils \
	&& apt-get clean && rm -rf /var/lib/apt/lists/
EXPOSE 8000
ENV  LD_LIBRARY_PATH=/app:$LD_LIBRARY_PATH
ENTRYPOINT ["./webrtc-streamer-linux.out"]

