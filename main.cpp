#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <numeric>
#include <random>
#include <cmath>
#include <imgui_internal.h>
#include "tinyfiledialogs.h" 
#include <iostream>

struct Asset {
    std::string name;
    int quantity;
    float price;
    ImU32 color;
    float value() const { return static_cast<float>(quantity) * price; }
};

struct TargetAllocation {
    std::string name;
    float targetPercent;
};

struct RebalanceAction {
    std::string name;
    float currentValue;
    float targetValue;
    float diffValue;
    int unitsToBuyOrSell;
};

class PortfolioApp {
private:
    std::vector<Asset> assets;
    std::vector<TargetAllocation> targets;
    std::vector<TargetAllocation> previousTargets;
    std::vector<RebalanceAction> actions;
    std::vector<Asset> previousAssets;
    char nameBuffer[128] = "";
    int quantity = 0;
    float price = 0.0f;
    float extraCapital = 0.0f;
    bool firstFrame = true;
    ImFont* robotoFont = nullptr;

    ImU32 generateRandomColor() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> hueDist(0.0f, 1.0f);
        return ImColor::HSV(hueDist(gen), 0.8f, 0.8f);
    }

    float getTotalValue() const {
        return std::accumulate(assets.begin(), assets.end(), 0.0f,
            [](float sum, const Asset& a) { return sum + a.value(); });
    }

    void drawBarChart() {
        ImGui::Text(u8"Доли активов:");

        float total_value = getTotalValue();
        if (total_value <= 0.0f) return;

        ImVec2 graph_size = ImVec2(ImGui::GetContentRegionAvail().x, 150.0f);
        ImVec2 cursor = ImGui::GetCursorScreenPos();

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(cursor, ImVec2(cursor.x + graph_size.x, cursor.y + graph_size.y), IM_COL32(50, 50, 50, 255));

        float x_step = graph_size.x / assets.size();
        float max_bar_height = graph_size.y * 0.8f;
        float current_x = cursor.x;

        for (const auto& asset : assets) {
            float height = (asset.value() / total_value) * max_bar_height;
            ImVec2 bar_start(current_x + 2.0f, cursor.y + graph_size.y - height);
            ImVec2 bar_end(current_x + x_step - 2.0f, cursor.y + graph_size.y);

            // Отрисовка столбца
            draw_list->AddRectFilled(bar_start, bar_end, asset.color);

            // Подпись названия актива
            if (x_step > 30.0f) { // Показываем текст только если столбцы достаточно широкие
                ImVec2 text_pos(current_x + 5.0f, cursor.y + graph_size.y - 20.0f);
                draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), asset.name.c_str());
            }

            current_x += x_step;
        }

        ImGui::Dummy(graph_size); // Сдвигаем курсор для последующих элементов
    }

    void calculateRebalance() {
        actions.clear();
        float total_value = getTotalValue();
        float total_target_percent = std::accumulate(targets.begin(), targets.end(), 0.0f,
            [](float sum, const TargetAllocation& t) { return sum + t.targetPercent; });

        if (abs(total_target_percent - 100.0f) > 0.01f) {
            return;
        }

        extraCapital = 0.0f;
        for (const auto& target : targets) {
            auto it = std::find_if(assets.begin(), assets.end(),
                [&](const Asset& a) { return a.name == target.name; });
            if (it != assets.end()) {
                float current_value = it->value();
                float target_value = total_value * (target.targetPercent / 100.0f);
                float diff = target_value - current_value;
                int units = static_cast<int>(std::round(diff / it->price));

                actions.push_back({ it->name, current_value, target_value, diff, units });
                extraCapital += diff;
            }
        }
    }

    void savePortfolio() {
        const char* filterPatterns[] = { "*.json" };
        const char* filePath = tinyfd_saveFileDialog("Сохранить портфель", "", 1, filterPatterns, "JSON files");
        if (filePath) {
            nlohmann::json j;
            for (const auto& asset : assets) {
                j["assets"].push_back({ {"name", asset.name}, {"quantity", asset.quantity}, {"price", asset.price} });
            }
            std::ofstream file(filePath);
            file << j.dump(4);
        }
    }

    void loadPortfolio() {
        const char* filterPatterns[] = { "*.json" };
        const char* filePath = tinyfd_openFileDialog("Загрузить портфель", "", 1, filterPatterns, "JSON files", 0);
        if (filePath) {
            std::ifstream file(filePath);
            if (file.is_open()) {
                nlohmann::json j;
                file >> j;
                assets.clear();
                targets.clear();
                for (const auto& item : j["assets"]) {
                    assets.push_back({ item["name"].get<std::string>(), item["quantity"].get<int>(), item["price"].get<float>(), generateRandomColor() });
                    targets.push_back({ item["name"].get<std::string>(), 0.0f });
                }
            }
        }
    }

