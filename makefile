CXXFLAGS!= fltk-config --cxxflags
LDFLAGS!= fltk-config --ldflags
LDFLAGS+= -lsndio

.SUFFIXES: .cpp .o
.SILENT: clean

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<
volcon: volcon.o node_list.o
	${CXX} ${CXXFLAGS} ${LDFLAGS} -o $@ volcon.o node_list.o
clean:
	rm -f volcon
	rm -f *.o
