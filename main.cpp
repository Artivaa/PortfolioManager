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
#include <cmath> // Для std::round
#include <imgui_internal.h>

struct Asset {
    std::string name;
    float quantity;
    float price;

    float value() const { return quantity * price; }
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
    float unitsToBuyOrSell;
};

class PortfolioApp {
private:
    std::vector<Asset> assets;
    std::vector<TargetAllocation> targets;
    std::vector<RebalanceAction> actions;
    char nameBuffer[128] = "";
    float quantity = 0.0f;
    float price = 0.0f;
    float extraCapital = 0.0f;
    bool firstFrame = true; // Для инициализации DockSpace один раз

    float getTotalValue() const {
        return std::accumulate(assets.begin(), assets.end(), 0.0f,
            [](float sum, const Asset& a) { return sum + a.value(); });
    }

    void drawPieChart() {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 center = ImGui::GetCursorScreenPos();
        center.x += 150; center.y += 150;
        float radius = 120.0f;
        float total_value = getTotalValue();
        float start_angle = 0.0f;

        static const ImU32 fixed_colors[] = {
            IM_COL32(255, 99, 132, 255),   // Розовый
            IM_COL32(54, 162, 235, 255),   // Голубой
            IM_COL32(255, 206, 86, 255),   // Желтый
            IM_COL32(75, 192, 192, 255),   // Бирюзовый
            IM_COL32(153, 102, 255, 255)   // Фиолетовый
        };
        std::vector<ImU32> colors;

        for (size_t i = 0; i < assets.size(); ++i) {
            if (total_value <= 0) continue;
            float percentage = (assets[i].value() / total_value) * 2.0f * 3.14159f;
            ImU32 color = fixed_colors[i % (sizeof(fixed_colors) / sizeof(fixed_colors[0]))];
            colors.push_back(color);

            draw_list->PathClear();
            draw_list->PathArcTo(center, radius, start_angle, start_angle + percentage, 32);
            draw_list->PathLineTo(center);
            draw_list->PathFillConvex(color);
            start_angle += percentage;

            float mid_angle = start_angle - percentage / 2;
            ImVec2 text_pos(center.x + cos(mid_angle) * radius * 0.6f, center.y + sin(mid_angle) * radius * 0.6f);
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f%%", (assets[i].value() / total_value) * 100.0f);
            draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), buf);
        }
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
                float units = std::round(diff / it->price);

                actions.push_back({ it->name, current_value, target_value, diff, units });
                extraCapital += diff;
            }
        }
    }

    void savePortfolio() {
        nlohmann::json j;
        for (const auto& asset : assets) {
            j["assets"].push_back({ {"name", asset.name}, {"quantity", asset.quantity}, {"price", asset.price} });
        }
        std::ofstream file("portfolio.json");
        file << j.dump(4);
    }

    void loadPortfolio() {
        std::ifstream file("portfolio.json");
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;
            assets.clear();
            targets.clear();
            for (const auto& item : j["assets"]) {
                assets.push_back({ item["name"].get<std::string>(),
                                 item["quantity"].get<float>(),
                                 item["price"].get<float>() });
                targets.push_back({ item["name"].get<std::string>(), 0.0f });
            }
        }
    }

