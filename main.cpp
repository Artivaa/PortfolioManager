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

    float getTotalValue() const {
        return std::accumulate(assets.begin(), assets.end(), 0.0f,
            [](float sum, const Asset& a) { return sum + a.value(); });
    }

    void savePortfolio() {
        nlohmann::json j;
        for (const auto& asset : assets) {
            j["assets"].push_back({ {"name", asset.name}, {"quantity", asset.quantity}, {"price", asset.price} });
        }
        std::ofstream file("portfolio.json");
        file << j.dump(4);
    }
   
public:
    void run() {
        // Инициализация GLFW
        if (!glfwInit()) {
            return;
        }

        // Создание окна
        GLFWwindow* window = glfwCreateWindow(1280, 720, "Portfolio Manager", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        // Инициализация ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        // Главный цикл
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

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

            ImGui::Text("Current Assets:");
            if (ImGui::BeginTable("AssetsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Quantity");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();
                for (const auto& asset : assets) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", asset.name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", asset.quantity);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f", asset.value());
                }
                ImGui::EndTable();
            }
            if (ImGui::Button("Save Portfolio")) savePortfolio();
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

        // Очистка
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