find_package(PkgConfig)

pkg_check_modules(PC_GLIB2 REQUIRED glib-2.0)

find_path(GLIB_SCHEMAS_DIR org.gnome.desktop.interface.gschema.xml
    HINTS ${PC_GLIB2_PREFIX}/share
    PATH_SUFFIXES glib-2.0/schemas)

if (GLIB_SCHEMAS_DIR)
    set(GSettingSchemas_FOUND true)
else()
    set(GSettingSchemas_FOUND false)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GSettingSchemas
    FOUND_VAR
       GSettingSchemas_FOUND
    REQUIRED_VARS
       GSettingSchemas_FOUND
)

mark_as_advanced(GSettingSchemas_FOUND)
