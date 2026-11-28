#include "imgui/imgui.h"
#include "stdinclude.hpp"
#include "scgui/scGUIData.hpp"
#include "scgui/scGUIAction.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <array>
#include <cmath>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <winrt/base.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "runtimeobject.lib")

extern HOOK_ORIG_TYPE SetResolution_orig;
// extern std::vector<std::pair<std::pair<int, int>, int>> replaceDressResIds;
extern std::map<std::string, CharaParam_t> charaParam;
extern CharaParam_t baseParam;
extern void saveGUIDataCache();
extern bool MvCenterCloseupsAnyCostume();
extern void SetMvCenterCloseupsAnyCostume(bool enabled);
extern void GetMvFocusFraming(float& roll, float& horizontal, float& vertical);
extern void SetMvFocusFraming(float roll, float horizontal, float vertical);
extern bool GetMvKeepFraming(bool fixed);
extern void SetMvKeepFraming(bool fixed, bool keep);
extern void ResetMvFraming(bool fixed);
extern void ResetMvFixedPanel(int panel);
extern void GetMvFixedFraming(float& yaw, float& pitch, float& roll, float& horizontal, float& vertical, float& zoom, int panel = -1);
extern void SetMvFixedFraming(float yaw, float pitch, float roll, float horizontal, float vertical, float zoom, int panel = -1);
extern bool GetMvSplitEnabled();
extern void SetMvSplitEnabled(bool enabled);
extern int GetMvSplitLayout();
extern void SetMvSplitLayout(int layout);
extern int GetMvSplitLayoutChoiceCount();
extern const char* GetMvSplitLayoutName(int layout);
extern int GetMvSplitVisibleCount();
extern bool IsMvSplitMixedLayout();
extern int GetMvSplitPanel();
extern void SetMvSplitPanel(int panel);
extern int GetMvSplitPanelNumber(int panel);
extern int GetMvSplitActorCount();
extern int GetMvSplitActor(int panel);
extern void SetMvSplitActor(int panel, int actor);
extern int GetMvSplitPreviewPane(int preview);
extern void SetMvSplitPreviewPane(int preview, int pane);
extern bool GetMvNormalFocusWindow();
extern void SetMvNormalFocusWindow(bool enabled);
extern int GetMvNormalFocusActor();
extern void SetMvNormalFocusActor(int actor);
extern void GetMvNormalFocusFraming(float& yaw, float& pitch, float& roll, float& horizontal, float& vertical, float& zoom);
extern void SetMvNormalFocusFraming(float yaw, float pitch, float roll, float horizontal, float vertical, float zoom);
extern void ResetMvNormalFocusFraming();
extern void RequestMvFormationTool(int action);
extern void RequestMvOutfitTable(int action);
extern void RequestRandomMv(std::vector<int> songs, std::vector<int> actors, int kinds, int policy,
	int count, int interval, bool preferSongOutfit, int outfitMode, int hairMode);
extern std::string GetMvFormationToolStatus();
extern std::array<std::string, 3> GetMvOutfitSources();
using MvPlaylistEntries = std::vector<std::pair<int, std::string>>;
using MvPlaylistGroups = std::vector<std::tuple<int, std::string, MvPlaylistEntries>>;
extern std::shared_ptr<const MvPlaylistGroups> GetMvPlaylistGroups(bool songs);
extern std::vector<std::pair<int, std::string>> GetMvSavedOutfits();


#define INPUT_AND_SLIDER_FLOAT(label, data, min, max) \
    ImGui::SetNextItemWidth(inputFloatWidth);\
    ImGui::InputFloat("##"##label, data);\
    ImGui::SameLine();\
    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth - 1.0f);\
    ImGui::SliderFloat(label, data, min, max)

