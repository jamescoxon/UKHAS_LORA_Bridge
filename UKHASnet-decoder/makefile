SHELL = /bin/sh
CC    = gcc
 
TARGET  = UKHASnet-decoder
SOURCES = UKHASnet-decoder.c
 
all: $(TARGET)
	
$(TARGET): $(SOURCES)
	$(CC) -std=gnu99 -o $(TARGET) $(SOURCES) -lcurl

$(TARGET).exe: $(SOURCES)
	i686-w64-mingw32-gcc -std=gnu99 -o $@ $^ \
		-DCURL_STATICLIB -static \
		-I$(HOME)/mingw/curl/include -L$(HOME)/mingw/curl/lib \
		-L$(HOME)/mingw/openssl/lib \
		-L$(HOME)/mingw/zlib/lib \
		-lcurl -lssl -lcrypto -lz -lws2_32 -lgdi32

clean:
	-rm -f $(TARGET) 
	-rm -f $(TARGET).exe
