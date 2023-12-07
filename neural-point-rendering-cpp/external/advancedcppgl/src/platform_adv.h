#pragma once

#ifdef _WIN32
	#ifdef BUILD_ADVANCED_CPPGL_DLL
		#define _API_ADV __declspec(dllexport)
		#define _EXT_API_ADV
	#else
		#define _API_ADV __declspec(dllimport)
		#define _EXT_API_ADV extern
	#endif
#else
	//UNIX, ignore _API
	#define _API_ADV
	#define _EXT_API_ADV
#endif