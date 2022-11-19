QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets openglwidgets 3dcore 3drender 3dlogic 3dinput 3dextras

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    maincore.cpp \
    mainwindow.cpp \
    networkrequest.cpp

HEADERS += \
    include/mm.hpp \
    include/qt3dwidget.h \
    maincore.h \
    mainwindow.h \
    networkrequest.h

FORMS += \
    mainwindow.ui

LIBS += "$${PWD}/lib/libqt3dwidget.a"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    cache/0_0_0.jpg


CONFIG(debug, debug|release) {
    SUBFOLDER = debug
} else {
    SUBFOLDER = release
}
copydata.commands = $(COPY_DIR) \"$${PWD}/cache\" \"$${OUT_PWD}/cache\"
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata
