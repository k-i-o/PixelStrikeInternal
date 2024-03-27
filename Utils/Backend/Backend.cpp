#include "../Backend/Backend.h"
#include "../../Cheat/Functions/Functions.h"

Backend::presentVariable originalPresent;
Backend::presentVariable hookedPresent;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool init = false;

Backend RunBackend;

bool Backend::DirectXPresentHook()
{
	ZeroMemory(&m_gSwapChainDescription, sizeof(m_gSwapChainDescription));

	m_gSwapChainDescription.BufferCount = 2;
	m_gSwapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_gSwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_gSwapChainDescription.OutputWindow = GetForegroundWindow();
	m_gSwapChainDescription.SampleDesc.Count = 1;
	m_gSwapChainDescription.Windowed = TRUE;
	m_gSwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	HRESULT createDevice = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, m_gFeatureLevels, 2, D3D11_SDK_VERSION, &m_gSwapChainDescription, &m_gSwapChain, &m_gDevice, nullptr, nullptr);
		
	if (FAILED(createDevice)) 
		return false; // dont return false make an endless cycle (only if u wanna go cpu boom) 

	void** DX11Vtable = *reinterpret_cast<void***>(m_gSwapChain);

	UnloadDevices(false); // don't need to reset mainrendertargetview
	hookedPresent = (Backend::presentVariable)DX11Vtable[8]; // 8. virtual table is present

	return true;
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (RunBackend.m_bOpenMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) // if menu open then handle imgui events
		return true;

	return CallWindowProc(RunBackend.m_goriginalWndProc, hWnd, uMsg, wParam, lParam);
}

void Backend::LoadImGui(HWND window, ID3D11Device* device, ID3D11DeviceContext* context)
{
	ImGui::CreateContext(); // creating the context cus we need imgui
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange; // dont change cursors

	SetColorsFlags();
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Tahoma.ttf", 24.0f);

	ImGui_ImplWin32_Init(window); // which window u wanna draw your imgui huh???
	ImGui_ImplDX11_Init(device, context); // u need the device's context since u can't draw with only device, thanx dx11
} // loading the imgui