#define FOR_INPUT_AND_SLIDER_FLOAT(label, data, min, max, hideIdName) \
    ImGui::SetNextItemWidth(inputFloatWidth);\
    ImGui::InputFloat(("##"##label + hideIdName).c_str(), data);\
    ImGui::SameLine();\
    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - inputFloatWidth - 1.0f);\
    ImGui::SliderFloat((label##"##" + hideIdName).c_str(), data, min, max)

#define HELP_TOOLTIP(label, text) \
    ImGui::TextDisabled(label); \
    if (ImGui::IsItemHovered()) { \
        ImGui::BeginTooltip(); \
        ImGui::Text(text); \
        ImGui::EndTooltip(); \
    }

namespace SCGUILoop {
	static float inputFloatWidth = 50.0f;

	struct FreeCamState {
		bool hasData = false;
		std::string name = "";
		float fov = 0.0f;
		struct { float x, y, z; } pos = { 0,0,0 };
		struct { float x, y, z; } lookAt = { 0,0,0 };
		bool overwriteClipPlane = false;
		float nearClipPlane = 0;
		float farClipPlane = 2;
	};

	rapidjson::Value SerializeFreeCamState(const FreeCamState& state, rapidjson::Document::AllocatorType& allocator) {
		rapidjson::Value obj(rapidjson::kObjectType);
		obj.AddMember("hasData", rapidjson::Value(state.hasData), allocator);
		if (!state.name.empty()) {
			obj.AddMember("name", rapidjson::Value(state.name.c_str(), allocator), allocator);
		}
		if (state.hasData) {
			obj.AddMember("fov", rapidjson::Value(state.fov), allocator);

			rapidjson::Value valPos(rapidjson::kArrayType);
			valPos.PushBack(state.pos.x, allocator);
			valPos.PushBack(state.pos.y, allocator);
			valPos.PushBack(state.pos.z, allocator);
			obj.AddMember("pos", valPos, allocator);

			rapidjson::Value valLookAt(rapidjson::kArrayType);
			valLookAt.PushBack(state.lookAt.x, allocator);
			valLookAt.PushBack(state.lookAt.y, allocator);
			valLookAt.PushBack(state.lookAt.z, allocator);
			obj.AddMember("lookAt", valLookAt, allocator);

			if (g_reenable_clipPlane) {
				obj.AddMember("overwriteClipPlane", rapidjson::Value(true), allocator);
				obj.AddMember("nearClipPlane", rapidjson::Value(g_nearClipPlane), allocator);
				obj.AddMember("farClipPlane", rapidjson::Value(g_farClipPlane), allocator);
			}
		}
		return obj;
	}
	FreeCamState DeserializeFreeCameraState(const rapidjson::Value& v) {
		FreeCamState state;
		if (v.HasMember("hasData") && v["hasData"].IsBool()) state.hasData = v["hasData"].GetBool();
		if (v.HasMember("name") && v["name"].IsString()) state.name = v["name"].GetString();
		if (state.hasData) {
			if (v.HasMember("fov") && v["fov"].IsNumber()) state.fov = v["fov"].GetFloat();
			if (v.HasMember("pos") && v["pos"].IsArray()) {
				const auto& arr = v["pos"].GetArray();
				auto length = arr.Size();
				if (length > 0 && arr[0].IsNumber()) state.pos.x = arr[0].GetFloat();
				if (length > 1 && arr[1].IsNumber()) state.pos.y = arr[1].GetFloat();
				if (length > 2 && arr[2].IsNumber()) state.pos.z = arr[2].GetFloat();
			}
			if (v.HasMember("lookAt") && v["lookAt"].IsArray()) {
				const auto& arr = v["lookAt"].GetArray();
				auto length = arr.Size();
				if (length > 0 && arr[0].IsNumber()) state.lookAt.x = arr[0].GetFloat();
				if (length > 1 && arr[1].IsNumber()) state.lookAt.y = arr[1].GetFloat();
				if (length > 2 && arr[2].IsNumber()) state.lookAt.z = arr[2].GetFloat();
			}
			if (v.HasMember("overwriteClipPlane") && v["overwriteClipPlane"].IsBool() && v["overwriteClipPlane"].GetBool()) {
				state.overwriteClipPlane = true;
				if (v.HasMember("nearClipPlane") && v["nearClipPlane"].IsFloat()) state.nearClipPlane = v["nearClipPlane"].GetFloat();
				if (v.HasMember("farClipPlane") && v["farClipPlane"].IsFloat()) state.nearClipPlane = v["farClipPlane"].GetFloat();
			}
		}
		return state;
	}

	rapidjson::Value SerializeUnitIdol(const UnitIdol& idol, rapidjson::Document::AllocatorType& allocator) {
		rapidjson::Value obj(rapidjson::kObjectType);
		obj.AddMember("charaId", rapidjson::Value(idol.CharaId), allocator);
		if (idol.CharaId >= 0) {
			obj.AddMember("clothId", rapidjson::Value(idol.ClothId), allocator);
			obj.AddMember("hairId", rapidjson::Value(idol.HairId), allocator);
			if (idol.CostumeStatusLoaded) {
				obj.AddMember("costumeMstCostumeId", rapidjson::Value(idol.CostumeMstCostumeId), allocator);
				obj.AddMember("costumeMstCharacterInfoId", rapidjson::Value(idol.CostumeMstCharacterInfoId), allocator);
				obj.AddMember("costumeType", rapidjson::Value(idol.CostumeType), allocator);
				obj.AddMember("costumeResourceId", rapidjson::Value(idol.CostumeResourceId), allocator);
			}

			rapidjson::Value accessoryIds(rapidjson::kArrayType);
			for (int i = 0; i < idol.AccessoryIds.size(); ++i) {
				accessoryIds.PushBack(idol.AccessoryIds[i], allocator);
			}
			obj.AddMember("accessoryIds", accessoryIds, allocator);
		}

		return obj;
	}
	UnitIdol DeserializeUnitIdol(const rapidjson::Value& v) {
		UnitIdol idol;
		if (v.HasMember("charaId") && v["charaId"].IsInt()) idol.CharaId = v["charaId"].GetInt();
		if (idol.CharaId >= 0) {
			if (v.HasMember("clothId") && v["clothId"].IsInt()) idol.ClothId = v["clothId"].GetInt();
			if (v.HasMember("hairId") && v["hairId"].IsInt()) idol.HairId = v["hairId"].GetInt();
			if (v.HasMember("costumeMstCostumeId") && v["costumeMstCostumeId"].IsInt()) idol.CostumeMstCostumeId = v["costumeMstCostumeId"].GetInt();
			if (v.HasMember("costumeMstCharacterInfoId") && v["costumeMstCharacterInfoId"].IsInt()) idol.CostumeMstCharacterInfoId = v["costumeMstCharacterInfoId"].GetInt();
			if (v.HasMember("costumeType") && v["costumeType"].IsInt()) idol.CostumeType = v["costumeType"].GetInt();
			if (v.HasMember("costumeResourceId") && v["costumeResourceId"].IsInt()) idol.CostumeResourceId = v["costumeResourceId"].GetInt();
			idol.CostumeStatusLoaded = idol.CostumeMstCostumeId >= 0 && idol.CostumeMstCharacterInfoId >= 0;
			if (v.HasMember("accessoryIds") && v["accessoryIds"].IsArray()) {
				const auto& arr = v["accessoryIds"].GetArray();
				idol.AccessoryIds.resize(arr.Size());
				for (int i = 0; i < arr.Size(); ++i) {
					idol.AccessoryIds[i] = arr[i].GetInt();
				}
			}
		}
		return idol;
	}


	static std::vector<FreeCamState> freeCamSlots(1);
	char newFreeCamSlotName[32]{};

	// call ShowMessageBox(...) function instead of setting this varaible directly
	std::string ___messageBoxContent{};
	// call ShowMessageBox(...) function instead of setting this varaible directly
	bool ___showMessageBox = false;
	void ShowMessageBox(const std::string& message) {
		___messageBoxContent = message;
		___showMessageBox = true;
	}

	// call ShowInputPopup(...) function instead of setting this varaible directly
	bool ___showInputPopup = false;
	// call ShowInputPopup(...) function instead of setting this varaible directly
	std::string ___inputPopupMessage{};
	// call ShowInputPopup(...) function instead of setting this varaible directly
	std::function< void(bool isOk, std::string input)> ___inputPopupCallback = nullptr;
	// don't access this varaible directly
	char ___inputPopupBuffer[1024]{};
	void ShowInputPopup(std::string message, std::string initValue, std::function<void(bool isOk, std::string input)> callback) {
		if (___inputPopupCallback != nullptr) {
			std::cerr << "[InputPopup] The popup is already open." << std::endl;
		}
		else {
			___inputPopupCallback = callback;
			___inputPopupMessage = message;
			___showInputPopup = true;
			snprintf(___inputPopupBuffer, sizeof(___inputPopupBuffer), "%s", initValue.c_str());
		}
	}

	void charaParamEditLoop() {
		if (ImGui::Begin("Character Parameter Edit")) {
			if (ImGui::CollapsingHeader("Sway offset (Non real-time, requires reloading)")) {
				static int currentEdit = 0x7;  // BreastPointed

				if (ImGui::BeginCombo("Edit Type", swayTypes[currentEdit].c_str())) {
					for (const auto& pair : swayTypes) {
						bool isSelected = (currentEdit == pair.first);
						if (ImGui::Selectable(pair.second.c_str(), isSelected)) {
							currentEdit = pair.first;
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				auto& currEditData = charaSwayStringOffset[currentEdit];

				INPUT_AND_SLIDER_FLOAT("Rate##sway", &currEditData.rate, -1.5, 1.5);
				INPUT_AND_SLIDER_FLOAT("BendStrength##sway", &currEditData.P_bendStrength, -5, 5.0);
				INPUT_AND_SLIDER_FLOAT("BaseGravity##sway", &currEditData.P_baseGravity, -50, 50.0);
				INPUT_AND_SLIDER_FLOAT("InertiaMoment##sway", &currEditData.P_inertiaMoment, -2, 2.0);
				INPUT_AND_SLIDER_FLOAT("AirResistance##sway", &currEditData.P_airResistance, -2, 2.0);
				INPUT_AND_SLIDER_FLOAT("DeformResistance##sway", &currEditData.P_deformResistance, -30, 30.0);

				if (ImGui::Button("Reset##sway")) {
					currEditData.rate = 0;
					currEditData.P_bendStrength = 0;
					currEditData.P_baseGravity = 0;
					currEditData.P_inertiaMoment = 0;
					currEditData.P_airResistance = 0;
					currEditData.P_deformResistance = 0;
				}
				ImGui::SameLine();
				if (ImGui::Button("Save##sway")) {
					saveGUIDataCache();
				}
				ImGui::NewLine();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("SwayString 偏移调整。加载角色时生效，非实时生效。");
				ImGui::EndTooltip();
			}

			bool baseApply = false;
			bool resetAll = false;
			if (ImGui::CollapsingHeader("Base Offset", ImGuiTreeNodeFlags_DefaultOpen)) {
				INPUT_AND_SLIDER_FLOAT("Height##base", &baseParam.height, -1.5, 1.5);
				INPUT_AND_SLIDER_FLOAT("Bust##base", &baseParam.bust, -1.5, 1.5);
				INPUT_AND_SLIDER_FLOAT("Head##base", &baseParam.head, -1.5, 1.5);
				INPUT_AND_SLIDER_FLOAT("Arm##base", &baseParam.arm, -1.5, 1.5);
				INPUT_AND_SLIDER_FLOAT("Hand##base", &baseParam.hand, -1.5, 1.5);
				if (ImGui::Button("Apply##base")) {
					baseApply = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset Base##base")) {
					baseParam.Reset();
					baseApply = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset All##base")) {
					baseParam.Reset();
					resetAll = true;
					baseApply = true;
				}
				ImGui::SameLine();
				if (ImGui::Checkbox("Real-time Update##base", &baseParam.gui_real_time_apply)) {
					if (baseParam.gui_real_time_apply) {
						baseApply = true;
					}
				}
			}

			for (auto& i : charaParam) {
				if (i.second.checkObjAlive()) {
					if (ImGui::CollapsingHeader(i.first.c_str())) {
						FOR_INPUT_AND_SLIDER_FLOAT("Height", &i.second.height, 0.0, 2.0, i.first);
						FOR_INPUT_AND_SLIDER_FLOAT("Bust", &i.second.bust, 0.0, 2.0, i.first);
						FOR_INPUT_AND_SLIDER_FLOAT("Head", &i.second.head, 0.0, 2.0, i.first);
						FOR_INPUT_AND_SLIDER_FLOAT("Arm", &i.second.arm, 0.0, 2.0, i.first);
						FOR_INPUT_AND_SLIDER_FLOAT("Hand", &i.second.hand, 0.0, 2.0, i.first);
						if (ImGui::Button(("Apply##" + i.first).c_str()) || baseApply) {
							i.second.ApplyOnMainThread();
						}
						ImGui::SameLine();
						if (ImGui::Button(("Reset##" + i.first).c_str()) || resetAll) {
							i.second.Reset();
							i.second.ApplyOnMainThread();
						}
						ImGui::SameLine();
						if (ImGui::Checkbox(("Real-time Update##" + i.first).c_str(), &i.second.gui_real_time_apply)) {
							if (i.second.gui_real_time_apply) {
								i.second.ApplyOnMainThread();
							}
						}
					}
					else {
						if (baseApply) {
							if (resetAll) {
								i.second.Reset();
							}
							i.second.ApplyOnMainThread();
						}
					}
				}
			}
		}
		ImGui::End();
	}

	void savedCostumeDataLoop() {
		if (ImGui::Begin("Saved Costume Data")) {
			for (auto it = savedCostumes.begin(); it != savedCostumes.end(); ) {
				char label[32];

				std::snprintf(label, sizeof(label), "%d", it->first);
				ImGui::Text(label);
				ImGui::SameLine();

				std::snprintf(label, sizeof(label), "%s##%s%d", "Remove", "scd", it->first);
				auto strUnitIdol = it->second.ToString();

				if (ImGui::Button(label)) {
					it = savedCostumes.erase(it);
				}
				else {
					++it;
				}

				ImGui::SameLine();
				ImGui::Text(strUnitIdol.c_str());
			}
		}
		ImGui::End();
	}

	int editingOverrideMvUnitIdolSlot = -1;
	char inputOverrideMvUnitIdol[1024] = "";
	void overrideMvUnitIdolLoop() {
		if (ImGui::Begin("Override MvUnit Idols")) {

			// export, append; remove all
			ImGui::Dummy(ImVec2(40, 0));
			ImGui::SameLine();
			if (ImGui::Button("Export to clipboard##overwriteIdols")) {
				rapidjson::Document doc;
				doc.SetArray();
				auto& allocator = doc.GetAllocator();
				for (auto& idol : overridenMvUnitIdols) {
					doc.PushBack(SerializeUnitIdol(idol, allocator), allocator);
				}
				rapidjson::StringBuffer buffer;
				rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
				doc.Accept(writer);
				std::string serialized(buffer.GetString());
				auto r = WriteClipboard(serialized);
				ShowMessageBox((r ? "Success" : "Failed"));
			}
			ImGui::SameLine();
			if (ImGui::Button("Overwrite from clipboard##overwriteIdols")) {
				std::string text;
				if (!ReadClipboard(&text)) {
					ShowMessageBox("Failed to read clipboard.");
				}
				else {
					rapidjson::Document doc;
					doc.Parse(text.c_str());
					if (doc.IsArray()) {
						const auto& arr = doc.GetArray();
						const auto length = std::min((int)arr.Size(), overridenMvUnitIdols_length);
						for (int i = 0; i < length; ++i) {
							auto parsed = DeserializeUnitIdol(arr[i]);
							overridenMvUnitIdols[i] = parsed;
						}
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Remove all##overwriteIdols")) {
				for (int i = 0; i < overridenMvUnitIdols_length; ++i) {
					overridenMvUnitIdols[i].Clear();
				}
			}

			for (int i = 0; i < overridenMvUnitIdols_length; ++i) {
				char label[1024];

				std::snprintf(label, sizeof(label), "%s##%s%d", "Remove", "omui", i);
				if (ImGui::Button(label)) {
					overridenMvUnitIdols[i].Clear();
				}
				ImGui::SameLine();
				std::snprintf(label, sizeof(label), "%s%d##%s", "Slot ", i, "omui");
				if (ImGui::Button(label)) {
					if (lastSavedCostume.IsEmpty()) {
						printf("No costume data saved yet.\n");
					}
					else {
						overridenMvUnitIdols[i] = lastSavedCostume;
					}
				}
				ImGui::SameLine();
				auto strIdolData = overridenMvUnitIdols[i].ToString();
				std::snprintf(label, sizeof(label), "%s##%d", strIdolData.c_str(), i);
				if (ImGui::Button(label)) {
					editingOverrideMvUnitIdolSlot = i;
					std::strncpy(inputOverrideMvUnitIdol, strIdolData.c_str(), sizeof(inputOverrideMvUnitIdol) - 1);
					inputOverrideMvUnitIdol[sizeof(inputOverrideMvUnitIdol) - 1] = '\0'; // Ensure null-termination
				}
			}
		}
		ImGui::End();

		if (editingOverrideMvUnitIdolSlot >= 0) {
			ImGui::OpenPopup("InputManuallyOverrideMvUnitIdol");
		}
		if (ImGui::BeginPopupModal("InputManuallyOverrideMvUnitIdol", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Set override data manually: (in JSON)");
			ImGui::InputText("##NameInput", inputOverrideMvUnitIdol, IM_ARRAYSIZE(inputOverrideMvUnitIdol));

			if (ImGui::Button("OK", ImVec2(120, 0))) {
				if (editingOverrideMvUnitIdolSlot >= 0 && editingOverrideMvUnitIdolSlot < 8) {
					overridenMvUnitIdols[editingOverrideMvUnitIdolSlot].LoadJson(inputOverrideMvUnitIdol);
				}
				else {
					printf("ArgumentOutOfRangeException at `overridenMvUnitIdols[editingOverrideMvUnitIdolSlot]`; editingOverrideMvUnitIdolSlot = %d.\n", editingOverrideMvUnitIdolSlot);
				}
				editingOverrideMvUnitIdolSlot = -1;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				editingOverrideMvUnitIdolSlot = -1;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void overrideRunwayMotionLoop() {
		if (ImGui::Begin("Override Runway Motions")) {

			ImGui::Indent(20);
			ImGui::Text("Known Runway Motions (updates when entering motion changing GUI)");
			ImGui::Unindent(20);

			if (ImGui::BeginTable("##KnownRunwayMotions", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
				ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("IsWalking", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("IsPose", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableHeadersRow();
				for (auto& [id, motion] : knownRunwayMotions) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%d", id);
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(motion.Name.c_str());
					ImGui::TableSetColumnIndex(2);
					ImGui::TextUnformatted(motion.IsWalking ? "O" : "X");
					ImGui::TableSetColumnIndex(3);
					ImGui::TextUnformatted(motion.IsPose ? "O" : "X");
				}
				ImGui::EndTable();
			}

			int positionOrder[] = { 4, 2, 0, 1, 3 };
			int fixedOrderLength = 5;
			int columnCount = overrideRunwayMotionData.size();

			static float motionInputWidth = ImGui::CalcTextSize("00").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			
			ImGui::Indent(20);
			ImGui::Text("Overriding values (0 for unchanging)");
			ImGui::Unindent(20);

			if (ImGui::BeginTable("##OverrideRunwayMotions", columnCount + 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
				ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed);
				for (int i = 0; i < columnCount; ++i) {
					ImGui::TableSetupColumn("Motion", ImGuiTableColumnFlags_WidthFixed, motionInputWidth, i + 1);
				}
				ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed);

				ImGui::TableNextRow();
				for (int i = 0; i < columnCount; ++i) {
					int dataIndex = i < fixedOrderLength ? positionOrder[i] : i;
					ImGui::TableSetColumnIndex(i + 1);
					ImGui::Text("[%d]", dataIndex);
				}

				ImGui::TableSetColumnIndex(columnCount + 1);
				if (ImGui::Button("+##AddRunwayMotion")) {
					overrideRunwayMotionData.push_back({ 0, 0 });
				}

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted("walking");
				for (int i = 0; i < columnCount; ++i) {
					int dataIndex = i < fixedOrderLength ? positionOrder[i] : i;
					ImGui::TableSetColumnIndex(i + 1);
					ImGui::PushID(dataIndex);
					ImGui::SetNextItemWidth(motionInputWidth);
					ImGui::InputScalar("##walking", ImGuiDataType_S32, &overrideRunwayMotionData[dataIndex].WalkingMotionId, nullptr, nullptr, "%d");
					ImGui::PopID();
				}

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted("pose");
				for (int i = 0; i < columnCount; ++i) {
					int dataIndex = i < fixedOrderLength ? positionOrder[i] : i;
					ImGui::TableSetColumnIndex(i + 1);
					ImGui::PushID(dataIndex);
					ImGui::SetNextItemWidth(motionInputWidth);
					ImGui::InputScalar("##pose", ImGuiDataType_S32, &overrideRunwayMotionData[dataIndex].PoseMotionId, nullptr, nullptr, "%d");
					ImGui::PopID();
				}

				ImGui::TableSetColumnIndex(columnCount + 1);
				ImGui::BeginDisabled(overrideRunwayMotionData.size() <= fixedOrderLength);
				if (ImGui::Button("-##RemoveRunwayMotion")) {
					overrideRunwayMotionData.pop_back();
				}
				ImGui::EndDisabled();

				ImGui::EndTable();
			}
		}
		ImGui::End();
	}


	void PosesLoop();
	int selectedPoseIndex = -1;
	struct ScannedGameObjectData {
		Il2CppObject* gameObject;
		std::string objName;
		std::string displayName;

		explicit operator bool() const {
			return reflection::UnityObject_op_Implicit(gameObject);
		}
	};
	std::vector<ScannedGameObjectData> scannedGameObjects{};

	void devTabLoop();

	void mainLoop() {
		if (ImGui::Begin("SC Plugin Config")) {
			if (ImGui::Button("Reload Config And Translate Data")) {
				g_reload_all_data();
			}
			ImGui::Checkbox("Waiting Extract Text", &SCGUIData::needExtractText);

			ImGui::Checkbox("Save & Replace costume changes", &g_save_and_replace_costume_changes);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "保存服装编辑信息并在MV播放时替换。\nSave costumes changing data and Replace when MV starts.");

			ImGui::Checkbox("Override MV unit idols", &g_overrie_mv_unit_idols);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "在操作窗口中保存用于替换MV播放时的角色信息。\nSave idols' data in control panel to replace them when playing MV.");

			ImGui::Checkbox("Show hidden costumes (overwrite isAllDressOrdered)", &g_show_hidden_costumes);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "在进入换装窗口前勾选有效。\nActive only when checked before entering costume changing view.");

			bool anyCostume = MvCenterCloseupsAnyCostume();
			if (ImGui::Checkbox("MV center close-ups with any costume", &anyCostume)) {
				SetMvCenterCloseupsAnyCostume(anyCostume);
			}

			ImGui::Checkbox("Apply costume changes automatically", &g_apply_costumes_automatically);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "勾选后将自动应用所选服装（如用于Photo Studio），为安全起见将禁止确认所选服装。\nSelected costumes will be applied automatically (like in Photo Studio), and Confirm button will be banned for safety reason.");

			ImGui::Checkbox("Enable VocalSeparatedOn forcibly", &g_override_isVocalSeparatedOn);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "若最终播放MV时的歌曲或所选角色不支持分段演唱则会导致卡死或崩溃。\nGame will freeze or crash if finally selected song or idols don't support separated vocal.");

			ImGui::Checkbox("Enable Character Parameter Editor", &g_enable_chara_param_edit);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "启用角色身体参数编辑器");

			ImGui::Checkbox("Unlock Stories (Dangerous)", &g_unlock_PIdol_and_SChara_events);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "阅读故事内容时会上传故事ID，理论上可追查非法数据。\nStory id will be uploaded when reading, and the invalid data can be tracked technically.");

			ImGui::Checkbox("Unlock PhotoStudio", &g_unlock_photo_studio);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "解锁Photo Studio中的内容。\nUnlock contents in Photo Studio.\n\n> config 'unlockPhotoStudio'");

			ImGui::Checkbox("Override runway motions", &g_override_runway_motion);
			ImGui::SameLine();
			HELP_TOOLTIP("(?)", "在新窗口中输入动作ID进行覆盖。\nInput Motion IDs in the new window to override.");


			if (ImGui::CollapsingHeader("Resolution Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("Window Resolution Settings");

				if (ImGui::Button("720P")) {
					SCGUIData::screenW = 1280;
					SCGUIData::screenH = 720;
				}
				ImGui::SameLine();
				if (ImGui::Button("1080P")) {
					SCGUIData::screenW = 1920;
					SCGUIData::screenH = 1080;
				}
				ImGui::SameLine();
				if (ImGui::Button("1440P")) {
					SCGUIData::screenW = 2560;
					SCGUIData::screenH = 1440;
				}
				ImGui::SameLine();
				if (ImGui::Button("1620P")) {
					SCGUIData::screenW = 2880;
					SCGUIData::screenH = 1620;
				}
				ImGui::SameLine();
				if (ImGui::Button("2160P")) {
					SCGUIData::screenW = 3840;
					SCGUIData::screenH = 2160;
				}

				ImGui::InputInt("Width", &SCGUIData::screenW);
				ImGui::InputInt("Height", &SCGUIData::screenH);
				ImGui::Checkbox("Full Screen", &SCGUIData::screenFull);
				if (ImGui::Button("Update Resolution")) {
					if (SetResolution_orig) {
						(reinterpret_cast<void (*)(int, int, bool)>HOOK_GET_ORIG(SetResolution))(SCGUIData::screenW, SCGUIData::screenH, SCGUIData::screenFull);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Swap")) {
					int temp = SCGUIData::screenW;
					SCGUIData::screenW = SCGUIData::screenH;
					SCGUIData::screenH = temp;
					if (SetResolution_orig) {
						(reinterpret_cast<void (*)(int, int, bool)>HOOK_GET_ORIG(SetResolution))(SCGUIData::screenW, SCGUIData::screenH, SCGUIData::screenFull);
					}
				}

				ImGui::Separator();

				INPUT_AND_SLIDER_FLOAT("3D Resolution Scale", &g_3d_resolution_scale, 0.1f, 5.0f);
				if (g_3d_resolution_scale == 1.0f) {
					SCCamera::currRenderResolution.x = SCGUIData::screenW;
					SCCamera::currRenderResolution.y = SCGUIData::screenH;
				}
				ImGui::Text("Current 3D Resolution: %d, %d", SCCamera::currRenderResolution.x, SCCamera::currRenderResolution.y);
			}

			if (ImGui::CollapsingHeader("Camera Info", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::InputFloat("Game Camera FOV", &SCGUIData::sysCamFov);
				ImGui::InputFloat3("Game Camera Pos (x, y, z)", &SCGUIData::sysCamPos.x);
				ImGui::InputFloat3("Game Camera LookAt (x, y, z)", &SCGUIData::sysCamLookAt.x);
				ImGui::InputFloat4("Game Camera Rotation (w, x, y, z)", &SCGUIData::sysCamRot.w);

				const auto framingControls = [](bool fixed, int panel = -1, bool normalFocus = false) {
					float yaw = 0, pitch = 0, zoom = 0;
					float roll, horizontal, vertical;
					bool changed = false;
					if (fixed) {
						if (normalFocus) GetMvNormalFocusFraming(yaw, pitch, roll, horizontal, vertical, zoom);
						else GetMvFixedFraming(yaw, pitch, roll, horizontal, vertical, zoom, panel);
						const bool orbit = panel >= 0;
						const float maxPitch = orbit ? 180.0f : 90.0f;
						changed |= ImGui::SliderFloat(orbit ? "Zoom toward actor" : "Zoom", &zoom,
							-3.0f, 3.0f, "%+.2f stops");
						changed |= ImGui::SliderFloat(orbit ? "Orbit left / right" : "Turn left / right", &yaw,
							-180.0f, 180.0f, "%.0f deg");
						changed |= ImGui::SliderFloat(orbit ? "Orbit below / above" : "Look down / up", &pitch,
							-maxPitch, maxPitch, "%.0f deg");
					} else {
						GetMvFocusFraming(roll, horizontal, vertical);
					}
					changed |= ImGui::SliderFloat("Roll (clockwise)", &roll, -180.0f, 180.0f, "%.0f deg");
					changed |= ImGui::SliderFloat("Move camera left / right", &horizontal, -3.0f, 3.0f, "%.2f");
					changed |= ImGui::SliderFloat("Move camera down / up", &vertical, -3.0f, 3.0f, "%.2f");
					if (ImGui::Button("90 deg left")) { roll = -90.0f; changed = true; }
					ImGui::SameLine();
					if (ImGui::Button("90 deg right")) { roll = 90.0f; changed = true; }
					ImGui::SameLine();
					if (ImGui::Button("Reset state")) {
						if (normalFocus) ResetMvNormalFocusFraming();
						else if (panel >= 0) ResetMvFixedPanel(panel);
						else ResetMvFraming(fixed);
					}
					else if (changed) {
						if (normalFocus) SetMvNormalFocusFraming(yaw, pitch, roll, horizontal, vertical, zoom);
						else if (fixed) SetMvFixedFraming(yaw, pitch, roll, horizontal, vertical, zoom, panel);
						else SetMvFocusFraming(roll, horizontal, vertical);
					}
					bool keep = GetMvKeepFraming(fixed);
					if (ImGui::Checkbox("Keep state after leaving MV", &keep)) SetMvKeepFraming(fixed, keep);
				};
				if (ImGui::CollapsingHeader("Focus framing", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::PushID("Focus framing");
					HELP_TOOLTIP("Controls (?)", "I / K / J / L: move camera up / down / left / right");
					framingControls(false);
					ImGui::PopID();
				}
				if (ImGui::TreeNode("正常自动镜头：竖向聚焦窗")) {
					HELP_TOOLTIP("窗口控制 (?)", "右键拖动 / WASD：环绕角色\n滚轮 / Z 和 X：缩放\nI / K / J / L：上下左右平移\nShift + 右键拖动 / Shift + WASD：平移\nQ 和 E：滚转\nR：重置");
					bool enabled = GetMvNormalFocusWindow();
					if (ImGui::Checkbox("单独打开竖向聚焦窗口", &enabled)) SetMvNormalFocusWindow(enabled);
					if (enabled) {
						int slots = std::max(1, GetMvSplitActorCount());
						int actor = std::clamp(GetMvNormalFocusActor(), 0, slots - 1);
						if (ImGui::SliderInt("聚焦角色站位", &actor, 0, slots - 1)) SetMvNormalFocusActor(actor);
						ImGui::TextDisabled("普通自动镜头保持原样；聚焦镜头以竖屏独立窗口显示。");
						ImGui::PushID("Normal vertical focus window");
						framingControls(true, 0, true);
						ImGui::PopID();
					}
					ImGui::TreePop();
				}
				if (ImGui::CollapsingHeader("Fixed framing")) {
					ImGui::PushID("Fixed framing");
					HELP_TOOLTIP("Controls (?)", "Right-drag / WASD: turn\nWheel / Z and X: zoom\nI / K / J / L: move camera up / down / left / right\nShift + right-drag / Shift + WASD: move\nQ and E: roll\nR: reset");
					bool split = GetMvSplitEnabled();
					if (ImGui::Checkbox("Multi-camera split screen", &split)) SetMvSplitEnabled(split);
					int panel = -1;
					if (split) {
						const int layout = GetMvSplitLayout();
						if (ImGui::BeginCombo("Layout", GetMvSplitLayoutName(layout))) {
							for (int i = 0; i < GetMvSplitLayoutChoiceCount(); ++i) {
								if (ImGui::Selectable(GetMvSplitLayoutName(i), i == layout)) SetMvSplitLayout(i);
							}
							ImGui::EndCombo();
						}
						const int panes = GetMvSplitVisibleCount();
						panel = GetMvSplitPanel();
						ImGui::TextUnformatted("Edit panel");
						for (int i = 0; i < panes; ++i) {
							ImGui::SameLine();
							const char label[]{static_cast<char>('0' + GetMvSplitPanelNumber(i)), '\0'};
							if (ImGui::RadioButton(label, panel == i)) {
								SetMvSplitPanel(i);
								panel = i;
							}
						}
						const int actorCount = GetMvSplitActorCount();
						if (actorCount > 0) {
							int actor = GetMvSplitActor(panel) + 1;
							if (actor > actorCount) actor = 1;
							if (ImGui::SliderInt("Actor slot", &actor, 1, actorCount)) SetMvSplitActor(panel, actor - 1);
							if (IsMvSplitMixedLayout() && ImGui::Button("Follow this actor in every pane"))
								for (int i = 0; i < panes; ++i) SetMvSplitActor(i, actor - 1);
							for (int i = 0; i < 2; ++i) {
								const int output = std::min(GetMvSplitPreviewPane(i), panes - 1);
								const auto label = output < 0 ? std::string("Off") : "Panel " + std::to_string(GetMvSplitPanelNumber(output));
								if (!ImGui::BeginCombo(i == 0 ? "Detached preview A" : "Detached preview B", label.c_str())) continue;
								if (ImGui::Selectable("Off", output < 0)) SetMvSplitPreviewPane(i, -1);
								for (int j = 0; j < panes; ++j) {
									const auto name = "Panel " + std::to_string(GetMvSplitPanelNumber(j));
									if (ImGui::Selectable(name.c_str(), output == j)) SetMvSplitPreviewPane(i, j);
								}
								ImGui::EndCombo();
							}
						}
						HELP_TOOLTIP("Follow actor (?)", "Use the game's numbered fixed-camera buttons to choose the actor for the selected panel.");
						HELP_TOOLTIP("Orbit controls (?)", "Right-drag / WASD: orbit around the actor\nWheel / Z and X: move closer or farther\nI / K / J / L: move the camera up / down / left / right");
					}
					framingControls(true, panel);
					ImGui::PopID();
				}

				if (ImGui::TreeNode("全员换装")) {
					static int outfitSource = 0;
					static int outfitStyle = 0;
					static int hairStyle = 0;
					const auto sources = GetMvOutfitSources();
					ImGui::Combo("换装来源", &outfitSource, "MV 首位\0服装预览\0");
					if (!sources[1].empty()) ImGui::TextWrapped("当前预览　%s", sources[1].c_str());
					if (ImGui::Button("保存当前预览服装")) RequestMvFormationTool(1);
					const auto& source = outfitSource ? (sources[1].empty() ? sources[2] : sources[1]) : sources[0];
					ImGui::TextWrapped("已选来源　%s", source.empty() ? "请先选择服装" : source.c_str());
					ImGui::Combo("服装", &outfitStyle, "优先同系列本人款\0可借穿时用来源款\0");
					ImGui::Combo("发型与配饰", &hairStyle, "优先使用该套服装的搭配\0使用偶像默认搭配\0");
					if (ImGui::Button("全员换装"))
						RequestMvFormationTool(16 + outfitSource + (outfitStyle ? 2 : 0) + (hairStyle ? 0 : 4));
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("随机歌曲")) {
					static std::unordered_map<int, bool> songs, idols;
					const auto groupPicker = [](const char* label, bool isSong, auto& checked) {
						std::vector<int> selected;
						const auto groups = GetMvPlaylistGroups(isSong);
						if (!groups || groups->empty()) {
							ImGui::TextDisabled("%s", isSong ? "打开 MV 歌曲列表后可选歌曲" : "正在读取偶像");
							return selected;
						}
						if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen)) {
							if (ImGui::Button("全选")) {
								for (const auto& group : *groups)
									for (const auto& item : std::get<2>(group)) checked[item.first] = true;
							}
							ImGui::SameLine();
							if (ImGui::Button("清空")) {
								for (const auto& group : *groups)
									for (const auto& item : std::get<2>(group)) checked[item.first] = false;
							}
							for (const auto& [group, name, items] : *groups) {
								ImGui::PushID(group);
								const auto selectedCount = std::count_if(items.begin(), items.end(), [&](const auto& item) {
									return checked.try_emplace(item.first, true).first->second;
								});
								bool entireGroup = static_cast<size_t>(selectedCount) == items.size();
								if (ImGui::Checkbox("##select-group", &entireGroup))
									for (const auto& item : items) checked[item.first] = entireGroup;
								ImGui::SameLine();
								if (ImGui::TreeNodeEx("##group-entries", ImGuiTreeNodeFlags_SpanAvailWidth, "%s (%d/%d)",
									name.c_str(), static_cast<int>(selectedCount), static_cast<int>(items.size()))) {
									for (const auto& [id, item] : items) {
										ImGui::PushID(id);
										ImGui::Checkbox(item.c_str(), &checked.at(id));
										ImGui::PopID();
									}
									ImGui::TreePop();
								}
								ImGui::PopID();
							}
							ImGui::TreePop();
						}
						for (const auto& group : *groups)
							for (const auto& [id, name] : std::get<2>(group))
								if (checked.try_emplace(id, true).first->second) selected.push_back(id);
						return selected;
					};
					auto selectedSongs = groupPicker("歌曲", true, songs);
					auto selectedIdols = groupPicker("偶像", false, idols);
					static int policy = 0, count = 3, interval = 5;
					static bool partsSongs = true, ordinarySongs = true, twoDimensional = true;
					ImGui::TextUnformatted("加入歌曲类型");
					ImGui::SameLine(); ImGui::Checkbox("有分唱", &partsSongs);
					ImGui::SameLine(); ImGui::Checkbox("无分唱", &ordinarySongs);
					ImGui::SameLine(); ImGui::Checkbox("2D MV", &twoDimensional);
					ImGui::Combo("人员不足时", &policy, "按人员条件筛选\0不限制（分唱优先）\0");
					ImGui::InputInt("不重复歌曲数量", &count);
					ImGui::InputInt("每首间隔（秒）", &interval);
					static bool preferSongOutfit = false;
					ImGui::Checkbox("优先使用乐曲 MV 原始服装", &preferSongOutfit);
					static int outfitMode = 0, randomHairMode = 0;
					ImGui::Combo("随机衣装方式", &outfitMode, "全随机\0和首位保持一致\0同名服装（本人优先）\0");
					ImGui::Combo("发型与配饰", &randomHairMode, "使用服装配套搭配\0使用本人默认搭配\0");
					ImGui::Separator();
					if (ImGui::Button("把当前预览服装加入随机表")) RequestMvOutfitTable(4);
					ImGui::SameLine(); if (ImGui::Button("导入 TXT")) RequestMvOutfitTable(6);
					ImGui::SameLine(); if (ImGui::Button("保存为 TXT")) RequestMvOutfitTable(5);
					ImGui::SameLine(); if (ImGui::Button("清空列表")) RequestMvOutfitTable(7);
					const auto savedOutfits = GetMvSavedOutfits();
					for (const auto& [index, name] : savedOutfits) {
						ImGui::PushID(2000 + index);
						ImGui::TextWrapped("%s", name.c_str());
						ImGui::SameLine();
						if (ImGui::SmallButton("删除")) RequestMvOutfitTable(2000 + index);
						ImGui::PopID();
					}
					if (ImGui::Button("开始随机播放"))
						RequestRandomMv(std::move(selectedSongs), std::move(selectedIdols),
							(partsSongs ? 1 : 0) | (ordinarySongs ? 2 : 0) | (twoDimensional ? 4 : 0),
							policy, count, interval, preferSongOutfit, outfitMode, randomHairMode);
					ImGui::SameLine();
					if (ImGui::Button("停止队列")) RequestMvFormationTool(3);
					const auto status = GetMvFormationToolStatus();
					if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
					ImGui::TextDisabled("服装表文件：mv-random-outfits.txt");
					ImGui::TreePop();
				}

				if (ImGui::CollapsingHeader("Free Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::Checkbox("Enable Free Camera", &g_enable_free_camera);
					ImGui::Checkbox("Enable ClipPlane overriding", &g_reenable_clipPlane);
					if (g_reenable_clipPlane) {
						ImGui::Dummy(ImVec2(40, 0));
						ImGui::SameLine();
						ImGui::InputFloat("near clip plane", &g_nearClipPlane);

						ImGui::Dummy(ImVec2(40, 0));
						ImGui::SameLine();
						ImGui::InputFloat("far clip plane", &g_farClipPlane);
					}
					INPUT_AND_SLIDER_FLOAT("Move Speed", &BaseCamera::moveStep, 0.0f, 0.5f);
					INPUT_AND_SLIDER_FLOAT("Mouse Speed", &g_free_camera_mouse_speed, 0.0f, 100.0f);
					INPUT_AND_SLIDER_FLOAT("Camera FOV", &SCCamera::baseCamera.fov, 0.0f, 360.0f);
					ImGui::InputFloat3("Camera Pos (x, y, z)", &SCCamera::baseCamera.pos.x);
					ImGui::InputFloat3("Camera LookAt (x, y, z)", &SCCamera::baseCamera.lookAt.x);

					ImGui::Separator();
					ImGui::Text("Save Camera State:");

					// export, append; remove all
					ImGui::Dummy(ImVec2(40, 0));
					ImGui::SameLine();
					if (ImGui::Button("Export to clipboard")) {
						rapidjson::Document doc;
						doc.SetArray();
						auto& allocator = doc.GetAllocator();
						for (auto& state : freeCamSlots) {
							doc.PushBack(SerializeFreeCamState(state, allocator), allocator);
						}
						rapidjson::StringBuffer buffer;
						rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
						doc.Accept(writer);
						std::string serialized(buffer.GetString());
						auto r = WriteClipboard(serialized);
						ShowMessageBox((r ? "Success" : "Failed"));
					}
					ImGui::SameLine();
					if (ImGui::Button("Append from clipboard")) {
						std::string text;
						if (!ReadClipboard(&text)) {
							ShowMessageBox("Failed to read clipboard.");
						}
						else {
							rapidjson::Document doc;
							doc.Parse(text.c_str());
							if (doc.IsArray()) {
								const auto& arr = doc.GetArray();
								for (auto& value : arr) {
									auto parsed = DeserializeFreeCameraState(value);
									freeCamSlots.emplace_back(parsed);
								}
							}
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove all saved states")) {
						freeCamSlots.clear();
					}

					// Camera Slot
					for (int i = 0; i < freeCamSlots.size(); ++i) {
						ImGui::PushID(i);

						// --- Save button ---
						if (ImGui::Button("Save")) {
							freeCamSlots[i].hasData = true;
							freeCamSlots[i].fov = SCCamera::baseCamera.fov;

							freeCamSlots[i].pos.x = SCCamera::baseCamera.pos.x;
							freeCamSlots[i].pos.y = SCCamera::baseCamera.pos.y;
							freeCamSlots[i].pos.z = SCCamera::baseCamera.pos.z;

							freeCamSlots[i].lookAt.x = SCCamera::baseCamera.lookAt.x;
							freeCamSlots[i].lookAt.y = SCCamera::baseCamera.lookAt.y;
							freeCamSlots[i].lookAt.z = SCCamera::baseCamera.lookAt.z;

							if (freeCamSlots[i].overwriteClipPlane = g_reenable_clipPlane) {
								freeCamSlots[i].nearClipPlane = g_nearClipPlane;
								freeCamSlots[i].farClipPlane = g_farClipPlane;
							}
						}
						ImGui::SameLine();

						// --- SLOT BUTTON (apply) ---
						std::string slotName;
						if (!freeCamSlots[i].name.empty()) {
							slotName = freeCamSlots[i].name;
						}
						else {
							char slotNameBuffer[32];
							snprintf(slotNameBuffer, 32, "Slot %d\0", i + 1);
							slotName = slotNameBuffer;
						}

						if (freeCamSlots[i].hasData) {
							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.0f, 0.4f, 0.8f, 1.0f)); // Blue
						}
						else {
							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f, 1.0f)); // Grey
						}

						if (ImGui::Button(slotName.c_str(), ImVec2(80, 0))) {
							if (freeCamSlots[i].hasData) {
								SCCamera::baseCamera.fov = freeCamSlots[i].fov;

								SCCamera::baseCamera.pos.x = freeCamSlots[i].pos.x;
								SCCamera::baseCamera.pos.y = freeCamSlots[i].pos.y;
								SCCamera::baseCamera.pos.z = freeCamSlots[i].pos.z;

								SCCamera::baseCamera.lookAt.x = freeCamSlots[i].lookAt.x;
								SCCamera::baseCamera.lookAt.y = freeCamSlots[i].lookAt.y;
								SCCamera::baseCamera.lookAt.z = freeCamSlots[i].lookAt.z;

								g_reenable_clipPlane = freeCamSlots[i].overwriteClipPlane;
								g_nearClipPlane = freeCamSlots[i].nearClipPlane;
								g_farClipPlane = freeCamSlots[i].farClipPlane;
							}
						}
						ImGui::PopStyleColor();

						// --- Clear button ---
						ImGui::SameLine();
						if (ImGui::Button("Clear")) {
							freeCamSlots[i].hasData = false;
							freeCamSlots[i].name = "";
						}

						// --- Remove button ---
						ImGui::SameLine();
						if (ImGui::Button("Remove")) {
							freeCamSlots.erase(freeCamSlots.begin() + i);
						}

						// --- Rename button ---
						ImGui::SameLine();
						if (ImGui::Button("Rename")) {
							std::strncpy(newFreeCamSlotName, slotName.c_str(), sizeof(newFreeCamSlotName));
							newFreeCamSlotName[sizeof(newFreeCamSlotName) - 1] = '\0';
							ImGui::OpenPopup("RenameFreecamstateSlot");
						}
						if (ImGui::BeginPopupModal("RenameFreecamstateSlot", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
							ImGui::Text("New name:  (up to 31 bytes)");
							ImGui::InputText("##FcsSlotNameInput", newFreeCamSlotName, IM_ARRAYSIZE(newFreeCamSlotName));

							if (ImGui::Button("OK", ImVec2(120, 0))) {
								freeCamSlots[i].name = newFreeCamSlotName;
								ImGui::CloseCurrentPopup();
							}
							ImGui::SameLine();
							if (ImGui::Button("Cancel", ImVec2(120, 0))) {
								editingOverrideMvUnitIdolSlot = -1;
								ImGui::CloseCurrentPopup();
							}

							ImGui::EndPopup();
						}

						ImGui::PopID();
					}

					// --- Add New Slot Button ---
					ImGui::Dummy(ImVec2(40, 0));
					ImGui::SameLine();
					if (ImGui::Button("Add New Slot")) {
						freeCamSlots.emplace_back();
					}
				}
			}

			if (ImGui::CollapsingHeader("MagicaCloth", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Override MagicaCloth", &g_magicacloth_override);
				ImGui::SameLine();
				HELP_TOOLTIP("(?)", "数据在模型加载时被更新。\nData is only updated when loading models.");

				ImGui::Text("Show MagicaCloth logs");
				ImGui::SameLine();
				ImGui::Checkbox("(cloth)", &g_magicacloth_output_cloth);
				ImGui::SameLine();
				HELP_TOOLTIP("(?)", "并非所有服装使用MagicCloth。勾选此选项可以了解哪些服装支持（在切换时）。\nNot all costumes are using MagicaCloth. Checking this can learn what costume is supported (in changing).");
				ImGui::SameLine();
				ImGui::Checkbox("(controller)", &g_magicacloth_output_controller);
				ImGui::SameLine();
				HELP_TOOLTIP("(?)", "并非所有数值均有效。勾选此选项可以了解修改是否被应用。\nNot all values are valid. Checking this can learn if the modification is applied.");

				if (ImGui::TreeNode("Details")) {
					INPUT_AND_SLIDER_FLOAT("Inertia.min", &g_magicacloth_inertia_min, 0.0f, 1.0f);
					INPUT_AND_SLIDER_FLOAT("Inertia.max", &g_magicacloth_inertia_max, 0.0f, 1.0f);
					INPUT_AND_SLIDER_FLOAT("Radius.min", &g_magicacloth_radius_min, 0.0f, 1.0f);
					INPUT_AND_SLIDER_FLOAT("Radius.max", &g_magicacloth_radius_max, 0.0f, 1.0f);
					ImGui::InputFloat("Angle.LimitAngle", &g_magicacloth_limitAngle);
					ImGui::InputFloat("Damping", &g_magicacloth_damping);
					ImGui::InputFloat("MovementSpeedLimit", &g_magicacloth_movementSpeedLimit);
					ImGui::InputFloat("RotationSpeedLimit", &g_magicacloth_rotationSpeedLimit);
					ImGui::InputFloat("LocalMovementSpeedLimit", &g_magicacloth_localMovementSpeedLimit);
					ImGui::InputFloat("LocalRotationSpeedLimit", &g_magicacloth_localRotationSpeedLimit);
					ImGui::InputFloat("ParticleSpeedLimit", &g_magicacloth_particleSpeedLimit);
					ImGui::InputFloat("Spring.LimitDistance", &g_magicacloth_springLimitDistance);
					ImGui::InputFloat("Spring.SpringNoise", &g_magicacloth_springNoise);
					ImGui::TreePop();
				}
			}

			if (ImGui::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Use unsafe probing for unknown shaders", &g_shader_unsafe_probing);
				HELP_TOOLTIP("(?)", "对不认识的渲染程序使用unsafe实现迅速检查，但随游戏引擎版本更新有崩溃风险。\nUse unsafe probing for unknown shaders extream quickly, with crash risk after game engine updates.");
				ImGui::Checkbox("Use quick probing for unknown shaders (when not unsafe)", &g_shader_quickprobing);
				ImGui::SameLine();
				HELP_TOOLTIP("(?)", "（在非unsafe模式下）对不认识的渲染程序启用快速探测。\nUse quick probing for unknown shaders. (quick upper: 8192; only available in not unsafe mode)");
				ImGui::Checkbox("Output asset names", &g_loadasset_output);
				ImGui::SameLine();
				HELP_TOOLTIP("(?)", "在资源加载时输出资源和资源包的名称。\nOutput assets' name and AssetBundle's name when loaded.");
				ImGui::Checkbox("Extract assets of:", &g_extract_asset);
				ImGui::SameLine();
				HELP_TOOLTIP("(?)", "启用资源提取功能。注意：此选项本身不进行提取。\nEnable the process of extracting assets.\nNote: This option itself doesn't extract anything.");

				if (g_extract_asset) {
					ImGui::Checkbox("Renderer", &g_extract_asset_renderer);
					ImGui::SameLine();
					HELP_TOOLTIP("(?)", "此选项将提取所有Renderer类型子类的资源。\nThis option will extract all assets inheriting class Renderer.");
					ImGui::SameLine();
					ImGui::Checkbox("Texture2D", &g_extract_asset_texture2d);
					ImGui::SameLine();
					HELP_TOOLTIP("(?)", "此选项只提取Texture2D类型对象。\nThis option only extract instances of class Texture2D.");
					ImGui::SameLine();
					ImGui::Checkbox("Image", &g_extract_asset_image);
					ImGui::SameLine();
					ImGui::Checkbox("RawImage", &g_extract_asset_rawimage);
					ImGui::SameLine();
					ImGui::Checkbox("Sprite", &g_extract_asset_sprite);
				}
			}

			if (ImGui::CollapsingHeader("Poses", ImGuiTreeNodeFlags_DefaultOpen)) {
				PosesLoop();
			}

			if (ImGui::CollapsingHeader("Legacy", ImGuiTreeNodeFlags_None)) {
				ImGui::Checkbox("Live Allow Same Idol (Dangerous)", &g_allow_same_idol);
				ImGui::SameLine();
				HELP_TOOLTIP("(?)", "影分身术！\n允许在 Live 中选择同一人。\n（此模式的编组数据会上传，请小心你的号）");
				ImGui::SameLine();
				HELP_TOOLTIP("(?)", "This feature can be replaced by 'Override MV unit idols'.");
			}

			if (ImGui::CollapsingHeader("Devs", ImGuiTreeNodeFlags_None)) {
				devTabLoop();
			}
		}

		if (___showMessageBox) {
			ImGui::OpenPopup("MessageBox");
		}
		if (ImGui::BeginPopupModal("MessageBox", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			___showMessageBox = false;
			ImGui::Text("%s", ___messageBoxContent.c_str());
			if (ImGui::Button("OK")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (___showInputPopup) {
			ImGui::OpenPopup("InputPopup");
		}
		if (ImGui::BeginPopupModal("InputPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			___showInputPopup = false;
			ImGui::Text("%s", ___inputPopupMessage.c_str());
			ImGui::InputText("##InputPopupText", ___inputPopupBuffer, IM_ARRAYSIZE(___inputPopupBuffer));
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				___inputPopupCallback(true, ___inputPopupBuffer);
				ImGui::CloseCurrentPopup();
				___inputPopupCallback = nullptr;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				___inputPopupCallback(false, "");
				ImGui::CloseCurrentPopup();
				___inputPopupCallback = nullptr;
			}
			ImGui::EndPopup();
		}
		ImGui::End();
		if (g_enable_chara_param_edit) charaParamEditLoop();
		if (g_save_and_replace_costume_changes) savedCostumeDataLoop();
		if (g_overrie_mv_unit_idols) overrideMvUnitIdolLoop();
		if (g_override_runway_motion) overrideRunwayMotionLoop();
	}


	void PosesLoop() {
		ImGui::PushID("PosesLoop");

		// btn Scan idol poses
		ImGui::Dummy(ImVec2(40, 0));
		ImGui::SameLine();
		if (ImGui::Button("Scan idol objects")) {
			mainThreadTasks.push_back([]() {
				scannedGameObjects.clear();
				auto idols = GetActiveIdolObjects();
				if (idols.size() == 0) {
					std::cerr << "[ScanIdolObjects] No idol object found." << std::endl;
				}
				for (auto& pair : idols) {
					auto name = reflection::UnityObject_get_name(pair.first)->ToUtf8String();
					scannedGameObjects.emplace_back((Il2CppObject*)pair.first, name, name.substr(6, 2));
				}
				return true;
				});
		}

		ImGui::Separator();

		ImGui::Text("Known GameObjects");

		// list of scannedGameObjects: [txtDisplayName - btnSerialize - btnApply - btnEdit]
		ImGui::Indent();
		for (int i = 0; i < scannedGameObjects.size(); ++i) {
			ImGui::PushID(i);
			auto& data = scannedGameObjects[i];

			// txtDisplayName
			ImGui::Text(data.displayName.c_str());
			ImGui::SameLine();
			// btnSerialize
			if (ImGui::Button("Serialize")) {
				if (!data) {
					ShowMessageBox("This GameObject isn't alive now.");
					std::cerr << "[ScanIdolObjects] Target object to serialize isn't alive now." << std::endl;
					scannedGameObjects.erase(scannedGameObjects.begin() + i);
					--i;
					selectedPoseIndex = -1;
				}
				else {
					auto json = SerializeIdolPose(data.gameObject);
					if (WriteClipboard(json)) {
						ShowMessageBox("Copied!");
						std::cout << "Idol pose json copied. (length = " << json.length() << ")" << std::endl;
					}
					else {
						ShowMessageBox("Failed to write clipboard. Check console for the serialized json.");
						std::cout << "====== Idol Pose Json ======" << std::endl
							<< json << std::endl
							<< "====== end of idol pose json ======" << std::endl;
					}
					savedTransformOverridingJson.emplace_back(data.displayName, json);
				}
			}
			ImGui::SameLine();
			// btnApply
			if (ImGui::Button("ApplyTo")) {
				if (!data) {
					ShowMessageBox("This GameObject isn't alive now.");
					std::cerr << "[ScanIdolObjects] Target object to apply isn't alive now." << std::endl;
					scannedGameObjects.erase(scannedGameObjects.begin() + i);
					--i;
					selectedPoseIndex = -1;
				}
				else if (selectedPoseIndex < 0 || selectedPoseIndex >= savedTransformOverridingJson.size()) {
					ShowMessageBox("Invalid selection state. Select a valid pose data before applying.");
				}
				else {
					auto& json = savedTransformOverridingJson[selectedPoseIndex].second;
					DeserializeIdolPose(json, data.gameObject, true);
				}
			}
			ImGui::SameLine();
			// btnClear
			if (ImGui::Button("Clear")) {
				if (!data) {
					ShowMessageBox("This GameObject isn't alive now.");
					std::cerr << "[ScanIdolObjects] Target object to clear isn't alive now." << std::endl;
					scannedGameObjects.erase(scannedGameObjects.begin() + i);
					--i;
					selectedPoseIndex = -1;
				}
				else {
					ClearIdolPose(data.gameObject);
				}
			}

			ImGui::PopID();
		}
		ImGui::Unindent();

		ImGui::Separator();

		// indented header "Cached poses"
		ImGui::Text("Cached poses");

		// list of savedTransformOverridingJson: [btnName(select) - btnRename - btnCopy - btnRemove]
		ImGui::Indent();
		for (int i = 0; i < savedTransformOverridingJson.size(); ++i) {
			ImGui::PushID(i);
			// btnName(select), with a different color when selected
			ImGui::PushStyleColor(
				ImGuiCol_Button, selectedPoseIndex == i
				? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)
				: ImGui::GetStyleColorVec4(ImGuiCol_Button)
			);
			if (ImGui::Button(savedTransformOverridingJson[i].first.c_str())) {
				if (selectedPoseIndex == i) {
					selectedPoseIndex = -1;
				}
				else {
					selectedPoseIndex = i;
				}
			}
			ImGui::PopStyleColor();
			ImGui::SameLine();
			if (ImGui::Button("Rename")) {
				int copiedIndex = i;
				ShowInputPopup("Enter a new name:", savedTransformOverridingJson[copiedIndex].first, [copiedIndex](bool isOk, std::string input) {
					if (isOk) {
						savedTransformOverridingJson[copiedIndex].first = input;
					}
					});
			}
			ImGui::SameLine();
			// btnCopy
			if (ImGui::Button("Copy")) {
				auto r = WriteClipboard(savedTransformOverridingJson[i].second);
				ShowMessageBox(r ? "Success" : "Failed");
			}
			ImGui::SameLine();
			// btnRemove
			if (ImGui::Button("Remove")) {
				savedTransformOverridingJson.erase(savedTransformOverridingJson.begin() + i);
				--i;
				selectedPoseIndex = -1;
			}
			ImGui::PopID();
		}
		ImGui::Unindent();

		// btn Clear cached poses
		if (ImGui::Button("Clear cached poses")) {
			savedTransformOverridingJson.clear();
		}

		// btn import from clipboard
		ImGui::SameLine();
		if (ImGui::Button("Import from clipboard")) {
			std::string json;
			if (ReadClipboard(&json)) {
				savedTransformOverridingJson.emplace_back("paste", json);
			}
			else {
				ShowMessageBox("Failed to read clipboard.");
			}
		}

		ImGui::Separator();

		ImGui::Dummy(ImVec2(40, 0));
		ImGui::SameLine();
		if (ImGui::Button("Clear all overridings")) {
			transformOverriding.clear();
		}

		ImGui::PopID();
	}


	void devTabLoop() {
#ifdef __TOOL_HOOK_NETWORKING__
		ImGui::Checkbox("Output networking calls", &tools::output_networking_calls);
#endif
#ifdef __TOOL_BUILD_CALLING_RELATIONS__
		if (ImGui::Button("BuildCallingRelations")) {
			tools::BuildCallingRelations();
			ShowMessageBox("ok");
		}
#endif
	}
}

namespace {
struct PreviewSwapChain {
	HWND window = nullptr;
	UINT width = 0, height = 0;
	winrt::com_ptr<IDXGISwapChain1> swap;
	winrt::com_ptr<ID3D11RenderTargetView> view;
	winrt::com_ptr<ID3D11Texture2D> texture;
	winrt::com_ptr<ID3D11ShaderResourceView> image;
	UINT textureWidth = 0, textureHeight = 0;

	bool Ensure(HWND hwnd, ID3D11Device* device, IDXGIFactory2* factory) {
		RECT rect{};
		if (!hwnd || IsIconic(hwnd) || !GetClientRect(hwnd, &rect) || rect.right <= 0 || rect.bottom <= 0) return false;
		const UINT w = rect.right, h = rect.bottom;
		if (window != hwnd) {
			*this = {};
			DXGI_SWAP_CHAIN_DESC1 desc{};
			desc.Width = w;
			desc.Height = h;
			desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount = 2;
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			winrt::check_hresult(factory->CreateSwapChainForHwnd(device, hwnd, &desc, nullptr, nullptr, swap.put()));
			window = hwnd;
		} else if (width != w || height != h) {
			view = nullptr;
			winrt::check_hresult(swap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0));
		}
		if (!view) {
			winrt::com_ptr<ID3D11Texture2D> buffer;
			winrt::check_hresult(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), buffer.put_void()));
			winrt::check_hresult(device->CreateRenderTargetView(buffer.get(), nullptr, view.put()));
		}
		width = w;
		height = h;
		return true;
	}
	void Upload(ID3D11Device* device, ID3D11DeviceContext* context, const void* data, UINT pitch, UINT w, UINT h) {
		if (!texture || textureWidth != w || textureHeight != h) {
			image = nullptr;
			texture = nullptr;
			D3D11_TEXTURE2D_DESC desc{};
			desc.Width = w;
			desc.Height = h;
			desc.MipLevels = desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			winrt::check_hresult(device->CreateTexture2D(&desc, nullptr, texture.put()));
			winrt::check_hresult(device->CreateShaderResourceView(texture.get(), nullptr, image.put()));
			textureWidth = w;
			textureHeight = h;
		}
		context->UpdateSubresource(texture.get(), 0, nullptr, data, pitch, 0);
	}
};

struct DetachedCameraWindows {
	winrt::com_ptr<ID3D11Device> device;
	winrt::com_ptr<ID3D11DeviceContext> context;
	winrt::com_ptr<IDXGIFactory2> factory;
	winrt::com_ptr<ID3D11VertexShader> vertex;
	winrt::com_ptr<ID3D11PixelShader> pixel;
	winrt::com_ptr<ID3D11SamplerState> sampler;
	winrt::com_ptr<ID3D11RasterizerState> rasterizer;
	PreviewSwapChain outputs[2]{};

	void CreateDrawResources() {
		constexpr char shader[] = R"(
			Texture2D frameTexture : register(t0);
			SamplerState frameSampler : register(s0);
			struct Varying { float4 position : SV_POSITION; float2 uv : TEXCOORD; };
			Varying vs(uint id : SV_VertexID) {
				float2 uv = float2((id << 1) & 2, id & 2);
				Varying result;
				result.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
				// Unity's render texture readback stores the bottom row first.
				result.uv = float2(uv.x, 1.0 - uv.y);
				return result;
			}
			float4 ps(Varying input) : SV_TARGET { return float4(frameTexture.Sample(frameSampler, input.uv).rgb, 1); }
		)";
		winrt::com_ptr<ID3DBlob> vs, ps;
		winrt::check_hresult(D3DCompile(shader, sizeof(shader) - 1, nullptr, nullptr, nullptr,
			"vs", "vs_4_0", 0, 0, vs.put(), nullptr));
		winrt::check_hresult(D3DCompile(shader, sizeof(shader) - 1, nullptr, nullptr, nullptr,
			"ps", "ps_4_0", 0, 0, ps.put(), nullptr));
		winrt::check_hresult(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, vertex.put()));
		winrt::check_hresult(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, pixel.put()));
		D3D11_SAMPLER_DESC desc{};
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		winrt::check_hresult(device->CreateSamplerState(&desc, sampler.put()));
		D3D11_RASTERIZER_DESC raster{};
		raster.FillMode = D3D11_FILL_SOLID;
		raster.CullMode = D3D11_CULL_NONE;
		raster.DepthClipEnable = TRUE;
		winrt::check_hresult(device->CreateRasterizerState(&raster, rasterizer.put()));
	}

	void Start() {
		winrt::check_hresult(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
			device.put(), nullptr, context.put()));
		auto dxgi = device.as<IDXGIDevice>();
		winrt::com_ptr<IDXGIAdapter> adapter;
		winrt::check_hresult(dxgi->GetAdapter(adapter.put()));
		winrt::check_hresult(adapter->GetParent(__uuidof(IDXGIFactory2), factory.put_void()));
		CreateDrawResources();
	}

	bool Draw(int index, HWND window, const void* data, unsigned pitch, float w, float h) {
		auto& output = outputs[index];
		if (!output.Ensure(window, device.get(), factory.get())) return false;
		output.Upload(device.get(), context.get(), data, pitch, static_cast<UINT>(w), static_cast<UINT>(h));
		const float scale = std::min(output.width / w, output.height / h);
		const float drawnW = w * scale, drawnH = h * scale;
		const D3D11_VIEWPORT viewport{(output.width - drawnW) * 0.5f,
			(output.height - drawnH) * 0.5f, drawnW, drawnH, 0, 1};
		constexpr float black[]{0, 0, 0, 1};
		auto* view = output.view.get();
		auto* image = output.image.get();
		auto* filter = sampler.get();
		context->ClearRenderTargetView(view, black);
		context->OMSetRenderTargets(1, &view, nullptr);
		context->RSSetState(rasterizer.get());
		context->RSSetViewports(1, &viewport);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->VSSetShader(vertex.get(), nullptr, 0);
		context->PSSetShader(pixel.get(), nullptr, 0);
		context->PSSetShaderResources(0, 1, &image);
		context->PSSetSamplers(0, 1, &filter);
		context->Draw(3, 0);
		ID3D11ShaderResourceView* empty = nullptr;
		context->PSSetShaderResources(0, 1, &empty);
		context->OMSetRenderTargets(0, nullptr, nullptr);
		const HRESULT result = output.swap->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
		if (result == DXGI_ERROR_WAS_STILL_DRAWING || result == DXGI_STATUS_OCCLUDED) return false;
		winrt::check_hresult(result);
		return true;
	}
};
DetachedCameraWindows cameraWindows;
}

void ResetMvCameraWindow(int index) {
	cameraWindows.outputs[index] = {};
}

void StopMvCameraWindows() {
	if (!cameraWindows.device) return;
	if (cameraWindows.context) cameraWindows.context->ClearState();
	cameraWindows = {};
}

bool DrawMvCameraFrame(int index, HWND window, const void* data, unsigned pitch,
	int width, int height, bool& displayed, std::string& error) {
	try {
		if (!cameraWindows.device) cameraWindows.Start();
		displayed = cameraWindows.Draw(index, window, data, pitch, static_cast<float>(width), static_cast<float>(height));
		return true;
	} catch (const winrt::hresult_error& e) {
		error = winrt::to_string(e.message());
		StopMvCameraWindows();
		return false;
	}
}
