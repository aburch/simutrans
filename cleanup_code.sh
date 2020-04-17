#
# properly rename include guards (needs perl)
echo "Checking include guards"
find . -type f -name "*.h" | grep -v "squirrel/" | while read f; do guard="$(echo $f | cut -b 3- | tr '[[:lower:]]' '[[:upper:]]' | tr -C '[[:alnum:]]' '_' | rev | cut -c2- | rev)"; perl -i -p0e "s/(\n){2,}#ifndef [^\n]*\n#define [^\n]*(\n)+/\n\n#ifndef $guard\n#define $guard\n\n\n/" $f; done

#
# remove trailing spaces
echo "Removing trailing whitespaces"
find . -type f -name "*.h" -o -name "*.cc" | grep -v "squirrel" | xargs sed -i -e "s/[ \t]*$//"
