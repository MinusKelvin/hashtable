TESTS := basic.test collisions.test odd-capacity.test
CC := gcc

test: $(TESTS)

%.test: test/%.c hashtable.h
	@mkdir -p bin
	$(CC) -o bin/$@ $<
	bin/$@
