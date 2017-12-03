
function SetTarget( _configuration, _platform, _basepath )
	--print(_configuration .. _platform)
	local platformname = _platform
	local archname = _platform
	if _platform == "x32" then
		platformname = "Win32"
		archname = "x86"
	end
	local strtarget = string.format( "%s/bin/%s_%s/", _basepath, _configuration, platformname ) 
	local strobj = string.format( "%s/intermediate/%s_%s", _basepath, _configuration, platformname ) 
	configuration {_configuration, _platform}
		targetdir( strtarget )
		objdir( strobj )
end


--------------------------------------------------------
solution "dae2pbrt"
	configurations { "Debug", "Release" }

	configuration "macosx"
		platforms { "native" }
	configuration "windows"
		platforms { "x32", "x64" }
		

	---------------------------------------------------------
	project "tinyxml2" 
		kind "StaticLib"
		language "C++"
		files { "../src/ext/tinyxml2/tinyxml2.h", "../src/ext/tinyxml2/tinyxml2.cpp" }

		defines { "_CRT_SECURE_NO_WARNINGS" }
		
		local targetpath = ".."
		configuration "windows"
			SetTarget( "Debug", "x32", targetpath )
			SetTarget( "Debug", "x64", targetpath )
			SetTarget( "Release", "x32", targetpath )
			SetTarget( "Release", "x64", targetpath )
			
		configuration "macosx"
			SetTarget( "Debug", "native", targetpath )
			SetTarget( "Release", "native", targetpath )
			
		configuration "Debug"
			defines { "_DEBUG" }
			flags { "Symbols" }
 
		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "Symbols" }
			--optimize "On"

		configuration "macosx"
            buildoptions { "-std=c++11" } --, "-stdlib=libc++" }

	---------------------------------------------------------
	project "dae2pbrt"
		kind "ConsoleApp"
		language "C++"
		files { "../src/**.h", "../src/**.cpp" }

		removefiles { "../src/ext/**" }
		targetname "dae2pbrt"
	
		defines { "_CRT_SECURE_NO_WARNINGS", "_WINDOWS", "_USRDLL" }
		flags { "NoPCH", "NoNativeWChar", "NoEditAndContinue" }

		includedirs { "../src", "../src/ext" }

		configuration "windows"	
			links { "tinyxml2"  }

		configuration "macosx"
			links { "tinyxml2" }

		local targetpath = ".."
		local libpath = "../.."
		configuration "windows"
			SetTarget( "Debug", "x32", targetpath )
			SetTarget( "Debug", "x64", targetpath )
			SetTarget( "Release", "x32", targetpath )
			SetTarget( "Release", "x64", targetpath )
			
		configuration "macosx"
			SetTarget( "Debug", "native", targetpath )
			SetTarget( "Release", "native", targetpath )

		configuration "Debug"
			defines { "_DEBUG" }
			flags { "Symbols" }
 
		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "Symbols"}
			--optimize "On"

		configuration "macosx"
            linkoptions  { "-std=c++11" } 
            buildoptions { "-std=c++11" }
			

