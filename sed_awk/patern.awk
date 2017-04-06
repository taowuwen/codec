/^[0-9]+$/ { print "it is a number" }
/^[a-zA-Z]+$/ { print "it is a string" }
/^$/ {print "it is a blank line"}
/^[[:space:]]+$/ {print "it is a blank line----2"}
