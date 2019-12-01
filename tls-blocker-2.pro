TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -L/usr/local/lib -lssl -lcrypto -lpcap

SOURCES += \
        log_manager.cpp \
        main.cpp \
        network_manager.cpp

HEADERS += \
    blocker.h \
    log_manager.h \
    mutex_queue.h \
    network_manager.h
