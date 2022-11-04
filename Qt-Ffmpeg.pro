TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    utils \
    crashhandler\
    ffmpeg \
    mainwindow \
    app

TRANSLATIONS += \
    translations/language.zh_cn.ts \
    translations/language.zh_en.ts

DISTFILES += \
    $$files(docs/*.png)\
    LICENSE \
    README.md
