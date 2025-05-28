# Инвестиционный менеджер портфеля

Приложение на C++ с использованием Visual C++, CMake, imGUI, OpenGL и менеджера пакетов vcpkg представляет собой инструмент, который поможет дивесифицировать портфель заданными целевыми аллокациями

## 🚀 Установка и сборка проекта

### 📥 1. Скачайте проект
* Само приложение доступно для скачивания на странице [Releases](https://github.com/Artivaa/PortfolioManager/releases)
* Скачать [SourceCode](https://github.com/Artivaa/PortfolioManager/archive/refs/tags/v1.0.zip)

### ⚙️ 2. Требования

* Visual Studio 2022 с компонентом **Desktop Development with C++**
* [vcpkg](https://github.com/microsoft/vcpkg) — менеджер пакетов для C++

### 🛠️ 3. Установка зависимостей через vcpkg

1. Если у вас ещё нет vcpkg, клонируйте и установите его:

   ```bash
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```
2. Установите необходимые пакеты:

   ```bash
   .\vcpkg.exe install tinyfiledialogs glfw3 nlohmann-json opengl
   ```
3. Интегрируйте vcpkg с Visual Studio:

   ```bash
   .\vcpkg.exe integrate install
   ```

### 🔧 4. Сборка проекта

#### Через Visual Studio

1. Откройте Visual Studio 2022.
2. Выберите **File → Open → CMake...** и укажите путь к папке с разархивированным релизом.
3. Visual Studio автоматически подтянет зависимости из vcpkg.
4. Постройте проект через **Build → Build All**.

#### Через командную строку

1. Откройте `x64 Native Tools Command Prompt for VS 2022`.
2. Перейдите в папку проекта:

   ```bash
   cd путь\к\проекту
   ```
3. Создайте папку для сборки и выполните cmake с указанием файла toolchain vcpkg:

   ```bash
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=[путь_к_vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```
