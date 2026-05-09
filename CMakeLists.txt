cmake_minimum_required(VERSION 3.16)

project(OCRVideoApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Provide the path to the newly installed Qt 6.5.3
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.5.3/msvc2019_64")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/dependencies/vcpkg/installed/x64-windows")

find_package(Qt6 6.5.3 REQUIRED COMPONENTS Core Gui Quick Multimedia Concurrent QuickControls2)
# Provide the path to OpenCV (we downloaded it locally)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/dependencies/opencv/build")
find_package(OpenCV REQUIRED)

# Try to locate Tesseract from local workspace vcpkg first.
set(TESSERACT_HINT_DIRS
    "${CMAKE_SOURCE_DIR}/dependencies/vcpkg/installed/x64-windows"
)

find_package(Tesseract QUIET)

set(TESSERACT_TARGET "")
if(Tesseract_FOUND)
    if(TARGET Tesseract::libtesseract)
        set(TESSERACT_TARGET Tesseract::libtesseract)
    elseif(TARGET Tesseract::tesseract)
        set(TESSERACT_TARGET Tesseract::tesseract)
    endif()
endif()

if(NOT TESSERACT_TARGET)
    find_path(TESSERACT_INCLUDE_DIR
        NAMES tesseract/baseapi.h
        HINTS ${TESSERACT_HINT_DIRS}
        PATH_SUFFIXES include
    )

    find_library(TESSERACT_LIBRARY_RELEASE
        NAMES tesseract55 tesseract
        HINTS ${TESSERACT_HINT_DIRS}
        PATH_SUFFIXES lib
    )

    find_library(TESSERACT_LIBRARY_DEBUG
        NAMES tesseract55d tesseractd
        HINTS ${TESSERACT_HINT_DIRS}
        PATH_SUFFIXES debug/lib lib
    )

    if(TESSERACT_INCLUDE_DIR AND (TESSERACT_LIBRARY_RELEASE OR TESSERACT_LIBRARY_DEBUG))
        add_library(OCRVideoApp::tesseract UNKNOWN IMPORTED)
        set_target_properties(OCRVideoApp::tesseract PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${TESSERACT_INCLUDE_DIR}"
        )

        if(TESSERACT_LIBRARY_RELEASE)
            set_property(TARGET OCRVideoApp::tesseract PROPERTY IMPORTED_LOCATION_RELEASE "${TESSERACT_LIBRARY_RELEASE}")
        endif()

        if(TESSERACT_LIBRARY_DEBUG)
            set_property(TARGET OCRVideoApp::tesseract PROPERTY IMPORTED_LOCATION_DEBUG "${TESSERACT_LIBRARY_DEBUG}")
        endif()

        if(NOT TESSERACT_LIBRARY_RELEASE AND TESSERACT_LIBRARY_DEBUG)
            set_property(TARGET OCRVideoApp::tesseract PROPERTY IMPORTED_LOCATION_RELEASE "${TESSERACT_LIBRARY_DEBUG}")
        endif()

        if(NOT TESSERACT_LIBRARY_DEBUG AND TESSERACT_LIBRARY_RELEASE)
            set_property(TARGET OCRVideoApp::tesseract PROPERTY IMPORTED_LOCATION_DEBUG "${TESSERACT_LIBRARY_RELEASE}")
        endif()

        set(TESSERACT_TARGET OCRVideoApp::tesseract)
    else()
        message(FATAL_ERROR
            "Tesseract not found in project dependencies. Expected path: ${CMAKE_SOURCE_DIR}/dependencies/vcpkg/installed/x64-windows")
    endif()
endif()

set(TESSDATA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/vcpkg/installed/x64-windows/share/tessdata")
set(TESSDATA_FALLBACK_DIR "${CMAKE_SOURCE_DIR}/dependencies/tessdata")

set(TESSDATA_DEPLOY_SOURCE "")
if(EXISTS "${TESSDATA_SOURCE_DIR}/eng.traineddata")
    set(TESSDATA_DEPLOY_SOURCE "${TESSDATA_SOURCE_DIR}")
elseif(EXISTS "${TESSDATA_FALLBACK_DIR}/eng.traineddata")
    set(TESSDATA_DEPLOY_SOURCE "${TESSDATA_FALLBACK_DIR}")
endif()

qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)

qt_add_executable(appOCRVideoApp
    main.cpp
    videoprocessor.h videoprocessor.cpp
)

qt_add_qml_module(appOCRVideoApp
    URI OCRVideoApp
    VERSION 1.0
    QML_FILES 
        qml/Main.qml
        qml/ColorDialog.qml
        qml/ResultDialog.qml
)

target_include_directories(appOCRVideoApp PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

target_compile_definitions(appOCRVideoApp PRIVATE
    OCR_DEFAULT_TESSDATA_DIR="${TESSDATA_FALLBACK_DIR}"
)

target_link_libraries(appOCRVideoApp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Concurrent
    Qt6::QuickControls2
    ${OpenCV_LIBS}
    ${TESSERACT_TARGET}
)

# For Windows, copy DLLs or set runtime paths
if(WIN32)
    set_target_properties(appOCRVideoApp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

install(TARGETS appOCRVideoApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Deploy OCR data files from available source path.
if(TESSDATA_DEPLOY_SOURCE)
    add_custom_command(TARGET appOCRVideoApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:appOCRVideoApp>/tessdata"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TESSDATA_DEPLOY_SOURCE}/eng.traineddata"
            "$<TARGET_FILE_DIR:appOCRVideoApp>/tessdata/eng.traineddata"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TESSDATA_FALLBACK_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TESSDATA_DEPLOY_SOURCE}/eng.traineddata"
            "${TESSDATA_FALLBACK_DIR}/eng.traineddata"
        COMMENT "Deploying Tesseract eng.traineddata"
    )
else()
    message(WARNING
    "eng.traineddata not found in project dependencies (vcpkg/share/tessdata or dependencies/tessdata). OCR init will fail until tessdata is provided.")
endif()
