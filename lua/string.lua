#!/usr/bin/lua


str="hello, world _yingkewifi wow _yingkewifi"


print(string.gsub(str, "_yingkewifi$", 'AAAA'))

function testmatch(str, pattn)

	if string.match(str, pattn) == nil then
		print("NOT MATCH\t"..str..">>>>"..pattn)
	else
		print("MATCH \t"..str..">>>>"..pattn)
	end
end



testmatch(str, "_yingkewifi")
testmatch(str, "_yingkewifi$")
testmatch("hello, world", "hello".."$")
testmatch("hello, world", "hello")


function sub_version(str)
	pttn="\V[12]$"

	pattn=string.match(str, pttn)

	if pattn ~= nil then
		pattn = '('..pattn..')'
		str, cnt =string.gsub(str, pttn, pattn)
	end

	return str
end

print(sub_version("Yingke Wifi G1 V2"))
print(sub_version("Yingke Wifi G1 V1"))
print(sub_version("Yingke Wifi G1"))
print(sub_version(""))
--		string.gsub(str, pttn, pattn)[0])
