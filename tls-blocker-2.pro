TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -L/usr/local/lib -pthread -lssl -lcrypto -lpcap

SOURCES += \
        log_manager.cpp \
        main.cpp \
        network_manager.cpp

HEADERS += \
    blocker.h \
    ether_addr.h \
    log_manager.h \
    mutex_queue.h \
    network_manager.h \
    sock_addr.h
