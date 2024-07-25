#include <cstdint>
#include <algorithm>
#include <bit>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma comment(lib, "imm32")
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")


using byte = uint8_t;
#include <aviutl/filter.hpp>


////////////////////////////////
// 情報源のアドレス．
////////////////////////////////
struct AviUtl110 {
	bool init(AviUtl::FilterPlugin* this_fp) {
		AviUtl::SysInfo si;
		this_fp->exfunc->get_sys_info(nullptr, &si);
		if (si.build != build110) return false;
		font = si.hfont;

		init_pointers(this_fp->hinst_parent);
		return true;
	}

	HFONT font;

	int32_t* preview_volume; // 0x2c4db4

private:
	constexpr static int32_t build110 = 11003;
	void init_pointers(HINSTANCE hinst) {
		auto pick_addr = [base_addr = reinterpret_cast<uintptr_t>(hinst)]<class T>(T& addr, ptrdiff_t offset) {
			addr = std::bit_cast<T>(base_addr + offset);
		};

		pick_addr(preview_volume, 0x2c4db4);
	}
} aviutl;


////////////////////////////////
// 設定 / 保存項目．
////////////////////////////////
static constinit struct Settings {
	struct Layout {
		bool vertical = false;
		bool mute_button = true;
	} layout;

	struct State {
		int32_t volume_unmuted = max_volume_unmuted;
		constexpr static int32_t min_volume_unmuted = 0, max_volume_unmuted = 256;
	} state;

	void load(char const* inifile) {
	#define load_int(head, field, section)	head##field = std::clamp(static_cast<decltype(head##field)>( \
		::GetPrivateProfileIntA(section, #field, static_cast<int>(head##field), inifile)), head##min_##field, head##max_##field)
	#define load_bool(head, field, section)	head##field = 0 != \
		::GetPrivateProfileIntA(section, #field, (head##field) ? 1 : 0, inifile)

		load_bool(layout., vertical, "Layout");
		load_bool(layout., mute_button, "Layout");

		load_int(state., volume_unmuted, "State");

	#undef load_bool
	#undef load_int
	}
	void save(char const* inifile) {
	#define save_int(head, field, section, max_len)	\
		do { \
			char buf[(max_len) + 1]; ::sprintf_s(buf, "%d", head##field); \
			::WritePrivateProfileStringA(section, #field, buf, inifile); \
		} while (false)
	//#define save_bool(head, field, section)	::WritePrivateProfileStringA(section, #field, (head##field) ? "1" : "0", inifile)
	//	
	//	save_bool(layout., vertical, "Layout");
	//	save_bool(layout., mute_button, "Layout");

		save_int(state., volume_unmuted, "State", std::size("256") - 1);

	//#undef save_bool
	#undef save_int
	}
	bool load(HMODULE dll) {
		return ini_operation(dll, [this](auto ini) { load(ini); });
	}
	bool save(HMODULE dll) {
		return ini_operation(dll, [this](auto ini) { save(ini); });
	}
private:
	bool ini_operation(HMODULE dll, auto op) {
		char inifile[MAX_PATH];
		int len = ::GetModuleFileNameA(dll, inifile, std::size(inifile));
		constexpr char ini[] = "ini";
		if (len < std::size(ini)) return false;
		std::memcpy(inifile + len - (std::size(ini) - 1), ini, std::size(ini) - 1);
		op(inifile);
		return true;
	}
} settings;


////////////////////////////////
// 状態定義．
////////////////////////////////
static constinit struct State {
	constexpr static auto
		min = Settings::State::min_volume_unmuted,
		max = Settings::State::max_volume_unmuted;

	bool muted = true;
	int32_t volume_unmuted = max;
	int32_t get_pos() const {
		return !settings.layout.vertical ? volume_unmuted : (min + max) - volume_unmuted;
	}
	void set_pos(int32_t pos) {
		volume_unmuted = !settings.layout.vertical ? pos : (min + max) - pos;
	}
	int32_t eff_vol() const { return muted ? min : volume_unmuted; }
	bool force_pull() {
		auto vol = std::clamp(*aviutl.preview_volume, min, max);
		if (eff_vol() == vol) return false;
		muted = false;
		volume_unmuted = vol;
		return true;
	}
	void force_push() const { *aviutl.preview_volume = eff_vol(); }
	constexpr static UINT mes_notify_gauge = WM_USER + 201;
	uintptr_t hook_id() const { return reinterpret_cast<uintptr_t>(this); }
} state;


////////////////////////////////
// DLL の参照カウントを増やす．
////////////////////////////////
static inline void IncrementRefCount(HMODULE dll)
{
	char path[MAX_PATH];
	::GetModuleFileNameA(dll, path, std::size(path));
	::LoadLibraryA(path);
}


////////////////////////////////
// フックのコールバック関数．
////////////////////////////////
static LRESULT CALLBACK hook_main_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data) {
	auto ret = ::DefSubclassProc(hwnd, message, wparam, lparam);

	// hook mouse messages that might change the playback volume.
	if (((message == WM_MOUSEMOVE && (wparam & MK_LBUTTON) != 0) || message == WM_LBUTTONDOWN) &&
		state.eff_vol() != *aviutl.preview_volume)
		::SendMessageW(reinterpret_cast<HWND>(data), State::mes_notify_gauge, wparam, lparam);
	return ret;
}


////////////////////////////////
// AviUtlに渡す関数の定義．
////////////////////////////////
static BOOL func_init(AviUtl::FilterPlugin* fp)
{
	if (!aviutl.init(fp)) {
		::MessageBoxA(nullptr, "AviUtl のバージョンが 1.10 である必要があります．",
			fp->name, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (settings.load(fp->dll_hinst))
		state.volume_unmuted = settings.state.volume_unmuted;

	return TRUE;
}
static BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
	constexpr auto font_name = L"Segoe Fluent Icons", mute_face = L"\ue74f";
	constexpr wchar_t const* loud_faces[] = { L"\ue992", L"\ue993", L"\ue994", L"\ue995" };

	static constinit HWND mute_button = nullptr, volume_slider = nullptr, volume_label = nullptr;
	static constinit HFONT button_font = nullptr;
	constexpr struct {
		int margin = 6,
			button = 30,
			slider = 36,
			label = 16,
			font_icon = -24;
	} metrics;

	// helper class to suppress callbacks by modifying controls.
	static constinit int suppress_callback = 0;
	struct SC {
		SC() { suppress_callback++; }
		~SC() { suppress_callback--; }
		static bool active() { return suppress_callback <= 0; }
	};

	// flag to indicate the controls needs updates.
	bool update_controls = false;
	switch (message) {
		using FilterMessage = AviUtl::FilterPlugin::WindowMessage;
	case FilterMessage::Init:
	{
		// create controls and other objects.
		SC sc{};

		// mute button.
		mute_button = ::CreateWindowExW(0, WC_BUTTONW, mute_face,
			(settings.layout.mute_button ? WS_VISIBLE : 0) |
			WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_FLAT | BS_TEXT | BS_CENTER | BS_VCENTER,
			metrics.margin, metrics.margin, metrics.button, metrics.button,
			hwnd, nullptr, fp->hinst_parent, nullptr);
		button_font = ::CreateFontW(metrics.font_icon, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			font_name);
		::SendMessageW(mute_button, WM_SETFONT, reinterpret_cast<WPARAM>(button_font), TRUE);
		::SendMessageW(mute_button, BM_SETCHECK, BST_UNCHECKED, {});

		// volume slider.
		volume_slider = ::CreateWindowExW(0, TRACKBAR_CLASSW, L"",
			WS_CHILD | WS_VISIBLE |
			(settings.layout.vertical ? TBS_VERT : TBS_TOOLTIPS | TBS_HORZ),
			(!settings.layout.vertical && settings.layout.mute_button ? metrics.button + 2 * metrics.margin : metrics.margin),
			settings.layout.vertical ? metrics.label + metrics.margin : metrics.margin,
			metrics.slider, metrics.slider,
			hwnd, nullptr, fp->hinst_parent, nullptr);
		::SendMessageW(volume_slider, TBM_SETRANGEMIN, FALSE, State::min);
		::SendMessageW(volume_slider, TBM_SETRANGEMAX, FALSE, State::max);
		::SendMessageW(volume_slider, TBM_SETPAGESIZE, 0, (State::max - State::min) / 8);
		::SendMessageW(volume_slider, TBM_SETTIC, 0, State::min);
		::SendMessageW(volume_slider, TBM_SETTIC, 0, (State::min + State::max) / 2);
		::SendMessageW(volume_slider, TBM_SETTIC, 0, State::max);

		// static for the current volume.
		if (settings.layout.vertical) {
			volume_label = ::CreateWindowExW(0, WC_STATICW, L"",
				WS_CHILD | WS_VISIBLE | SS_CENTER,
				metrics.margin, metrics.margin, metrics.button, metrics.label,
				hwnd, nullptr, fp->hinst_parent, nullptr);
			::SendMessageW(volume_label, WM_SETFONT, reinterpret_cast<WPARAM>(aviutl.font), TRUE);
		}

		// disallow IME.
		::ImmReleaseContext(hwnd, ::ImmAssociateContext(hwnd, nullptr));

		// hook mouse event on the main window while playing.
		::SetWindowSubclass(fp->hwnd_parent, hook_main_proc, state.hook_id(), reinterpret_cast<DWORD_PTR>(hwnd));
		break;
	}
	case FilterMessage::Exit:
	{
		// delete allocated objects.
		constexpr auto destroy = [](HWND& hwnd) {
			if (hwnd == nullptr) return;
			::DestroyWindow(hwnd);
			hwnd = nullptr;
		};
		destroy(mute_button);
		if (button_font != nullptr) {
			::DeleteObject(button_font);
			button_font = nullptr;
		}
		destroy(volume_slider);
		destroy(volume_label);

		// unhook mouse event on the main window.
		::RemoveWindowSubclass(fp->hwnd_parent, hook_main_proc, state.hook_id());

		// increment the reference count, otherwise this DLL will be released
		// before hook_main_proc() returns, resulting an error.
		IncrementRefCount(fp->dll_hinst);

		// save the state.
		settings.state.volume_unmuted = state.volume_unmuted;
		settings.save(fp->dll_hinst);
		break;
	}

	case State::mes_notify_gauge:
	case FilterMessage::ChangeWindow:
		if (SC::active() && mute_button != nullptr && volume_slider != nullptr &&
			fp->exfunc->is_filter_window_disp(fp)) {
			// pull the changes from AviUtl.
			update_controls = state.force_pull();

			if (message == FilterMessage::ChangeWindow) {
				// adjust layout when window shows up.
				update_controls = true;
				goto on_resize;
			}
		}
		break;
	case WM_SIZE:
		if (mute_button != nullptr && volume_slider != nullptr &&
			fp->exfunc->is_filter_window_disp(fp)) {
			// adjusts layout of the contols.
		on_resize:
			RECT rc; ::GetClientRect(hwnd, &rc);
			if (settings.layout.vertical) {
				// vertically layouted.
				int btm = 0;
				if (settings.layout.mute_button) {
					btm += metrics.button + metrics.margin;
					::SetWindowPos(mute_button, nullptr, metrics.margin, rc.bottom - btm,
						0, 0, SWP_NOSIZE | SWP_NOZORDER);
				}
				::SetWindowPos(volume_slider, nullptr, 0, 0,
					metrics.slider, std::max<int>(rc.bottom - (btm + metrics.label + metrics.margin), metrics.slider),
					SWP_NOMOVE | SWP_NOZORDER);
			}
			else {
				// horizontally layouted.
				int left = metrics.margin;
				if (settings.layout.mute_button)
					left += metrics.button + metrics.margin;
				::SetWindowPos(volume_slider, nullptr, 0, 0,
					std::max<int>(rc.right - (left + metrics.margin), metrics.slider), metrics.slider,
					SWP_NOMOVE | SWP_NOZORDER);
			}
		}
		break;
	case WM_COMMAND:
		if (SC::active() && mute_button != nullptr && volume_slider != nullptr &&
			fp->exfunc->is_filter_window_disp(fp)) {
			if (reinterpret_cast<HWND>(lparam) == mute_button) {
				switch (wparam >> 16) {
				case BN_CLICKED:
					// on mute button toggled.
					state.muted = ::SendMessageW(mute_button, BM_GETCHECK, 0, 0) != BST_UNCHECKED;

					// push the change to AviUtl.
					*aviutl.preview_volume = state.eff_vol();
					update_controls = true;

					// unfocus from button.
					SC sc{}; ::SetFocus(hwnd);
					break;
				}
			}
		}
		break;

	case WM_HSCROLL:
	case WM_VSCROLL:
		if (SC::active() && mute_button != nullptr && volume_slider != nullptr &&
			fp->exfunc->is_filter_window_disp(fp)) {
			if (reinterpret_cast<HWND>(lparam) == volume_slider) {
				switch (0xffff & wparam) {
				case TB_THUMBPOSITION:
					break;
				default:
				{
					// on slider moved.
					state.muted = false;
					state.set_pos(std::clamp(static_cast<int32_t>(
						::SendMessageW(volume_slider, TBM_GETPOS, 0, 0)), State::min, State::max));

					// push the change to AviUtl.
					*aviutl.preview_volume = state.eff_vol();
					update_controls = true;

					// unfocus from the slider.
					SC sc{}; ::SetFocus(hwnd);
					break;
				}
				}
			}
		}
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		// invoke shortcut key commands.
		::SendMessageW(fp->hwnd_parent, message, wparam, lparam);
		break;
	}

	if (update_controls) {
		SC sc{};

		// reflect the checked state of the mute button.
		if (state.muted != (::SendMessageW(mute_button, BM_GETCHECK, 0, 0) != BST_UNCHECKED))
			::SendMessageW(mute_button, BM_SETCHECK, state.muted ? BST_CHECKED : BST_UNCHECKED, {});

		// set the icon for the mute button.
		auto face = mute_face;
		if (!state.muted) {
			size_t phase = std::size(loud_faces) * (state.volume_unmuted - State::min) / (State::max - State::min);
			face = loud_faces[std::min(phase, std::size(loud_faces) - 1)];
		}
		::SendMessageW(mute_button, WM_SETTEXT, {}, reinterpret_cast<LPARAM>(face));

		// set the slider position.
		if (auto pos = state.get_pos();
			pos != static_cast<int32_t>(::SendMessageW(volume_slider, TBM_GETPOS, 0, 0)))
			::SendMessageW(volume_slider, TBM_SETPOS, TRUE, pos);

		if (settings.layout.vertical) {
			// the volume label.
			wchar_t str[std::size(L"256")];
			::swprintf_s(str, L"%d", std::clamp(state.volume_unmuted, State::min, State::max));
			::SendMessageW(volume_label, WM_SETTEXT, {}, reinterpret_cast<LPARAM>(str));
		}
	}
	return FALSE;
}


////////////////////////////////
// Entry point.
////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hinst);
		break;
	}
	return TRUE;
}


////////////////////////////////
// 看板．
////////////////////////////////
#define PLUGIN_NAME		"プレビュー音量"
#define PLUGIN_VERSION	"v1.00"
#define PLUGIN_AUTHOR	"sigma-axis"
#define PLUGIN_INFO_FMT(name, ver, author)	(name##" "##ver##" by "##author)
#define PLUGIN_INFO		PLUGIN_INFO_FMT(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR)

extern "C" __declspec(dllexport) AviUtl::FilterPluginDLL* __stdcall GetFilterTable(void)
{
	constexpr int initial_width = 256, initial_height = 256;

	// （フィルタとは名ばかりの）看板．
	using Flag = AviUtl::FilterPlugin::Flag;
	static constinit AviUtl::FilterPluginDLL filter{
		.flag = Flag::AlwaysActive | Flag::ExInformation |
			Flag::NoInitData |
			Flag::WindowThickFrame | Flag::WindowSize |
			Flag::PriorityLowest,
		.x = initial_width, .y = initial_height,
		.name = PLUGIN_NAME,

		.func_init = func_init,
		.func_WndProc = func_WndProc,
		.information = PLUGIN_INFO,
	};
	return &filter;
}

