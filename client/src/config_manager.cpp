#include "config_manager.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

#include "reflection/trajectory.hpp"

#include <fmt/base.h>
#include <gtk/gtk.h>
#include <imgui.h>

ConfigManager::ConfigManager()
{
    m_ballPrediction  = NULL;
    m_carBoostAmounts = NULL;
    m_carHitbox       = NULL;
    m_boostPadTimers  = NULL;
}

void ConfigManager::Initialize(BallPrediction &ballPrediction, CarBoostAmounts &carBoostAmounts, CarHitbox &carHitbox, BoostPadTimers &boostPadTimers)
{
    m_executableDirectory = std::filesystem::read_symlink("/proc/self/exe").parent_path();
    m_ballPrediction      = &ballPrediction;
    m_carBoostAmounts     = &carBoostAmounts;
    m_carHitbox           = &carHitbox;
    m_boostPadTimers      = &boostPadTimers;
    snprintf(m_collisionMeshesDirectory, sizeof(m_collisionMeshesDirectory), "%s", Trajectory::GetCollisionMeshesFolder().c_str());
    snprintf(m_profileName, sizeof(m_profileName), "default");
    LoadConfigDirectory();
    ApplyConfigDirectory();

    if (m_configNames.empty())
    {
        SaveConfig();
        RefreshConfigs();
        return;
    }

    snprintf(m_profileName, sizeof(m_profileName), "%s", m_configNames[0].c_str());
    LoadConfig();
}

