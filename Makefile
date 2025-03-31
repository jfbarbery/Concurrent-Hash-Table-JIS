chash: chash.c # Add other source code files here
	@gcc chash.c -o chash

chash.c:
	@echo "Do you seriously not have the source code!?!?"
	
test: chash.c # A testing target that will also run the compiled source code, and clean up when done
	@gcc chash.c -o testchash && ./testchash; rm -f testchash
	
clean:
	rm -f chash # Add other output files here