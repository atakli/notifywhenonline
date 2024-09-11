CONFIG += console
CONFIG -= qt

QMAKE_CXXFLAGS_WARN_ON += /std:c++latest

SOURCES += \
    main.cpp

CONFIG(debug, debug|release) {
INCLUDEPATH += "C:\Users\Emre ATAKLI\Documents\clones\tesseract\include"
LIBS += -L"C:\Program Files\Tesseract-OCR"
}

