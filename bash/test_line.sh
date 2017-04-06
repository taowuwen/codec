#!/usr/bin/env bash


echo "enable = ${1:-0}"

export printline='eval echo -e "`date` $$ [`pwd`/`basename $0`:${FUNCNAME:-OUTOFFUNC}:$LINENO]\t$*"'


test_func_name()
{
	${printline}
}

test_func_name
${printline}
