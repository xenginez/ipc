/*!
 * \file	inject.hpp
 *
 * \author	ZhengYuanQing
 * \date	2023/03/14
 * \email	zhengyuanqing.95@gmail.com
 *
 */
#ifndef INJECT_HPP__89692EE0_6A00_4F90_83A9_0046DBCCACAA
#define INJECT_HPP__89692EE0_6A00_4F90_83A9_0046DBCCACAA

#include <tuple>
#include <thread>
#include <windows.h>
#include <psapi.h>

template< typename ... T > class ipc;

template< typename R, typename ... T > class ipc< R( T... ) >
{
public:
	using self_type = ipc< R( T... ) >;
	using result_type = R;
	using parameter_type = std::tuple< T... >;
	using function_type = R( * )( T... );
	using load_library_type = HMODULE( * )( LPCSTR );
	using get_proc_address_type = FARPROC( * )( HMODULE, LPCSTR );

	struct info_type
	{
		result_type result = {};
		parameter_type args = {};
		char module[100] = {};
		char function[100] = {};
		load_library_type load_library = nullptr;
		get_proc_address_type get_proc_address = nullptr;
	};

public:
	static result_type call( HANDLE process, const char * module, const char * function, T...args )
	{
		info_type info;
		strcpy_s( info.module, 100, module );
		strcpy_s( info.function, 100, function );
		info.args = std::make_tuple( of( process, args )... );
		auto kernel32 = GetModuleHandleA( "kernel32.dll" );
		info.load_library = (load_library_type)GetProcAddress( kernel32, "LoadLibraryA" );
		info.get_proc_address = (get_proc_address_type)GetProcAddress( kernel32, "GetProcAddress" );

		if ( auto info_base = VirtualAllocEx( process, nullptr, sizeof( info_type ), MEM_COMMIT, PAGE_READWRITE ) )
		{
			if ( WriteProcessMemory( process, info_base, &info, sizeof( info_type ), nullptr ) )
			{
				if ( auto code_base = VirtualAllocEx( process, nullptr, 1024, MEM_COMMIT, PAGE_EXECUTE_READWRITE ) )
				{
					if ( WriteProcessMemory( process, code_base, shell_code, 1024, nullptr ) )
					{
						if ( auto thread = CreateRemoteThread( process, nullptr, 0, (LPTHREAD_START_ROUTINE)code_base, info_base, 0, nullptr ) )
						{
							WaitForSingleObject( thread, INFINITE );

							ReadProcessMemory( process, info_base, &info, sizeof( result_type ), nullptr );

							CloseHandle( thread );
						}
					}

					VirtualFreeEx( process, code_base, 0, MEM_RELEASE );
				}

				ReadProcessMemory( process, info_base, &info, sizeof( result_type ), nullptr );
			}

			VirtualFreeEx( process, info_base, 0, MEM_RELEASE );
		}

		return info.result;
	}

private:
	static bool of( HANDLE process, bool arg ) { return arg; }
	static std::int8_t of( HANDLE process, std::int8_t arg ) { return arg; }
	static std::int16_t of( HANDLE process, std::int16_t arg ) { return arg; }
	static std::int32_t of( HANDLE process, std::int32_t arg ) { return arg; }
	static std::int64_t of( HANDLE process, std::int64_t arg ) { return arg; }
	static std::uint8_t of( HANDLE process, std::uint8_t arg ) { return arg; }
	static std::uint16_t of( HANDLE process, std::uint16_t arg ) { return arg; }
	static std::uint32_t of( HANDLE process, std::uint32_t arg ) { return arg; }
	static std::uint64_t of( HANDLE process, std::uint64_t arg ) { return arg; }
	static float of( HANDLE process, float arg ) { return arg; }
	static double of( HANDLE process, double arg ) { return arg; }
	static const char * of( HANDLE process, const char * arg )
	{
		auto size = strlen( arg ) + 1;
		if ( auto base = VirtualAllocEx( process, nullptr, size, MEM_COMMIT, PAGE_READWRITE ) )
		{
			if ( WriteProcessMemory( process, base, arg, size, nullptr ) )
			{
				return (const char *)base;
			}
		}
		return nullptr;
	}
	template< typename U > static U * of( HANDLE process, U * arg )
	{
		if ( auto base = VirtualAllocEx( process, nullptr, sizeof( U ), MEM_COMMIT, PAGE_READWRITE ) )
		{
			if ( WriteProcessMemory( process, base, arg, sizeof( U ), nullptr ) )
			{
				return (U *)base;
			}
		}
		return nullptr;
	}
	template< typename U > static std::enable_if<std::is_pod_v<U>, U> of( HANDLE process, U arg )
	{
		return arg;
	}

private:
	static DWORD shell_code( info_type * data )
	{
		if ( auto mod = data->load_library( data->module ) )
		{
			if ( auto func = data->get_proc_address( mod, data->function ) )
			{
				data->result = std::apply( ( (function_type)func ), data->args );

				return 0;
			}
		}

		return 1;
	}
};

#endif//INJECT_HPP__89692EE0_6A00_4F90_83A9_0046DBCCACAA
