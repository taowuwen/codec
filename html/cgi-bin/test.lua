#!/usr/bin/lua


print("Content-Type: text/html\r\n\r\n")

print("<html>\n<body>\n")
print("<p> <h1>")

print(os.getenv("GATEWAY_INTERFACE"))
print(os.getenv("SERVER_SOFTWARE"))
print(os.getenv("PATH"))
print(os.getenv("SERVER_SOFTWARE"))
print(os.getenv("SERVER_ADDR"))
print(os.getenv("SERVER_PORT"))
print(os.getenv("REMOTE_HOST"))
print(os.getenv("REMOTE_ADDR"))
print(os.getenv("REMOTE_PORT"))
print(os.getenv("QUERY_STRING"))


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
print("</h1></p>")

print("</body>\n</html>\n")
