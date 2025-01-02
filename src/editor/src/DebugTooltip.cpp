#include "DebugTooltip.hpp"
#include "imgui/imgui.h"

void Hush::DebugTooltip::OnRender() noexcept
{
	if (s_debugTooltip == nullptr) {
		s_debugTooltip = this;
	}
	ImGui::Begin("Debug tooltip (for rendering)");
	ImGui::SliderFloat("Set FOV", &this->degFov, 10.0f, 100.0f);
	ImGui::End();
}
