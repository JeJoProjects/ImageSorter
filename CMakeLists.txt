cmake_minimum_required(VERSION 3.20)
project(ImageSorter VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find all C++ source files in the current directory
file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp")

# Configure the resource file
configure_file(resources.rc.in ${CMAKE_CURRENT_BINARY_DIR}/resources.rc @ONLY)

# Add the executable with the resource file and all source files
add_executable(ImageSorter WIN32
    ${SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/resources.rc
)

target_include_directories(ImageSorter PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Link necessary libraries
target_link_libraries(ImageSorter PRIVATE comctl32 Shell32)

# Set Windows subsystem
set_target_properties(ImageSorter PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/MANIFESTUAC:\"level='asInvoker' uiAccess='false'\""
)

# Add manifest for Common Controls v6
set_property(TARGET ImageSorter PROPERTY VS_MANIFEST_XML
    "<?xml version='1.0' encoding='UTF-8' standalone='yes'?>
    <assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'>
      <dependency>
        <dependentAssembly>
          <assemblyIdentity type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*' />
        </dependentAssembly>
      </dependency>
    </assembly>"
)

# Set the startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ImageSorter)

# Enable folder view in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
