TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -L/usr/local/lib -pthread -lssl -lcrypto -lpcap

SOURCES += \
        src/check_manager.cpp \
        src/log_manager.cpp \
        src/main.cpp \
        src/network_manager.cpp

HEADERS += \
    src/blocker.h \
    src/check_manager.h \
    src/ether_addr.h \
    src/log_manager.h \
    src/mutex_map.h \
    src/mutex_queue.h \
    src/network_header.h \
    src/network_manager.h \
    src/rst_packet.h \
    src/session.h \
    src/session_manager.h \
    src/singleton.h \
    src/sock_addr.h