public:
    void run() {
        if (!glfwInit()) {
            return;
        }

        GLFWwindow* window = glfwCreateWindow(1280, 720, "Portfolio Manager", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        // Включаем поддержку DockSpace
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Создаем DockSpace
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

            // Настройка начальной компоновки (выполняется один раз)
            if (firstFrame) {
                firstFrame = false;

                ImGui::DockBuilderRemoveNode(dockspace_id); // Очищаем предыдущую компоновку
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                ImGuiID dock_main_id = dockspace_id;
                ImGuiID dock_left_id, dock_right_id;
                ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.4f, &dock_left_id, &dock_right_id);

                ImGuiID dock_left_up_id, dock_left_down_id;
                ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Up, 0.5f, &dock_left_up_id, &dock_left_down_id);

                ImGuiID dock_right_up_id, dock_right_down_id;
                ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Up, 0.5f, &dock_right_up_id, &dock_right_down_id);

                ImGui::DockBuilderDockWindow("Asset Input", dock_left_up_id);
                ImGui::DockBuilderDockWindow("Target Allocations", dock_left_down_id);
                ImGui::DockBuilderDockWindow("Portfolio Breakdown", dock_right_up_id);
                ImGui::DockBuilderDockWindow("Rebalance Results", dock_right_down_id);

                ImGui::DockBuilderFinish(dockspace_id);
            }

            ImGui::End();
            ImGui::PopStyleVar(3);

            // Панель 1: Asset Input
            ImGui::Begin("Asset Input");
            ImGui::InputText("Name", nameBuffer, IM_ARRAYSIZE(nameBuffer));
            ImGui::InputFloat("Quantity", &quantity, 0.1f, 1.0f, "%.2f");
            ImGui::InputFloat("Price", &price, 0.1f, 1.0f, "%.2f");
            if (ImGui::Button("Add Asset") && nameBuffer[0] && quantity > 0 && price > 0) {
                assets.push_back({ nameBuffer, quantity, price });
                targets.push_back({ nameBuffer, 0.0f });
                nameBuffer[0] = '\0';
                quantity = price = 0.0f;
            }

            ImGui::SameLine();
            if (ImGui::Button("Load Portfolio")) loadPortfolio();

            ImGui::Text("Current Assets:");
            if (ImGui::BeginTable("AssetsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Quantity");
                ImGui::TableSetupColumn("Value");
                ImGui::TableSetupColumn("Action");
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < assets.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", assets[i].name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", assets[i].quantity);
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
            if (ImGui::Button("Save Portfolio")) savePortfolio();
            ImGui::End();

            // Панель 2: Portfolio Breakdown
            ImGui::Begin("Portfolio Breakdown");
            // Таблица слева
            ImGui::Text("Asset Shares:");
            if (ImGui::BeginTable("SharesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Percentage", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 120.0f);
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
            // Диаграмма правее таблицы
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 200); // Сдвиг вправо
            drawPieChart();

            ImGui::End();

            // Панель 3: Target Allocations
            ImGui::Begin("Target Allocations");
            for (auto& target : targets) {
                ImGui::InputFloat(target.name.c_str(), &target.targetPercent, 0.1f, 1.0f, "%.2f");
                if (target.targetPercent < 0.0f) target.targetPercent = 0.0f;
            }
            if (ImGui::Button("Calculate Rebalance")) calculateRebalance();
            float total_target_percent = std::accumulate(targets.begin(), targets.end(), 0.0f,
                [](float sum, const TargetAllocation& t) { return sum + t.targetPercent; });
            if (abs(total_target_percent - 100.0f) > 0.01f) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Target percentages must sum to 100%%!");
            }
            ImGui::End();

            // Панель 4: Rebalance Results
            ImGui::Begin("Rebalance Results");
            if (ImGui::BeginTable("RebalanceTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Diff Value");
                ImGui::TableSetupColumn("Units to Buy/Sell");
                ImGui::TableSetupColumn("Action");
                ImGui::TableHeadersRow();
                for (const auto& action : actions) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", action.name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", action.diffValue);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d", static_cast<int>(action.unitsToBuyOrSell));
                    ImGui::TableSetColumnIndex(3); ImGui::Text(action.unitsToBuyOrSell >= 0 ? "Buy" : "Sell");
                    if (action.unitsToBuyOrSell < 0) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(150, 150, 150, 255));
                    }
                    else if (action.unitsToBuyOrSell > 0) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(150, 120, 90, 255));
                    }
                }
                ImGui::EndTable();
            }
            ImGui::Text("Extra Capital Needed: %.2f", extraCapital);
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