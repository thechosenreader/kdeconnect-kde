set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_filemanager_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_FILEMANAGER CATEGORY_NAME kdeconnect.plugin.filemanager
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin filemanager)")

set(kdeconnect_filemanager_SRCS
    filemanagerplugin.cpp
    ${debug_file_SRCS}
)

find_package(QuaZip5 REQUIRED)
message("${QUAZIP_INCLUDE_DIRS}")
include_directories(${QUAZIP_INCLUDE_DIRS})

kdeconnect_add_plugin(kdeconnect_filemanager JSON kdeconnect_filemanager.json SOURCES ${kdeconnect_filemanager_SRCS})

target_link_libraries(kdeconnect_filemanager
    kdeconnectcore
    Qt5::DBus
    KF5::KCMUtils
    KF5::I18n
    ${QUAZIP_LIBRARIES})
