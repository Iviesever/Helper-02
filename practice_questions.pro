QT       += core gui widgets
TARGET = practice_questions
TEMPLATE = app

# 关键：开启 C++23 以兼容你的代码
CONFIG += c++23

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    platform_utils.cpp \
    storage_manager.cpp \
    parser/text_parser.cpp \
    pages/HomePage.cpp \
    pages/HistoryPage.cpp \
    pages/ExamConfigPage.cpp \
    pages/ParserStrategyPage.cpp \
    pages/PracticeStrategyPage.cpp \
    pages/SettingsPage.cpp

HEADERS += \
    MainWindow.h \
    parser/text_parser.h \
    parser/parser_strategy.h \
    platform_utils.h \
    question.h \
    storage_manager.h \
    pages/HomePage.h \
    pages/SettingsPage.h \
    pages/HistoryPage.h \
    pages/ExamConfigPage.h \
    pages/ParserStrategyPage.h \
    pages/PracticeStrategyPage.h

FORMS += \
    MainWindow.ui \
    pages/HomePage.ui \
    pages/SettingsPage.ui \
    pages/HistoryPage.ui \
    pages/ExamConfigPage.ui \
    pages/ParserStrategyPage.ui \
    pages/PracticeStrategyPage.ui

# Android 配置目录
android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

DISTFILES += \
    android/AndroidManifest.xml \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/build.gradle \
    android/res/values/libs.xml \
    android/res/values/libs.xml \
    android/res/xml/qtprovider_paths.xml \
    android/res/xml/qtprovider_paths.xml