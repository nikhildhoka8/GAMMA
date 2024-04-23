a.out:
	g++ -std=c++17 main.cpp -g -o project4 -lstdc++fs -fpermissive

run: a.out
	./project4
	
clean:
	rm project4
