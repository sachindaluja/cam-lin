CC = gcc
CXX = g++

TARGET = ../../lib/libvidcapture.a

default: $(TARGET)
$(TARGET): ../../include/v4l2.h v4l2.o
	ar -cq $@ v4l2.o
clean:
	-rm *.o $(TARGET)

# Rules
%.o: %.c
	$(CC) -g -c -I../../include -o $@ $<

%.o: %.cpp
	$(CXX) -g -c -I../../include -o $@ $<
