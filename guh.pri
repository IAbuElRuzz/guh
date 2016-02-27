# Parse and export GUH_VERSION_STRING
GUH_VERSION_STRING=$$system('dpkg-parsechangelog | sed -n -e "s/^Version: //p"')

# define protocol versions
JSON_PROTOCOL_VERSION=36
REST_API_VERSION=1

DEFINES += GUH_VERSION_STRING=\\\"$${GUH_VERSION_STRING}\\\" \
           JSON_PROTOCOL_VERSION=\\\"$${JSON_PROTOCOL_VERSION}\\\" \
           REST_API_VERSION=\\\"$${REST_API_VERSION}\\\"

QT+= network

QMAKE_CXXFLAGS += -Werror -std=c++11
QMAKE_LFLAGS += -std=c++11

# Check for Bluetoot LE support (Qt >= 5.4)
equals(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 3) {
    QT += bluetooth
    DEFINES += BLUETOOTH_LE
}

# Enable coverage option    
coverage {
    OBJECTS_DIR =
    MOC_DIR =
    TOP_SRC_DIR = $$PWD

    LIBS += -lgcov
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LDFLAGS += --coverage

    QMAKE_EXTRA_TARGETS += coverage cov
    QMAKE_EXTRA_TARGETS += clean-gcno clean-gcda coverage-html \
        generate-coverage-html clean-coverage-html coverage-gcovr \
        generate-gcovr generate-coverage-gcovr clean-coverage-gcovr

    clean-gcno.commands = \
        "@echo Removing old coverage instrumentation"; \
        "find -name '*.gcno' -print | xargs -r rm"

    clean-gcda.commands = \
        "@echo Removing old coverage results"; \
        "find -name '*.gcda' -print | xargs -r rm"

    coverage-html.depends = clean-gcda check generate-coverage-html

    generate-coverage-html.commands = \
        "@echo Collecting coverage data"; \
        "lcov --directory $${TOP_SRC_DIR} --capture --output-file coverage.info --no-checksum --compat-libtool"; \
        "lcov --extract coverage.info \"*/server/*.cpp\" --extract coverage.info \"*/libguh/*.cpp\" -o coverage.info"; \
        "lcov --remove coverage.info \"moc_*.cpp\" --remove coverage.info \"*/test/*\" -o coverage.info"; \
        "LANG=C genhtml --prefix $${TOP_SRC_DIR} --output-directory coverage-html --title \"Code Coverage\" --legend --show-details coverage.info"

    clean-coverage-html.depends = clean-gcda
    clean-coverage-html.commands = \
        "lcov --directory $${TOP_SRC_DIR} -z"; \
        "rm -rf coverage.info coverage-html"

    coverage-gcovr.depends = clean-gcda check generate-coverage-gcovr

    generate-coverage-gcovr.commands = \
        "@echo Generating coverage GCOVR report"; \
        "gcovr -x -r $${TOP_SRC_DIR} -o $${TOP_SRC_DIR}/coverage.xml -e \".*/moc_.*\" -e \"tests/.*\" -e \".*\\.h\""

    clean-coverage-gcovr.depends = clean-gcda
    clean-coverage-gcovr.commands = \
        "rm -rf $${TOP_SRC_DIR}/coverage.xml"

    QMAKE_CLEAN += *.gcda *.gcno coverage.info coverage.xml
}


# Enable Radio 433 MHz for GPIO's
enable433gpio {
    DEFINES += GPIO433
}

# check websocket support (Qt >= 5.3)
equals(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 2) {
    DEFINES += WEBSOCKET
}

top_srcdir=$$PWD
top_builddir=$$shadowed($$PWD)
