CXXFLAGS    = -g -std=c++0x -Wall -Wno-reorder -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable #-fdiagnostics-color=always

CPPFLAGS = `root-config --cflags`
LDFLAGS = `root-config --libs`

ZMQInclude = -I../zeromq-4.0.7-master/include
ZMQLib = -L../zeromq-4.0.7-master/lib -lzmq

#ZMQInclude = -I/home/marc/LinuxSystemFiles/ToolAnalysis/ToolAnalysis/ToolDAQ/zeromq-4.0.7/include
#ZMQLib = -L/home/marc/LinuxSystemFiles/ToolAnalysis/ToolAnalysis/ToolDAQ/zeromq-4.0.7/lib -lzmq

all: main

main: include/Camac lib/Camac include/HV lib/HV
	g++ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) src/main.cpp -o main -I include -I/home/annie_local/Downloads/ $(ZMQInclude) $(ZMQLib) -Llib -L/usr/lib/x86_64-linux-gnu -lCC -lm -lxx_usb -lJ8 -lL4 -lL5 -lC1 -lX11 -lpthread -lXtst -lHV #-lL3 -lxx_usb

include/Camac:
	@cp UserTools/camacinc/CamacCrate/CamacCrate.h include/
	@cp UserTools/camacinc/Jorway85A/Jorway85A.h include/
#	@cp UserTools/camacinc/CAENC117B/CAENC117B.h include/
	@cp UserTools/camacinc/CAENC117B/my_Caen_C117B.h include/
#	@cp UserTools/camacinc/Lecroy3377/Lecroy3377.h include/
	@cp UserTools/camacinc/Lecroy4300b/Lecroy4300b.h include/
	@cp UserTools/camacinc/Lecroy4413/Lecroy4413.h include/
	@cp UserTools/camacinc/XXUSB/libxxusb.h include/
	@cp UserTools/camacinc/XXUSB/usb.h include/

include/HV:
	@cp UserTools/ZMQ_HV_Control/PMTTestingHVcontrol.h include/

lib/HV:
	g++ $(CXXFLAGS) -shared -fPIC UserTools/ZMQ_HV_Control/PMTTestingHVcontrol.cpp -I include $(ZMQInclude) $(ZMQLib) -o lib/libHV.so

lib/Camac:
#	@cp UserTools/camacinc/makelib/libxx_usb.so lib/
#	@cp UserTools/camacinc/XXUSB/libxx_usb.so lib/
	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/CamacCrate/CamacCrate.cpp -I include -L lib -o lib/libCC.so
#	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/Lecroy3377/Lecroy3377.cpp -I include -L lib -o lib/libL3.so
	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/Lecroy4300b/Lecroy4300b.cpp -I include -L lib -o lib/libL4.so
	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/Lecroy4413/Lecroy4413.cpp -I include -L lib -o lib/libL5.so
	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/Jorway85A/Jorway85A.cpp -I include -L lib -o lib/libJ8.so
#	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/CAENC117B/CAENC117B.cpp -I include -L lib -o lib/libC1.so
	g++ $(CXXFLAGS) -shared -fPIC UserTools/camacinc/CAENC117B/my_Caen_C117B.cpp -I include -L lib -o lib/libC1.so

wienertest: include/Camac lib/Camac
	g++ $(CXXFLAGS) src/wiener_two_nim_test.cpp -o wienertest -I include -Llib -lCC -lm -lxx_usb -lpthread

plotwaveforms:
	g++ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) src/plot_wavedump.cpp -o plot_wavedump -I include -Llib -lpthread

fixkeys:
	g++ $(CXXFLAGS) src/restore_key_repeat.cpp -o fixkeys -lX11 -lXtst

caentest: include/Camac lib/Camac
	g++ $(CXXFLAGS) src/caennet_test.cpp -o caennet_test -I include -Llib -lCC -lm -lxx_usb -lJ8 -lL4 -lL5 -lC1 -lpthread #-lL3 -lxx_usb
