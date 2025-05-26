cmake_minimum_required(VERSION 3.10)
project(PortfolioManager LANGUAGES CXX)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ищем библиотеки через vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tinyfiledialogs CONFIG REQUIRED)

# Добавляем исполняемый файл

add_executable(PortfolioManager WIN32
    main.cpp

    # ImGui core
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp

    # ImGui backends
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

# Указываем директории для включения заголовков
target_include_directories(PortfolioManager PRIVATE
    ${CMAKE_SOURCE_DIR}/imgui
    ${CMAKE_SOURCE_DIR}/imgui/backends
)

# Линкуем нужные библиотеки
target_link_libraries(PortfolioManager PRIVATE
    glfw
    tinyfiledialogs::tinyfiledialogs
    OpenGL::GL
    nlohmann_json::nlohmann_json
)

configure_file(
    ${CMAKE_SOURCE_DIR}/fonts/Roboto-Medium.ttf
    ${CMAKE_BINARY_DIR}/fonts/Roboto-Medium.ttf
    COPYONLY
)
