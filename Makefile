# Name of the executable
EXEC = cppaudiocapture

# Include PortAudio and FFTW headers and library archives
CLIB = -I./lib/portaudio/include ./lib/portaudio/lib/.libs/libportaudio.a \
	-lrt -lasound -ljack -pthread -I./lib/fftw-3.3.10/api -lfftw3

$(EXEC): main.cpp
	g++ -o $@ $^ $(CLIB)

install-deps: install-portaudio install-fftw
.PHONY: install-deps

uninstall-deps: uninstall-portaudio uninstall-fftw
.PHONY: uninstall-deps

install-portaudio:
	mkdir -p lib

	curl https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz | tar -zx -C lib
	cd lib/portaudio && ./configure && $(MAKE) -j
.PHONY: install-portaudio

uninstall-portaudio:
	cd lib/portaudio && $(MAKE) uninstall
	rm -rf lib/portaudio
.PHONY: uninstall-portaudio

install-fftw:
	mkdir -p lib

	curl http://www.fftw.org/fftw-3.3.10.tar.gz | tar -zx -C lib
	cd lib/fftw-3.3.10 && ./configure && $(MAKE) -j && sudo $(MAKE) install
.PHONY: install-fftw

uninstall-fftw:
	cd lib/fftw-3.3.10 && $(MAKE) uninstall
	rm -rf lib/fftw-3.3.10
.PHONY: uninstall-fftw

clean:
	rm -f $(EXEC)
.PHONY: clean

clean-all: clean
	rm -rf lib
.PHONY: clean-all
