$RGSS_SCRIPTS = load_data(MkxpData::ScriptPack)

if !$RGSS_SCRIPTS.is_a?(Array)
	raise IOError.new("Unable to open '" + MkxpData::ScriptPack + "'")
end

$RGSS_SCRIPTS.each do |script|
	if !script.is_a?(Array)
		break
	end
	
	decodedScriptPtr = FFI::MemoryPointer.new(:pointer)
	encodedScriptPtr = FFI::MemoryPointer.new(:char, script[2].bytesize)
	encodedScriptPtr.write_bytes(script[2])
	
	if MkxpBinding::mkxpScriptDecode(decodedScriptPtr, encodedScriptPtr, script[2].bytesize) == 0 or decodedScriptPtr.read_pointer.null?
		raise RuntimeError.new("Error decoding script '" + script[1] + "'")
		break
	end
	
	if script.count < 4
		script.push(decodedScriptPtr.read_pointer.read_string)
	else
		script[3] = decodedScriptPtr.read_pointer.read_string
	end
		
	LibC::free(decodedScriptPtr.read_pointer)
end

MkxpData::PreloadedScripts.each do |scriptFile|
	MkxpBinding::run_custom_script(scriptFile)
end

while true do
	$RGSS_SCRIPTS.length.times do |i|
		script = $RGSS_SCRIPTS.at(i)
		if !script.is_a?(Array)
			next
		end
		
		secnum = i.to_s()
		
		MkxpBinding::mkxpScriptBacktraceInsert(MkxpData::BacktraceData, MkxpData::Config::UseScriptNames ? (secnum + ":" + script[1].to_s()) : (MkxpData::RGSSVersion >=3 ? ("{" + secnum + "}") : ("Section" + secnum)), script[1].to_s())
		
		eval(script[3].to_s(), binding, script[1].to_s())
	end
	
	MkxpBinding::mkxpProcessReset
end
