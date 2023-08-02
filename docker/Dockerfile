FROM ubuntu:20.04

EXPOSE 1935/tcp
EXPOSE 554/tcp
EXPOSE 80/tcp
EXPOSE 443/tcp
EXPOSE 5088/udp
EXPOSE 5088/tcp
EXPOSE 6088/udp
EXPOSE 6088/tcp
EXPOSE 7088/udp
EXPOSE 7088/tcp
EXPOSE 8088/udp
EXPOSE 8088/tcp
EXPOSE 9088/udp
EXPOSE 9088/tcp

RUN mkdir -p /opt/media

COPY ./bin /opt/media/ABLMediaServer

WORKDIR /opt/media/ABLMediaServer

RUN chmod +x ABLMediaServer 

ENV  LD_LIBRARY_PATH=/opt/media/ABLMediaServer:$LD_LIBRARY_PATH

ENTRYPOINT ["./ABLMediaServer"]

