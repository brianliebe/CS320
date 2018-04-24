all:
	g++ cache-sim.cpp -o cache-sim -std=c++11

test:	all
	./cache-sim students/trace1.txt out.txt

clean:
	rm cache-sim
