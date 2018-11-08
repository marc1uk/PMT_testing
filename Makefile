CXXFLAGS    = -g -std=c++0x -Wall -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable

all: main

main: include/Camac lib/Camac
	g++ $(CXXFLAGS) src/main.cpp -o main -I include -L lib -lCC -lm -lxx_usb -lJ8 -lL5 #-lL3 -lL4 -lm -lxx_usb

include/Camac:
	@cp UserTools/camacinc/CamacCrate/CamacCrate.h include/
	@cp UserTools/camacinc/Jorway85A/Jorway85A.h include/
#	@cp UserTools/camacinc/Lecroy3377/Lecroy3377.h include/
#	@cp UserTools/camacinc/Lecroy4300b/Lecroy4300b.h include/
	@cp UserTools/camacinc/Lecroy4413/Lecroy4413.h include/
	@cp UserTools/camacinc/XXUSB/libxxusb.h include/
	@cp UserTools/camacinc/XXUSB/usb.h include/

lib/Camac:
	@cp UserTools/camacinc/makelib/libxx_usb.so lib/
	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/CamacCrate/CamacCrate.cpp -I include -L lib -o lib/libCC.so
#	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/Lecroy3377/Lecroy3377.cpp -I include -L lib -o lib/libL3.so
#	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/Lecroy4300b/Lecroy4300b.cpp -I include -L lib -o lib/libL4.so
	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/Lecroy4413/Lecroy4413.cpp -I include -L lib -o lib/libL5.so
	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/Jorway85A/Jorway85A.cpp -I include -L lib -o lib/libJ8.so
