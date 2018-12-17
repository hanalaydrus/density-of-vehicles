FROM mariadb:10.3

ENV MYSQL_DATABASE=density

COPY ./density.sql /docker-entrypoint-initdb.d/

RUN sed -i '/\[mysqld\]/a server-id=1\nlog-bin' /etc/mysql/my.cnf
