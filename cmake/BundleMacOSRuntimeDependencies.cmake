if(NOT DEFINED APP_BUNDLE OR NOT IS_DIRECTORY "${APP_BUNDLE}")
    message(FATAL_ERROR "APP_BUNDLE must identify a built macOS application")
endif()

include(BundleUtilities)
file(REMOVE_RECURSE "${APP_BUNDLE}/Contents/Frameworks")
set(BU_CHMOD_BUNDLE_ITEMS ON)
fixup_bundle("${APP_BUNDLE}" "" "${SEARCH_DIRECTORIES}")
verify_app("${APP_BUNDLE}")
