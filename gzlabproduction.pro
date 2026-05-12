QT       += core gui sql widgets multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = Production

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RC_ICONS = lab.ico

SOURCES += \
    database/fileprocessing.cpp \
    database/shelfdatabase.cpp \
    src/Shelf/shelfdisplaydialog.cpp \
    src/Shelf/shelfitemdialog.cpp \
    src/home/adduserdialog.cpp \
    database/database_manager.cpp \
    main.cpp \
    mainwindow.cpp \
    src/home/assemblydialog.cpp \
    src/home/homedialog.cpp \
    src/home/personalcenterdialog.cpp \
    src/home/projectorwindow.cpp \
    src/home/rackcfgform.cpp \
    src/home/stepshowform.cpp \
    src/home/usermanagerdialog.cpp \
    src/home/visionprocessor.cpp \
    src/login/logindialog.cpp \
    src/user/userassemblydialog.cpp \
    src/user/userassemblyitemdialog.cpp \
    src/user/useroperationdialog.cpp \
    src/user/usersettingsdialog.cpp

HEADERS += \
    database/fileprocessing.h \
    database/shelfdatabase.h \
    qt_file_logger.h \
    src/Shelf/shelfdisplaydialog.h \
    src/Shelf/shelfitemdialog.h \
    src/home/adduserdialog.h \
    database/database_manager.h \
    database/fileconfigure.h \
    mainwindow.h \
    src/home/assemblydialog.h \
    src/home/homedialog.h \
    src/home/personalcenterdialog.h \
    src/home/projectorwindow.h \
    src/home/rackcfgform.h \
    src/home/stepshowform.h \
    src/home/usermanagerdialog.h \
    src/home/visionprocessor.h \
    src/login/logindialog.h \
    src/user/userassemblydialog.h \
    src/user/userassemblyitemdialog.h \
    src/user/useroperationdialog.h \
    src/user/usersettingsdialog.h \
    structs/datatypes.h

FORMS += \
    src/Shelf/shelfdisplaydialog.ui \
    src/Shelf/shelfitemdialog.ui \
    src/home/adduserdialog.ui \
    mainwindow.ui \
    src/home/homedialog.ui \
    src/home/personalcenterdialog.ui \
    src/home/rackcfgform.ui \
    src/home/stepshowform.ui \
    src/home/usermanagerdialog.ui \
    src/login/logindialog.ui \
    src/user/userassemblydialog.ui \
    src/user/userassemblyitemdialog.ui \
    src/user/useroperationdialog.ui \
    src/user/usersettingsdialog.ui

OBJECTS_DIR = build/obj
MOC_DIR     = build/moc
UI_DIR      = build/ui
RCC_DIR     = build/rcc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    prostyle.qrc

# 核心修复：添加头文件检索路径（解决"adduserdialog.h not found"）
# $$PWD 代表当前.pro文件所在目录，自动适配Windows/Linux/macOS
INCLUDEPATH += $$PWD \               # 根目录（mainwindow.h所在）
               $$PWD/src/home \      # adduserdialog.h所在目录
               $$PWD/src/login \     # logindialog.h所在目录
               $$PWD/database \      # database_manager.h所在目录
               $$PWD/structs \       # datatypes.h所在目录
               $$PWD/src/Shelf \     # shelfdisplaydialog.h所在目录
               $$PWD/src/user \      # useroperationdialog.h所在目录

# ----------------------------------------------------
# OpenCV 4.5.5 MinGW 分体库精确配置
# ----------------------------------------------------

# 1. 头文件包含路径不变
INCLUDEPATH += D:/OpenCV/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/include

# 2. 库文件包含路径
LIBS += -LD:/OpenCV/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/x64/mingw/lib \
        -lopencv_core455 \
        -lopencv_imgproc455 \
        -lopencv_highgui455 \
        -lopencv_videoio455 \
        -lopencv_video455 \
        -lopencv_imgcodecs455 \
        -lopencv_objdetect455
