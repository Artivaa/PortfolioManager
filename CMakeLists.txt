cmake_minimum_required(VERSION 3.10)
project(PortfolioManager LANGUAGES CXX)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ищем библиотеки через vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Добавляем исполняемый файл
add_executable(PortfolioManager
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
    OpenGL::GL
    nlohmann_json::nlohmann_json
)
