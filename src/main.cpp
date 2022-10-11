#include <stddef.h>

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace {
    void InitializeLogging() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= PluginDeclaration::GetSingleton()->GetName();
        *path += L".log";

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    class JournalMenuEx : public RE::JournalMenu {
    public:
        enum class Tab : std::uint32_t { kQuest, kPlayerInfo, kSystem };

        void Hook_Accept(RE::FxDelegateHandler::CallbackProcessor* a_cbReg)  // 01
        {
            _Accept(this, a_cbReg);
            fxDelegate->callbacks.Remove("RememberCurrentTabIndex");
            a_cbReg->Process("RememberCurrentTabIndex", RememberCurrentTabIndex);
        }

        RE::UI_MESSAGE_RESULTS Hook_ProcessMessage(RE::UIMessage& a_message)  // 04
        {
            using Message = RE::UI_MESSAGE_TYPE;
            if (a_message.type == Message::kShow) {
                auto ui = RE::UI::GetSingleton();
                auto uiStr = RE::InterfaceStrings::GetSingleton();
                if (ui->IsMenuOpen(uiStr->mapMenu)) {
                    *_savedTabIdx = Tab::kQuest;
                } else {
                    *_savedTabIdx = Tab::kSystem;
                }
            }

            return _ProcessMessage(this, a_message);
        }

        static void RememberCurrentTabIndex([[maybe_unused]] const RE::FxDelegateArgs& a_params) { return; }

        static void InstallHooks() {
            REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_JournalMenu[0]);
            _Accept = vTable.write_vfunc(0x1, &JournalMenuEx::Hook_Accept);
            _ProcessMessage = vTable.write_vfunc(0x4, &JournalMenuEx::Hook_ProcessMessage);

            log::info("Installed hooks");
        }

        using Accept_t = decltype(&RE::JournalMenu::Accept);
        using ProcessMessage_t = decltype(&RE::JournalMenu::ProcessMessage);

        static inline REL::Relocation<Accept_t> _Accept;
        static inline REL::Relocation<ProcessMessage_t> _ProcessMessage;
        static inline REL::Relocation<Tab*> _savedTabIdx{RELOCATION_ID(520167, 406697)};
    };
}

SKSEPluginLoad(const LoadInterface* skse) {
    InitializeLogging();

    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);


    Init(skse);

    JournalMenuEx::InstallHooks();

    log::info("{} has finished loading.", plugin->GetName());
    return true;
}
