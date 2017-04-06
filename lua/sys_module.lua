function update_version(str)
	pttn="\V[12]$"

	if str == nil then
		return str
	end

	pattn=string.match(str, pttn)

	if pattn ~= nil then
		pattn = '('..pattn..')'
		str, cnt =string.gsub(str, pttn, pattn)
	end

	return str
end
