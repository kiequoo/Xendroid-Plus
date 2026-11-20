// Linux/non-Win32 stub: frame generation is a Windows-only feature (winfg,
// lsfg). Provides the FrameGenContext definition and no-op method bodies so
// the shared presenter code that calls ResetFrameGeneration() etc. links on
// Linux without pulling in <android/log.h>-dependent lsfg sources.

struct VulkanPresenter::FrameGenContext {};

// Active macro: ScheduleGuestOutput → ScheduleLegacyGuestOutput
bool VulkanPresenter::ScheduleGuestOutput() { return false; }

void VulkanPresenter::FrameGenerationThread() {}

void VulkanPresenter::StopFrameGenerationThread() {}

void VulkanPresenter::ResetFrameGeneration() { fg_context_.reset(); }

bool VulkanPresenter::PrepareGeneratedFrame(VkCommandBuffer,
    std::shared_ptr<GuestOutputImage>&, GuestOutputProperties&, uint64_t) {
  return false;
}

bool VulkanPresenter::TakeInGuestOutput(VkCommandBuffer,
    std::shared_ptr<GuestOutputImage>&, GuestOutputProperties&, uint64_t,
    bool) {
  return false;
}
