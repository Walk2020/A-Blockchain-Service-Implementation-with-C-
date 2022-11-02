make all: clientA.cpp clientB.cpp serverM.cpp serverA.cpp serverB.cpp serverC.cpp
	g++ -o clientA clientA.cpp

	g++ -o clientB clientB.cpp

	g++ -o serverM serverM.cpp
	
	g++ -o serverA serverA.cpp

	g++ -o serverB serverB.cpp	

	g++ -o serverC serverC.cpp


