QT       += core gui opengl openglwidgets svg svgwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    assetmanager.cpp \
    componentlistwidget.cpp \
    blueprintitem.cpp \
    designscene.cpp \
    dooritem.cpp \
    furnitureitem.cpp \
    main.cpp \
    mainwindow.cpp \
    modelcache.cpp \
    openingitem.cpp \
    view3dwidget.cpp \
    view2dwidget.cpp \
    windowitem.cpp \
    wallitem.cpp

HEADERS += \
    assetmanager.h \
    componentlistwidget.h \
    blueprintitem.h \
    designscene.h \
    dooritem.h \
    furnitureitem.h \
    mainwindow.h \
    modelcache.h \
    openingitem.h \
    view3dwidget.h \
    view2dwidget.h \
    windowitem.h \
    wallitem.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

# Assimp (vcpkg)
win32-g++ {
    # MinGW dynamic
    ASSIMP_DIR = C:/Users/18438/vcpkg/installed/x64-mingw-dynamic
    INCLUDEPATH += $$ASSIMP_DIR/include
    LIBS += -L$$ASSIMP_DIR/lib -lassimp
} else {
    # MSVC 使用 x64-windows
    ASSIMP_DIR = C:/Users/18438/vcpkg/installed/x64-windows
    INCLUDEPATH += $$ASSIMP_DIR/include
    LIBS += -L$$ASSIMP_DIR/lib -lassimp-vc143-mt
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
