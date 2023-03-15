#include <string>

#include "ipc.hpp"

int main()
{
	DWORD aProcesses[1024], cbNeeded;
	EnumProcesses( aProcesses, sizeof( aProcesses ), &cbNeeded );
	
	std::string name; name.resize( 1024 );
	for ( size_t i = 0; i < cbNeeded / sizeof( DWORD ); i++ )
	{
		if ( aProcesses[i] != 0 )
		{
			if (HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, aProcesses[i] ) )
			{
				if ( K32GetModuleBaseNameA( process, NULL, name.data(), (DWORD)name.size() ) > 0 )
				{
					if ( name.find( "dbgview64.exe" ) != -1 )
					{
						ipc<int( HWND, LPCSTR, LPCSTR, UINT )>::call( process, "user32.dll", "MessageBoxA", nullptr, "text", "title", MB_OK );

						break;
					}
				}

				CloseHandle( process );
			}
		}
	}

	system( "pause" );

	return 0;
}
