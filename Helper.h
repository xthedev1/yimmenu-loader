#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "includes.h"
#include <Windows.h>
#include <vector>
#include <string>
#include "fa_solid_900.h"
#include "font_awesome_5.h"
#include "imgui/imgui.h"
#include <wtypes.h>
#include "imgui_notify.h"
#include"nlohmann/json.hpp"
#include <fstream>

const int WINDOW_W = 754;
const int WINDOW_H = 618;
using json = nlohmann::json;

namespace Helper
{
	bool g_Running = true;
	bool ShowMainPage = false;
	bool ShowLoading = true;
	static int Tabs = 0;

	float AlphaAnimation = 0.0f;

	struct Fonts
	{
		ImFont* mainFont;
		ImFont* IconFont;
		ImFont* afont;
		ImFont* Boldfont;
		ImFont* Fontie;
	} fonts;

	void Style()
	{
		
		ImGuiStyle& style = ImGui::GetStyle();

		style.Colors[ImGuiCol_FrameBg] = ImColor(11, 11, 11, 255);
		style.Colors[ImGuiCol_FrameBgActive] = ImColor(18, 17, 17, 255);
		style.Colors[ImGuiCol_FrameBgHovered] = ImColor(18, 17, 17, 255);

		style.Colors[ImGuiCol_ChildBg] = ImColor(9, 9, 9, 255);
		style.Colors[ImGuiCol_Border] = ImColor(20, 20, 20, 255);
		style.Colors[ImGuiCol_Separator] = ImColor(21, 21, 21, 255);

		style.Colors[ImGuiCol_WindowBg] = ImColor(12, 12, 12, 255);

		style.FramePadding = ImVec2(12.0f, 12.0f);
		style.WindowPadding = ImVec2(12.0f, 12.0f);
		style.WindowRounding = 8.0f;
		style.ChildBorderSize = 0.5f;
		style.ChildRounding = 8.0f;
		return;
	}

	std::string getLocalAppDataPath() {
		char* localAppData;
		size_t len;
		if (_dupenv_s(&localAppData, &len, "LOCALAPPDATA") == 0 && localAppData != nullptr) {
			std::string result(localAppData);
			free(localAppData);
			return result;
		}
		else {
			return "";
		}
	}

	std::string tm_to_readable_time(tm ctx) {
		char buffer[80];

		strftime(buffer, sizeof(buffer), "%a %m/%d/%y %H:%M:%S %Z", &ctx);

		return std::string(buffer);
	}

	static std::time_t string_to_timet(std::string timestamp) {
		auto cv = strtol(timestamp.c_str(), NULL, 10);

		return (time_t)cv;
	}

	static std::tm timet_to_tm(time_t timestamp) {
		std::tm context;

		localtime_s(&context, &timestamp);

		return context;
	}

	std::string ReadFromJson(std::string path, std::string section)
	{
		if (!std::filesystem::exists(path))
			return "File Not Found";
		std::ifstream file(path);
		json data = json::parse(file);
		return data[section];
	}

	bool CheckIfJsonKeyExists(std::string path, std::string section)
	{
		if (!std::filesystem::exists(path))
			return false;
		std::ifstream file(path);
		json data = json::parse(file);
		return data.contains(section);
	}

	bool WriteToJson(std::string path, std::string name, std::string value, bool userpass, std::string name2, std::string value2)
	{
		json file;
		if (!userpass)
		{
			file[name] = value;
		}
		else
		{
			file[name] = value;
			file[name2] = value2;
		}

		std::ofstream jsonfile(path, std::ios::out);
		jsonfile << file;
		jsonfile.close();
		if (!std::filesystem::exists(path))
			return false;

		return true;
	}
}