public:
    void run() {
        if (!glfwInit()) return;

        GLFWwindow* window = glfwCreateWindow(1280, 720, u8"Менеджер Портфеля", nullptr, nullptr);
        if (!window) { glfwTerminate(); return; }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        
        ImFontConfig fontConfig;
        fontConfig.MergeMode = false; 
        ImWchar ranges[] = { 0x0020, 0x00FF, 
                             0x0400, 0x04FF, 
                             0 };
        robotoFont = io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf", 16.0f, &fontConfig, ranges);
        if (robotoFont == nullptr) {
            std::cerr << "Не удалось загрузить шрифт Roboto Medium." << std::endl;
        }
        else {
            io.FontDefault = robotoFont;
        }

        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            ImGui::Begin("DockSpace Window", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

            if (firstFrame) {
                firstFrame = false;
                ImGui::DockBuilderRemoveNode(dockspace_id);
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                ImGuiID dock_main_id = dockspace_id;
                ImGuiID dock_left_id, dock_right_id;
                ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.4f, &dock_left_id, &dock_right_id);

                ImGuiID dock_left_up_id, dock_left_down_id;
                ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Up, 0.5f, &dock_left_up_id, &dock_left_down_id);

                ImGuiID dock_right_up_id, dock_right_down_id;
                ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Up, 0.5f, &dock_right_up_id, &dock_right_down_id);

                ImGui::DockBuilderDockWindow(u8"Ввод активов", dock_left_up_id);
                ImGui::DockBuilderDockWindow(u8"Целевая аллокация", dock_left_down_id);
                ImGui::DockBuilderDockWindow(u8"Структура портфеля", dock_right_up_id);
                ImGui::DockBuilderDockWindow(u8"Результаты ребалансировки", dock_right_down_id);

                ImGui::DockBuilderFinish(dockspace_id);
            }

            ImGui::End();
            ImGui::PopStyleVar(3);

            // Панель 1: Asset Input
            ImGui::Begin(u8"Ввод активов");
            ImGui::InputText(u8"Имя", nameBuffer, IM_ARRAYSIZE(nameBuffer));
            ImGui::InputInt(u8"Количество", &quantity);
            ImGui::InputFloat(u8"Цена", &price, 0.1f, 1.0f, "%.2f");
            if (ImGui::Button(u8"Добавить актив") && nameBuffer[0] && quantity > 0 && price > 0) {
                assets.push_back({ nameBuffer, quantity, price, generateRandomColor() });
                targets.push_back({ nameBuffer, 0.0f });
                nameBuffer[0] = '\0';
                quantity = 0;
                price = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"Загрузить портфель")) loadPortfolio();

            ImGui::Text(u8"Текущие активы");
            if (ImGui::BeginTable("AssetsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn(u8"Имя");
                ImGui::TableSetupColumn(u8"Количество");
                ImGui::TableSetupColumn(u8"Цена");
                ImGui::TableSetupColumn(u8"Действие");
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < assets.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", assets[i].name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%d", assets[i].quantity);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f", assets[i].value());
                    ImGui::TableSetColumnIndex(3);
                    if (ImGui::Button(("Delete##" + assets[i].name).c_str())) {
                        assets.erase(assets.begin() + i);
                        targets.erase(targets.begin() + i);
                        --i;
                    }
                }
                ImGui::EndTable();
            }
            if (ImGui::Button(u8"Сохранить портфель")) savePortfolio();
            ImGui::End();

            // Панель 2: Portfolio Breakdown
            ImGui::Begin(u8"Структура портфеля");
            ImGui::Text(u8"Доли активов:");
            if (ImGui::BeginTable("SharesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn(u8"Имя");
                ImGui::TableSetupColumn(u8"Процент");
                ImGui::TableSetupColumn(u8"Цена");
                ImGui::TableHeadersRow();
                float total_value = getTotalValue();
                for (int i = 0; i < assets.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, i % 2 == 0 ? IM_COL32(30, 30, 30, 255) : IM_COL32(40, 40, 40, 255));
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", assets[i].name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f%%", total_value > 0 ? (assets[i].value() / total_value) * 100.0f : 0.0f);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f", assets[i].value());
                }
                ImGui::EndTable();
            }
            drawBarChart();
            ImGui::End();

            // Панель 3: Target Allocations
            ImGui::Begin(u8"Целевая аллокация");
            
            for (auto& target : targets) {
                ImGui::InputFloat(target.name.c_str(), &target.targetPercent, 0.1f, 1.0f, "%.2f");
                if (target.targetPercent < 0.0f) target.targetPercent = 0.0f;
            }
            if (ImGui::Button(u8"Рассчитать")) {
                previousTargets = targets;
                calculateRebalance();
            }
            
            float total_target_percent = std::accumulate(targets.begin(), targets.end(), 0.0f,
                [](float sum, const TargetAllocation& t) { return sum + t.targetPercent; });
            if (total_target_percent < 100.0f) {
                ImGui::Text(u8"Осталось до 100%%: %.2f%%", 100.0f - total_target_percent);
            }
            else if (total_target_percent > 100.0f) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"Превышение на: %.2f%%", total_target_percent - 100.0f);
            }
            else {
                ImGui::Text(u8"Итого: 100%%");
            }
            
            
            ImGui::End();

            // Панель 4: Rebalance Results
            ImGui::Begin(u8"Результаты ребалансировки");
            if (ImGui::BeginTable("RebalanceTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn(u8"Имя");
                ImGui::TableSetupColumn(u8"Изменения");
                ImGui::TableSetupColumn(u8"Кол-во Купить/Продать");
                ImGui::TableSetupColumn(u8"Действие");
                ImGui::TableHeadersRow();
                for (const auto& action : actions) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", action.name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", action.diffValue);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d", std::abs(action.unitsToBuyOrSell));
                    ImGui::TableSetColumnIndex(3);
                    if (action.unitsToBuyOrSell > 0) {
                        ImGui::Text(u8"Купить");
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(100, 255, 100, 255));
                    }
                    else if (action.unitsToBuyOrSell < 0) {
                        ImGui::Text(u8"Продать");
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 100, 100, 255));
                    }
                    else {
                        ImGui::Text(u8"Удержать");
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(200, 200, 200, 255));
                    }
                }
                ImGui::EndTable();
            }
            ImGui::Text(u8"Дополнительный капитал: %.2f", extraCapital);
            if (ImGui::Button(u8"Применить ребалансировку")) {
                previousAssets = assets;
                for (const auto& action : actions) {
                    auto it = std::find_if(assets.begin(), assets.end(),
                        [&](const Asset& a) { return a.name == action.name; });
                    if (it != assets.end()) {
                        it->quantity += action.unitsToBuyOrSell;
                        if (it->quantity < 0) it->quantity = 0;
                    }
                }
                actions.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"Отменить изменения") && !previousAssets.empty()) {
                assets = previousAssets; // Восстановление состояния
            }
            ImGui::End();

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    PortfolioApp app;
    app.run();
    return 0;
}