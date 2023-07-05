#include <string>
#include <iostream>
#include <filesystem>
#include <Windows.h>

#include <MinHook.h>

namespace fs = std::filesystem;


namespace
{
	bool mh_init_done = false;


	// MonoImage* mono_image_open_from_data_with_name(char* data, guint32 data_len, gboolean need_copy, MonoImageOpenStatus* status, gboolean refonly, const char* name)
	// https://github.com/Unity-Technologies/mono/blob/d20b7310dcfd02edb5c6963b218a8405b92702d7/mono/metadata/image.c#L2084

	typedef void* (*MonoImageOpenFromDataWithName_T)(char*, unsigned int, bool, void*, bool, const char*);
	MonoImageOpenFromDataWithName_T mono_image_open_original = NULL;

	void* mono_image_open_hook(char* data, unsigned int data_len, bool need_copy, void* status, bool refonly, const char* name) 
	{
		std::cout << "mono_image_open_from_data_with_name: " << name << "\n";

		fs::path module_path(name);

		fs::path path_out = fs::path("./dumps") / module_path.filename();
		fs::create_directories(path_out.parent_path());

		FILE* fp = NULL;
		if (fopen_s(&fp, path_out.string().c_str(), "wb") == 0) {
			fwrite(data, sizeof(char), data_len, fp);
			fclose(fp);
		}
		
		return mono_image_open_original(data, data_len, need_copy, status, refonly, name);
	}


	typedef HMODULE(__stdcall* LoadLibraryW_T)(LPCWSTR);
	LoadLibraryW_T load_library_original = NULL;

	HMODULE __stdcall load_library_hook(_In_ LPCWSTR lpLibFileName)
	{
		std::wcout << "LoadLibraryW: " << lpLibFileName << "\n";

		fs::path library_path(lpLibFileName);
		if (library_path.filename() == "mono-2.0-bdwgc.dll") {
			HMODULE mono_runtime = load_library_original(lpLibFileName);
			FARPROC mono_image_open = GetProcAddress(mono_runtime, "mono_image_open_from_data_with_name");

			std::cout << "Creating the hook for mono_image_open_from_data_with_name" << "\n";
			std::cout << "Pointer of original mono_image_open_from_data_with_name: " << mono_image_open << "\n";
			std::cout << "Pointer of replacement mono_image_open_from_data_with_name: " << &mono_image_open_hook << "\n";

			MH_STATUS hook_result = MH_CreateHook(
				mono_image_open,
				&mono_image_open_hook,
				reinterpret_cast<LPVOID*>(&mono_image_open_original)
			);

			if (hook_result != MH_OK)
			{
				std::cout << "Failed to create the hook for mono_image_open_from_data_with_name: MH_STATUS " << hook_result << "\n";
				return mono_runtime;
			}

			MH_EnableHook(mono_image_open);

			MH_DisableHook(&LoadLibraryW);
			MH_RemoveHook(&LoadLibraryW);

			return mono_runtime;
		}

		return load_library_original(lpLibFileName);
	}
}

bool init_hook()
{
	if (mh_init_done)
		return true;

	if (MH_Initialize() != MH_OK)
		return false;

	mh_init_done = true;
	
	std::cout << "Creating the hook for LoadLibraryW" << "\n";
	std::cout << "Pointer of original LoadLibraryW: " << &LoadLibraryW << "\n";
	std::cout << "Pointer of replacement LoadLibraryW: " << &load_library_hook << "\n";

	MH_STATUS hook_result = MH_CreateHook(
		&LoadLibraryW,
		&load_library_hook,
		reinterpret_cast<LPVOID*>(&load_library_original)
	);

	if (hook_result != MH_OK) 
	{
		std::cout << "Failed to create the hook for LoadLibraryW: MH_STATUS " << hook_result << "\n";
		return false;
	}
	
	MH_EnableHook(&LoadLibraryW);

	return true;
}

void remove_hook()
{
	if (!mh_init_done)
		return;

	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}
