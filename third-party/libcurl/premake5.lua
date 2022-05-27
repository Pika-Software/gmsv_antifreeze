local current_dir = _SCRIPT_DIR

function IncludeLibCurl()
	if os.host() == "windows" then
		syslibdirs(current_dir .. "/bin/**")
		links "libcurl_imp"
	else
		links "libcurl"
	end
	sysincludedirs(current_dir .. "/include")
end