CXX = /usr/bin/g++
CFLAGS = -std=c++17
VERSION = 3.3.2
BLENDER = /usr/bin/blender
LIB_NAME = xml-mesh


all: lib/lib$(LIB_NAME).so.$(VERSION)


test: bin/visual data/dummy.xml
	bin/visual data/dummy.xml data/dummy.png run

clean:
	rm -f bin/visual obj/* lib/* data/dummy.xml core


data/dummy.xml: data/dummy.blend
	$(BLENDER) $< --background --python xml_exporter.py -- dummy $@


bin/visual: lib/lib$(LIB_NAME).so.$(VERSION) include/xml-mesh/mesh.h tests/visual.cpp
	$(CXX) $(CFLAGS) -I include/xml-mesh tests/visual.cpp lib/lib$(LIB_NAME).so.$(VERSION) -lboost_filesystem -lboost_system -lpng -lGL -lGLEW -lSDL2 -o $@


lib/lib$(LIB_NAME).so.$(VERSION): obj/parse.o obj/math.o obj/animate.o obj/error.o obj/build.o obj/access.o
	mkdir -p lib
	$(CXX) $^ -lxml2 -o $@ -shared -fPIC


obj/%.o: src/%.cpp include/xml-mesh/mesh.h
	mkdir -p obj
	$(CXX) $(CFLAGS) -I include/xml-mesh -c $< -o $@ -fPIC


install: lib/lib$(LIB_NAME).so.$(VERSION)
	/usr/bin/install -d -m755 /usr/local/lib
	/usr/bin/install -d -m755 /usr/local/include/xml-mesh
	/usr/bin/install -m644 lib/lib$(LIB_NAME).so.$(VERSION) /usr/local/lib/lib$(LIB_NAME).so.$(VERSION)
	ln -sf /usr/local/lib/lib$(LIB_NAME).so.$(VERSION) /usr/local/lib/lib$(LIB_NAME).so
	/usr/bin/install -D include/xml-mesh/*.h /usr/local/include/xml-mesh/

uninstall:
	rm -f /usr/local/lib/lib$(LIB_NAME).so* /usr/local/include/xml-mesh/*.h
