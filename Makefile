CPP = g++
CPPFLAGS  = -g -Wall -std=c++17
all:
	g++ -std=c++17 -o aws aws.cpp
	g++ -std=c++17 -o client client.cpp
	g++ -std=c++17 -o serverA serverA.cpp
	g++ -std=c++17 -o serverB serverB.cpp
clean:
	rm -f client aws serverA serverB
.PHONY: serverA
serverA: serverA
	./serverA

.PHONY: serverB
serverB: serverB
	./serverB

.PHONY: aws
aws: aws
	./aws