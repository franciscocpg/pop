TEMPLATE=subdirs
SUBDIRS=quazip qztest
qztest.depends = quazip
CONFIG += staticlib
LIBS+=-L/../zlib/libz.a
INCLUDEPATH+=../zlib/
