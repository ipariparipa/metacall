TEMPLATE = subdirs

SUBDIRS += lib \
           test \

test.depends += lib
lib.file = lib/metacall.pro
