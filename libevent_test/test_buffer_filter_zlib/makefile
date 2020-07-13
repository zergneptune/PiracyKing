test_buffer_filter_zlib: test_buffer_filter_zlib.cpp zlib_server.cpp zlib_client.cpp
	g++ -g $^ -o $@ -levent -lz -std=c++11

.PHONY: clean
clean:
	rm test_buffer_filter_zlib
