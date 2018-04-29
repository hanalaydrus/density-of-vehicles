build:
	protoc -I . --cpp_out=. densityContract.proto
	g++ -std=c++11 `pkg-config --cflags protobuf grpc` -c -o densityContract.pb.o densityContract.pb.cc
	protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` densityContract.proto
	g++ -std=c++11 `pkg-config --cflags protobuf grpc` -c -o densityContract.grpc.pb.o densityContract.grpc.pb.cc
	g++ -std=c++11 `pkg-config --cflags protobuf grpc opencv` -c -o main.o main.cc
	g++ -std=c++11 `pkg-config --cflags opencv` -c -o IPM.o IPM.cc
	g++ -std=c++11 -c -o Model.o Model.cc
	g++ densityContract.pb.o densityContract.grpc.pb.o main.o IPM.o Model.o -o main -lmysqlcppconn -lcurl `pkg-config --libs protobuf grpc++ grpc opencv` -ldl

run:
	./main

clean:
	rm -f *.o *.pb.cc *.pb.h main

build_db_image:
	docker build -t asia.gcr.io/tugas-akhir-hana/mariadb-density:latest ./mariadb/

# if no trigger at google container registry
# build_service_image:
# 	docker build -t asia.gcr.io/tugas-akhir-hana/density-of-vehicle:latest .

push_gcp_image:
	docker push asia.gcr.io/tugas-akhir-hana/mariadb-density:latest
	# docker push asia.gcr.io/tugas-akhir-hana/density-of-vehicle:latest