void ConfigManager::DrawMenu()
{
    if (!ImGui::CollapsingHeader("Configurations", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::InputText("Config Folder", m_configDirectory, sizeof(m_configDirectory));
    ImGui::SameLine();
    if (ImGui::Button("Apply Config Folder"))
    {
        ApplyConfigDirectory();
    }

    ImGui::SameLine();
    if (ImGui::Button("Choose Config Folder"))
    {
        SelectConfigDirectory();
    }

    ImGui::InputText("Collision Meshes Folder", m_collisionMeshesDirectory, sizeof(m_collisionMeshesDirectory));
    ImGui::SameLine();
    if (ImGui::Button("Apply Collision Meshes Folder"))
    {
        ApplyCollisionMeshesDirectory();
    }

    ImGui::SameLine();
    if (ImGui::Button("Choose Collision Meshes Folder"))
    {
        SelectCollisionMeshesDirectory();
    }

    ImGui::TextUnformatted("Restart Nerine after changing the collision meshes folder.");

    if (ImGui::BeginCombo("Profile", m_profileName))
    {
        for (size_t i = 0; i < m_configNames.size(); i++)
        {
            bool selected = m_configNames[i] == m_profileName;
            if (ImGui::Selectable(m_configNames[i].c_str(), selected))
            {
                snprintf(m_profileName, sizeof(m_profileName), "%s", m_configNames[i].c_str());
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::InputText("Profile Name", m_profileName, sizeof(m_profileName));
    if (ImGui::Button("Load Profile"))
    {
        LoadConfig();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save Profile") && m_profileName[0])
    {
        SaveConfig();
        RefreshConfigs();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Profile"))
    {
        DeleteConfig();
    }
}

void ConfigManager::ApplyConfigDirectory()
{
    std::filesystem::create_directories(m_configDirectory);
    SaveConfigDirectory();
    RefreshConfigs();
}

void ConfigManager::ApplyCollisionMeshesDirectory()
{
    Trajectory::SetCollisionMeshesFolder(m_collisionMeshesDirectory);
}

void ConfigManager::DeleteConfig()
{
    for (size_t i = 0; i < m_configNames.size(); i++)
    {
        if (m_configNames[i] != m_profileName)
        {
            continue;
        }

        std::filesystem::remove(std::filesystem::path(m_configDirectory) / (m_configNames[i] + ".toml"));
        RefreshConfigs();
        if (m_configNames.empty())
        {
            m_profileName[0] = '\0';
            return;
        }

        snprintf(m_profileName, sizeof(m_profileName), "%s", m_configNames[0].c_str());
        LoadConfig();
        return;
    }
}

void ConfigManager::LoadConfigDirectory()
{
    std::filesystem::path configDirectoryFile = m_executableDirectory / "config_directory.toml";
    if (!std::filesystem::exists(configDirectoryFile))
    {
        snprintf(m_configDirectory, sizeof(m_configDirectory), "%s", (m_executableDirectory / "configs").c_str());
        return;
    }

    try
    {
        toml::table configDirectory = toml::parse_file(configDirectoryFile.c_str());
        std::string directory       = configDirectory["config_directory"].value_or("");
        snprintf(m_configDirectory, sizeof(m_configDirectory), "%s", directory.c_str());
    }
    catch (const toml::parse_error &error)
    {
        fmt::println(stderr, "Failed to load config directory: {}", error.description());
    }
}

void ConfigManager::LoadConfig()
{
    std::filesystem::path configFile = std::filesystem::path(m_configDirectory) / (std::string(m_profileName) + ".toml");
    if (!std::filesystem::exists(configFile))
    {
        return;
    }

    try
    {
        toml::table config                 = toml::parse_file(configFile.c_str());
        const toml::table *paths           = config["paths"].as_table();
        const toml::table *ballPrediction  = config["ball_prediction"].as_table();
        const toml::table *carBoostAmounts = config["car_boost_amounts"].as_table();
        const toml::table *carHitbox       = config["car_hitbox"].as_table();
        const toml::table *boostPadTimers  = config["boost_pad_timers"].as_table();

        if (paths)
        {
            std::string collisionMeshesDirectory = (*paths)["collision_meshes_directory"].value_or("");
            if (!collisionMeshesDirectory.empty())
            {
                snprintf(m_collisionMeshesDirectory, sizeof(m_collisionMeshesDirectory), "%s", collisionMeshesDirectory.c_str());
                ApplyCollisionMeshesDirectory();
            }
        }

        if (ballPrediction)
        {
            m_ballPrediction->LoadConfig(*ballPrediction);
        }

        if (carBoostAmounts)
        {
            m_carBoostAmounts->LoadConfig(*carBoostAmounts);
        }

        if (carHitbox)
        {
            m_carHitbox->LoadConfig(*carHitbox);
        }

        if (boostPadTimers)
        {
            m_boostPadTimers->LoadConfig(*boostPadTimers);
        }
    }
    catch (const toml::parse_error &error)
    {
        fmt::println(stderr, "Failed to load profile '{}': {}", m_profileName, error.description());
    }
}

void ConfigManager::RefreshConfigs()
{
    m_configNames.clear();

    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(m_configDirectory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".toml")
        {
            m_configNames.push_back(entry.path().stem().string());
        }
    }

    std::sort(m_configNames.begin(), m_configNames.end());
}

void ConfigManager::SaveConfig() const
{
    toml::table config;
    toml::table paths;
    toml::table ballPrediction;
    toml::table carBoostAmounts;
    toml::table carHitbox;
    toml::table boostPadTimers;

    paths.insert("collision_meshes_directory", m_collisionMeshesDirectory);
    m_ballPrediction->SaveConfig(ballPrediction);
    m_carBoostAmounts->SaveConfig(carBoostAmounts);
    m_carHitbox->SaveConfig(carHitbox);
    m_boostPadTimers->SaveConfig(boostPadTimers);
    config.insert("paths", std::move(paths));
    config.insert("ball_prediction", std::move(ballPrediction));
    config.insert("car_boost_amounts", std::move(carBoostAmounts));
    config.insert("car_hitbox", std::move(carHitbox));
    config.insert("boost_pad_timers", std::move(boostPadTimers));

    std::ofstream file(std::filesystem::path(m_configDirectory) / (std::string(m_profileName) + ".toml"));
    file << config;
}

void ConfigManager::SaveConfigDirectory() const
{
    toml::table configDirectory;

    configDirectory.insert("config_directory", m_configDirectory);

    std::ofstream file(m_executableDirectory / "config_directory.toml");
    file << configDirectory;
}

void ConfigManager::SelectCollisionMeshesDirectory()
{
    GtkFileDialog *dialog = gtk_file_dialog_new();
    GFile *initialFolder  = g_file_new_for_path(m_collisionMeshesDirectory);

    gtk_file_dialog_set_title(dialog, "Choose Collision Meshes Folder");
    gtk_file_dialog_set_initial_folder(dialog, initialFolder);
    gtk_file_dialog_select_folder(dialog, NULL, NULL, OnCollisionMeshesDirectorySelected, this);
    g_object_unref(initialFolder);
    g_object_unref(dialog);
}

void ConfigManager::SelectConfigDirectory()
{
    GtkFileDialog *dialog = gtk_file_dialog_new();
    GFile *initialFolder  = g_file_new_for_path(m_configDirectory);

    gtk_file_dialog_set_title(dialog, "Choose Config Folder");
    gtk_file_dialog_set_initial_folder(dialog, initialFolder);
    gtk_file_dialog_select_folder(dialog, NULL, NULL, OnConfigDirectorySelected, this);
    g_object_unref(initialFolder);
    g_object_unref(dialog);
}

void ConfigManager::OnCollisionMeshesDirectorySelected(GObject *source, GAsyncResult *result, gpointer data)
{
    ConfigManager *configManager = (ConfigManager *) data;
    GError *error                = NULL;
    GFile *folder                = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!folder)
    {
        g_error_free(error);
        return;
    }

    char *path = g_file_get_path(folder);

    snprintf(configManager->m_collisionMeshesDirectory, sizeof(configManager->m_collisionMeshesDirectory), "%s", path);
    g_free(path);
    g_object_unref(folder);
    configManager->ApplyCollisionMeshesDirectory();
}

void ConfigManager::OnConfigDirectorySelected(GObject *source, GAsyncResult *result, gpointer data)
{
    ConfigManager *configManager = (ConfigManager *) data;
    GError *error                = NULL;
    GFile *folder                = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!folder)
    {
        g_error_free(error);
        return;
    }

    char *path = g_file_get_path(folder);

    snprintf(configManager->m_configDirectory, sizeof(configManager->m_configDirectory), "%s", path);
    g_free(path);
    g_object_unref(folder);
    configManager->ApplyConfigDirectory();
}
