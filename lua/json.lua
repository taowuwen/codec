#!/usr/bin/env lua

require "luci.sys"
require "luci.json"
require "luci.http"
require "luci.fs"

function print_values(tbl)
	if type(tbl) ~= "table" then
		print("type error")
		return nil
	end

	print("table size: "..table.getn(tbl).."  length: "..#tbl)

	for k,v in pairs (tbl) do
		print(k, v)
	end
end

function get_json_result(cmd)
	local jobj = luci.sys.exec(cmd .." 2>/dev/null")

	if jobj == nil or jobj == '' then
		jobj = "{}"
	end

	print(">>>$ "..cmd .. "\n==>"..jobj)

	return luci.json.decode(jobj)
end


function parse_json()
	local cmd="/tmp/ems/bin/ems_bwlist method=get flag=7"
	return get_json_result(cmd)
end


jsonbwlist=parse_json()

print(jsonbwlist)

if jsonbwlist.url ~= nil then
	print("type url: "..type(jsonbwlist.url).."length = "..#jsonbwlist.url)
	print_values(jsonbwlist.url)

end

if jsonbwlist.whitemac ~= nil then
	print("type whitemac: "..type(jsonbwlist.whitemac).."length = "..#jsonbwlist.whitemac)
	print_values(jsonbwlist.whitemac)
	--print("whitemac: "..jsonbwlist.whitemac[0])
	--print("whitemac: "..jsonbwlist.whitemac[1])
end


if jsonbwlist.blackmac ~= nil then
	print("type blackmac: "..type(jsonbwlist.blackmac).."length = "..#jsonbwlist.blackmac)
	print("type blackmac: "..type(jsonbwlist.blackmac).."length = "..table.getn(jsonbwlist.blackmac))
	print_values(jsonbwlist.blackmac)
	--print("blackmac: "..jsonbwlist.blackmac[0])
	--print("blackmac: "..jsonbwlist.blackmac[1])
end




function get_encryption_type()
	return string.trim(luci.sys.exec("uci get wireless.@wifi-iface[0].encryption -P /tmp/state/"))
--	encryption_userdata =  io.popen("cat /etc/config/wireless | grep  encryption | awk  '{print $3}'")
--	encryption = encryption_userdata:read('*all')
--	return string.trim(encryption);
end


print(get_encryption_type())

portalinfo=get_json_result("/tmp/ems/bin/ems_portal method=get")
print_values(portalinfo)
print("enable: "..portalinfo.enable)
print("enable---: "..portalinfo["enable"])

enable=portalinfo.enable1 or 1000 

print("enable---: "..enable)





