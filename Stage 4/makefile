all: drinks_bar atom_supplier molecule_requester

drinks_bar: drinks_bar.cpp
	g++ -o drinks_bar drinks_bar.cpp

atom_supplier: atom_supplier.cpp
	g++ -o atom_supplier atom_supplier.cpp

molecule_requester: molecule_requester.cpp
	g++ -o molecule_requester molecule_requester.cpp


#IF didn't work sudo lsof -i :5555
clean-ports:
	sudo fuser -k 5555/tcp || true
	sudo fuser -k 6666/udp || true

clean:
	rm -f drinks_bar atom_supplier molecule_requester
	