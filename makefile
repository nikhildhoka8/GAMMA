a.out:
	g++ -std=c++17 main.cpp -g -o project3 -lstdc++fs -fpermissive

run: a.out
	./project3
	
clean:
	rm project3
