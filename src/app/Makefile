CC = gcc
CXX = g++
PROJNAME = app
TARGETNAME = $(PROJNAME)
TARGET = ../../bin/$(TARGETNAME)

default: $(TARGET)
$(TARGET): main-win.h main-win.o main.o ../../include/v4l2.h capture-draw-area.h capture-draw-area.o ../../lib/libvidcapture.a
	$(CXX) -g main-win.o main.o capture-draw-area.o -L../../lib -lvidcapture `pkg-config gtkmm-3.0 --libs` -lv4l2 -o $@
clean:
	-rm *.o $(TARGET)

# Rules
%.o: %.c
	$(CC) -g -c `pkg-config gtk+-3.0 --cflags` -I../../include -o $@ $<

%.o: %.cpp
	$(CXX) -g -c -std=c++11 `pkg-config gtkmm-3.0 --cflags` -I../../include -o $@ $<
