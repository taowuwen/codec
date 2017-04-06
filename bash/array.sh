#!/usr/bin/env bash

array=("one" "two" "three")
array1=(1, 2, 3)


#array
printarray()
{
	declare -a ary=("${!1}")

	echo ${ary[@]}

	declare -a argAry1=("${!1}")
	echo "${argAry1[@]}"
}

printarray array[@]
printarray array1[@]


takes_ary_as_arg()
{
	declare -a argAry1=("${!1}")
	echo "${argAry1[@]}"

	declare -a argAry2=("${!2}")
	echo "${argAry2[@]}"
}

try_with_local_arys()
{
# array variables could have local scope
	local descTable=(
		"sli4-iread"
		"sli4-iwrite"
		"sli3-iread"
		"sli3-iwrite"
	)

	local optsTable=(
		"--msix  --iread"
		"--msix  --iwrite"
		"--msi   --iread"
		"--msi   --iwrite"
	)
	takes_ary_as_arg descTable[@] optsTable[@]
}
try_with_local_arys



myfunc()
{
	local hello world easy way the
	

	print_hello()
	{
		echo "hello, world=$hello"
	}

	hello="tww"

	print_hello
}

myfunc


baba=booba
variable=baba
echo ${variable} # baba
echo ${!variable} # booba

echo $variable
echo ${!variable}


