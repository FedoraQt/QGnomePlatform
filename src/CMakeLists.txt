add_subdirectory(common)

if (NOT DISABLE_DECORATION_SUPPORT)
    add_subdirectory(decoration)
endif()

if (NOT DISABLE_THEME_SUPPORT)
    add_subdirectory(theme)
endif()
