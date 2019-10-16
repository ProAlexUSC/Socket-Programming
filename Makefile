all:
	g++ -std=c++17 -o aws aws.cpp
	g++ -std=c++17 -o client client.cpp
	g++ -std=c++17 -o serverA serverA.cpp

aws:
	./aws