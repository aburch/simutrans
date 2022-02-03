# gawk file to minimize patch size

# strip trailing whitespace
{
	sub(/[[:space:]]+$/, "")
}

# replace include <sq*.h> by include "../sq*.h"
/include.*\<sq/ {
	a = gensub(/<(.*)>/, "\"../\\1\"", "g");
	print a
	next
}

# replace four spaces by tabs
/    / {
	a = gensub(/    /, "\t", "g");
	print a
	next
}

# replace stupid endif comments
/endif.*_H_/ {
	print "#endif"
	next
}

{
	print $0
}
