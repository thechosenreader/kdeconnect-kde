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

find_package(QuaZip-Qt5 REQUIRED)
message("${QUAZIP_INCLUDE_DIRS}")
message("${QUAZIP_LIBRARIES}")
set(QUAZIP_INCLUDE_DIRS /usr/local/include/QuaZip-Qt5-1.1/quazip/)
set(QUAZIP_LIBRARIES /usr/local/lib/libquazip1-qt5.so)
include_directories(${QUAZIP_INCLUDE_DIRS})
if(QUAZIP_FOUND)
  message("quazip found")
else(QUAZIP_FOUND)
message("quazip not found")
endif(QUAZIP_FOUND)
kdeconnect_add_plugin(kdeconnect_filemanager JSON kdeconnect_filemanager.json SOURCES ${kdeconnect_filemanager_SRCS})

target_link_libraries(kdeconnect_filemanager
    kdeconnectcore
    Qt5::DBus
    KF5::KCMUtils
    KF5::I18n
    ${QUAZIP_LIBRARIES})


if(NOT SAILFISHOS)
    set( kdeconnect_filemanager_config_SRCS filemanager_config.cpp )

    add_library(kdeconnect_filemanager_config MODULE ${kdeconnect_filemanager_config_SRCS} )
    target_link_libraries( kdeconnect_filemanager_config
        kdeconnectcore
        kdeconnectpluginkcm
        KF5::I18n
        KF5::CoreAddons
        KF5::ConfigWidgets

    )

    install(TARGETS kdeconnect_filemanager_config DESTINATION ${PLUGIN_INSTALL_DIR} )
    install(FILES kdeconnect_filemanager_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
endif()