void Backend::DrawImGui(ID3D11DeviceContext* context, ID3D11RenderTargetView* targetview)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (m_bOpenMenu)
	{
		ImGui::Begin("Pixel Strike 3D Internal Cheats by kio", &m_bOpenMenu);
		if (ImGui::BeginTabBar("tabs"))
		{
			if (ImGui::BeginTabItem("Cheats"))
			{
				ImGui::Checkbox("Camera FOV", &Variables::EnableCamera);
				if (Variables::EnableCamera)
					ImGui::SliderFloat("##CameraFOV", &Variables::CameraFov, 20, 180, "Camera FOV: %.0f");
				ImGui::Checkbox("Circle FOV", &Variables::EnableCircleFov);
				if (Variables::EnableCircleFov)
				{
					ImGui::SliderFloat("##CircleFOV", &Variables::CircleFov, 20, 180, "Circle FOV: %.0f");
				}
				ImGui::Checkbox("Enable Snaplines", &Variables::EnableSnaplines);
				if (Variables::EnableSnaplines)
				{
					ImGui::SliderInt("##LineType", &Variables::LineTypes, 1, 3, "Line Type: %i");
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Misc"))
			{
				ImGui::Checkbox("Rainbow Snaplines", &Variables::EnableRainbowSnaplines);
				if (!Variables::EnableRainbowSnaplines) {
					ImGui::ColorEdit3("Snaplines Color", (float*)&Variables::PlayerSnaplineColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoInputs);
				}
				ImGui::Checkbox("Rainbow FOV Circle", &Variables::EnableRainbow);
				ImGui::Spacing();
				ImGui::Checkbox("Watermark", &Variables::EnableWatermark);
				if (Variables::EnableWatermark)
					ImGui::Checkbox("FPS", &Variables::EnableFPS);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

			/*ImGui::Checkbox("Enable edit recoil", &Variables::EnableRecoil);
			if (Variables::EnableRecoil)
				ImGui::SliderFloat("##Recoil", &Variables::RecoilEdit, 0.0f, 10.0f, "Recoil Value: %f");*/

			//ImGui::Checkbox("Show Health", &Variables::ShowHealth);
			//ImGui::Checkbox("Enable Boxes", &Variables::EnableBoxes);
			//if (Variables::EnableBoxes)
			//{
			//	ImGui::SameLine();
			//	ImGui::ColorEdit3("##SnaplineColor", (float*)&Variables::BoxesColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoInputs);
			//	ImGui::Checkbox("Rainbow", &Variables::EnableRainbowBoxes);
			//}
		
		ImGui::End();
	}

	RunBackend.RenderCheat();
	ImGui::EndFrame();
	ImGui::Render();
	context->OMSetRenderTargets(1, &targetview, NULL);  // 1 render target, render it to our monitor, no dsv
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // drawing the imgui menu
}

void Backend::UnloadImGui()
{
	MH_DisableHook(hookedPresent); 
	MH_RemoveHook(hookedPresent);
	MH_Uninitialize();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Backend::UnloadDevices(bool renderTargetViewReset)
{
	if(renderTargetViewReset) if (m_gMainRenderTargetView) { m_gMainRenderTargetView->Release(); m_gMainRenderTargetView = nullptr; }
	if (m_gPointerContext) { m_gPointerContext->Release(); m_gPointerContext = nullptr; }
	if (m_gDevice) { m_gDevice->Release(); m_gDevice = nullptr; }
	SetWindowLongPtr(m_gWindow, GWLP_WNDPROC, (LONG_PTR)(m_goriginalWndProc));
}

void Backend::RenderCheat()
{
	if (Variables::EnableCamera)
	{
		//Unity::CCamera* CameraMain = Unity::Camera::GetMain();
		auto CameraMain = GameFunctions::GetUnityCamera();

		if (CameraMain != nullptr)
			CameraMain->CallMethodSafe<void*>("set_fieldOfView", Variables::CameraFov);
	}

	if (Variables::EnableCircleFov && Variables::EnableRainbow)
		Utils::UseFov(true);
	else if (Variables::EnableCircleFov && !Variables::EnableRainbow)
		Utils::UseFov(false);

	if (Variables::EnableWatermark && Variables::EnableFPS)
		Utils::Watermark("@kiocode", true);
	else if(Variables::EnableWatermark && !Variables::EnableFPS)
		Utils::Watermark("@kiocode", false);
	
	if (Utils::PlayerList.size() > 0 && (Variables::EnableSnaplines || Variables::EnableBoxes)) // Remember to add other variables
	{
		for (int i = 0; i < Utils::PlayerList.size(); i++)
		{
			if (!Utils::PlayerList[i]) continue;

			auto PlayerPosition = Utils::PlayerList[i]->GetTransform()->GetPosition();

			Unity::Vector3 RealPlayerPos = PlayerPosition;
			RealPlayerPos.y -= 0.2f;

			if (Variables::EnableSnaplines)
			{

				Vector2 ScreenPosition;
				if (Utils::World2Screen(RealPlayerPos, ScreenPosition)) {
					ImColor Color;
					if (Variables::EnableRainbowSnaplines)
						Color = ImColor(Variables::RainbowColor.x, Variables::RainbowColor.y, Variables::RainbowColor.z, Variables::RainbowColor.w);
					else
						Color = Variables::PlayerSnaplineColor;

					switch (Variables::LineTypes)
					{
					case 1:
						ImGui::GetBackgroundDrawList()->AddLine(ImVec2(Variables::ScreenCenter.x, Variables::ScreenSize.y), ImVec2(ScreenPosition.x, ScreenPosition.y), Color, 1.5f);
						break;
					case 2:
						ImGui::GetBackgroundDrawList()->AddLine(ImVec2(Variables::ScreenCenter.x, Variables::ScreenCenter.y), ImVec2(ScreenPosition.x, ScreenPosition.y), Color, 1.5f);
						break;
					case 3:
						ImGui::GetBackgroundDrawList()->AddLine(ImVec2(Variables::ScreenCenter.x, 0), ImVec2(ScreenPosition.x, ScreenPosition.y), Color, 1.5f);
						break;
					}
				}
			}
			if (Utils::PlayerList[i]->CallMethodSafe<bool>("get_isLocal")) {
				printf("local player found %s", Utils::PlayerList[i]->GetMemberValue<std::string>("playerName"));
			}
			// if (Variables::ShowHealth) {
			// 	if (Utils::PlayerList[i]->GetComponent("PlayerController")) continue;
			// 	float thickness = 1.5f;

			// 	Vector2 ScreenPosition;
			// 	if (Utils::World2Screen(RealPlayerPos, ScreenPosition)) {
			// 		auto health = Utils::PlayerList[i]->GetComponent("Health");

			// 		int healthValue = health->GetMemberValue<int>("CurrentHealth");
			// 		int maxHealthValue = health->GetMemberValue<int>("MaxHealth");

			// 		printf("%d, %d", healthValue, maxHealthValue);

			// 		ImColor borderColor = ImColor(0, 0, 0);
			// 		ImColor fillColor = ImColor(0, 255, 0);

			// 		Unity::Vector3 HeadPosition = Unity::Vector3(RealPlayerPos.x, RealPlayerPos.y + thickness, RealPlayerPos.z);
			// 		Unity::Vector3 FeetPosition = Unity::Vector3(RealPlayerPos.x, RealPlayerPos.y - thickness, RealPlayerPos.z);

			// 		Vector2 ScreenHeadPosition, ScreenFeetPosition;
			// 		Utils::World2Screen(HeadPosition, ScreenHeadPosition);
			// 		Utils::World2Screen(FeetPosition, ScreenFeetPosition);

			// 		float height = ScreenHeadPosition.y - ScreenFeetPosition.y;
			// 		float width = height / 2;

			// 		ImVec2 currentHealthLimit = ImVec2(ScreenFeetPosition.x - (width / 2) + (width * (healthValue / maxHealthValue)), (float)Variables::ScreenSize.y - ScreenFeetPosition.y - height);

			// 		ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(ScreenFeetPosition.x - (width / 2), (float)Variables::ScreenSize.y - ScreenFeetPosition.y - height), currentHealthLimit, fillColor);
			// 		ImGui::GetBackgroundDrawList()->AddRect(ImVec2(ScreenFeetPosition.x - (width / 2), (float)Variables::ScreenSize.y - ScreenFeetPosition.y - height), ImVec2(width, height), borderColor, thickness);
			// 	}
			// }

			/*if (Variables::EnableBoxes) {
				if (Utils::PlayerList[i]->GetComponent("PlayerController")) continue;

				Vector2 ScreenPosition;
				if (Utils::World2Screen(RealPlayerPos, ScreenPosition)) {
					ImColor Color;
					if (Variables::EnableRainbowBoxes)
						Color = ImColor(Variables::RainbowColor.x, Variables::RainbowColor.y, Variables::RainbowColor.z, Variables::RainbowColor.w);
					else
						Color = Variables::BoxesColor;

					float thickness = 1.5f;

					Unity::Vector3 HeadPosition = Unity::Vector3(RealPlayerPos.x, RealPlayerPos.y + thickness, RealPlayerPos.z);
					Unity::Vector3 FeetPosition = Unity::Vector3(RealPlayerPos.x, RealPlayerPos.y - thickness, RealPlayerPos.z);

					Vector2 ScreenHeadPosition, ScreenFeetPosition;
					Utils::World2Screen(HeadPosition, ScreenHeadPosition);
					Utils::World2Screen(FeetPosition, ScreenFeetPosition);

					float height = ScreenHeadPosition.y - ScreenFeetPosition.y;
					float width = height / 2;

					ImGui::GetBackgroundDrawList()->AddRect(ImVec2(ScreenFeetPosition.x - (width / 2), (float)Variables::ScreenSize.y - ScreenHeadPosition.y), ImVec2(width, height), Color, thickness);
				}

			}*/
		}
	}
}	

static long __stdcall PresentHook(IDXGISwapChain* pointerSwapChain, UINT sync, UINT flags)
{
	if (!init) {
		if (SUCCEEDED(pointerSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&RunBackend.m_gDevice))) // check if device working 
		{
			RunBackend.m_gDevice->GetImmediateContext(&RunBackend.m_gPointerContext); // need context immediately!!
			pointerSwapChain->GetDesc(&RunBackend.m_gPresentHookSwapChain); // welp we need the presenthook's outputwindow so it's actually ours o_o
			RunBackend.m_gWindow = RunBackend.m_gPresentHookSwapChain.OutputWindow;

			pointerSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&RunBackend.m_gPointerBackBuffer); // getting back buffer
			if (RunBackend.m_gPointerBackBuffer != nullptr) RunBackend.m_gDevice->CreateRenderTargetView(RunBackend.m_gPointerBackBuffer, NULL, &RunBackend.m_gMainRenderTargetView); // from backbuffer to our monitor
			RunBackend.m_gPointerBackBuffer->Release(); // don't need this shit anymore, but please comeback the next injection

			RunBackend.LoadImGui(RunBackend.m_gWindow, RunBackend.m_gDevice, RunBackend.m_gPointerContext); // load imgui!!!
			RunBackend.m_goriginalWndProc = (WNDPROC)SetWindowLongPtr(RunBackend.m_gWindow, GWLP_WNDPROC, (LONG_PTR)WndProc); // i think u need this

			RunBackend.m_gPointerContext->RSGetViewports(&RunBackend.m_gVps, &RunBackend.m_gViewport);
			Variables::ScreenSize = { RunBackend.m_gViewport.Width, RunBackend.m_gViewport.Height };
			Variables::ScreenCenter = { RunBackend.m_gViewport.Width / 2.0f, RunBackend.m_gViewport.Height / 2.0f };

			ImGui::GetIO().Fonts->AddFontDefault();
			RunBackend.g_mFontConfig.GlyphExtraSpacing.x = 1.2;
			BaseFonts::GameFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(BaseFonts::TTSquaresCondensedBold, 14, 14, &RunBackend.g_mFontConfig);
			ImGui::GetIO().Fonts->AddFontDefault();

			init = true;
		}
		else
			return originalPresent(pointerSwapChain, sync, flags); // returning original too
	}

	if (Utils::KeyPressed(VK_INSERT))
		RunBackend.m_bOpenMenu = !RunBackend.m_bOpenMenu;

	static float isRed = 0.0f, isGreen = 0.01f, isBlue = 0.0f;
	int FrameCount = ImGui::GetFrameCount();

	if (isGreen == 0.01f && isBlue == 0.0f) isRed += 0.01f;
	if (isRed > 0.99f && isBlue == 0.0f) {isRed = 1.0f; isGreen += 0.01f; }
	if (isGreen > 0.99f && isBlue == 0.0f) { isGreen = 1.0f; isRed -= 0.01f; }
	if (isRed < 0.01f && isGreen == 1.0f){ isRed = 0.0f; isBlue += 0.01f; }
	if (isBlue > 0.99f && isRed == 0.0f) { isBlue = 1.0f; isGreen -= 0.01f; } // ugliest function ive ever seen
	if (isGreen < 0.01f && isBlue == 1.0f) { isGreen = 0.0f; isRed += 0.01f; }
	if (isRed > 0.99f && isGreen == 0.0f) { isRed = 1.0f; isBlue -= 0.01f; }
	if (isBlue < 0.01f && isGreen == 0.0f) { isBlue = 0.0f; isRed -= 0.01f;
		if (isRed < 0.01f) isGreen = 0.01f; }

	Variables::RainbowColor = ImVec4(isRed, isGreen, isBlue, 1.0f);

	RunBackend.DrawImGui(RunBackend.m_gPointerContext, RunBackend.m_gMainRenderTargetView); // draw imgui every time
	return originalPresent(pointerSwapChain, sync, flags); // return the original so no stack corruption
}

bool Backend::Load()
{
	RunBackend.DirectXPresentHook(); // this always okay if game directx11
	MH_Initialize(); // aint no error checking cuz if minhook bad then its your problem 

	MH_CreateHook(reinterpret_cast<void**>(hookedPresent), &PresentHook, reinterpret_cast<void**>(&originalPresent)); 
	MH_EnableHook(hookedPresent); // hooking present

	return true;
}

void Backend::Unload()
{
	UnloadImGui(); // imgui unload
	UnloadDevices(true); // unloading all devices
}

void Backend::SetColorsFlags()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.FrameBorderSize = 1.f;
	style.TabBorderSize = 1.f;
	style.WindowTitleAlign.x = 0.50f;
	style.WindowPadding = ImVec2(5, 5);
	style.WindowRounding = 4.0f;
	style.FramePadding = ImVec2(6, 6);
	style.FrameRounding = 2.0f;
	style.ItemSpacing = ImVec2(12, 8);
	style.ItemInnerSpacing = ImVec2(8, 6);
	style.IndentSpacing = 25.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 20.0f;
	style.GrabRounding = 3.0f;
	style.WindowMinSize = ImVec2(200, 100);

	style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.01f, 0.91f, 0.96f, 0.94f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.04f, 0.04f, 0.04f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.38f, 0.09f, 0.89f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.00f, 1.00f, 0.31f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.43f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.38f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.43f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.00f, 0.87f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.38f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.84f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.86f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.47f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.49f, 0.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.43f, 0.00f, 1.00f, 0.89f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.77f, 0.06f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.97f, 0.00f, 1.00f, 0.84f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.28f, 0.28f, 0.57f, 0.82f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.35f, 0.35f, 0.65f, 0.84f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.73f);
}