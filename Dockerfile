FROM alpine:latest
COPY ./main /main
RUN chmod +x /main
CMD ["./main"]