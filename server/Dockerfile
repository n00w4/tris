FROM gcc:14.2.0 AS build

WORKDIR /app

COPY . .

WORKDIR /app/server
RUN make clean && make

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y libc6 && rm -rf /var/lib/apt/lists/*

COPY --from=build /app/server/tris_server /usr/local/bin/tris_server

EXPOSE 8080

CMD ["tris_server"]
