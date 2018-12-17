FROM mariadb:10.3

ENV MYSQL_DATABASE=density

COPY ./density.sql /docker-entrypoint-initdb.d/

RUN RAND="$(date +%s | rev | cut -c 1-2)$(echo ${RANDOM})" && sed -i '/\[mysqld\]/a server-id='$RAND'\nlog-bin' /etc/mysql/my.cnf
