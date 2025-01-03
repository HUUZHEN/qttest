QT       += core gui widgets charts

CONFIG   += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

# 部署目標規則（默認即可，無需修改）
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 指定 Qt6 Charts 的鏈接庫路徑
LIBS += -LC:/Qt/6.7.3/mingw_64/lib -lQt6Charts -lQt6ChartsQml
