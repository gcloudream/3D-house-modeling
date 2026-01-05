QT       += core gui opengl openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    componentlistwidget.cpp \
    blueprintitem.cpp \
    designscene.cpp \
    dooritem.cpp \
    main.cpp \
    mainwindow.cpp \
    openingitem.cpp \
    view3dwidget.cpp \
    view2dwidget.cpp \
    windowitem.cpp \
    wallitem.cpp

HEADERS += \
    componentlistwidget.h \
    blueprintitem.h \
    designscene.h \
    dooritem.h \
    mainwindow.h \
    openingitem.h \
    view3dwidget.h \
    view2dwidget.h \
    windowitem.h \
    wallitem.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
