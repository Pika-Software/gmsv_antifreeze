local current_dir = _SCRIPT_DIR

function IncludeLibCurl()
	syslibdirs(current_dir .. "/bin/**")
	if os.host() == "windows" then
		links "libcurl_imp"
	else
		links "curl"
		links "ssl"
		links "crypto"
		links "pthread"
	end
	sysincludedirs(current_dir .. "/include")
end