source ./arches.bash

resize_dir="$1x$2"

mkdir -p "$resize_dir"

echo "making some $resize_dir architectures..."

for arch in $ALL_ARCHES; do
	if [[ -z "$3" ]]; then
		out_file="$resize_dir/$arch"
		mkdir -p "$resize_dir"
	else
		mkdir -p "$3"
		out_file="${3}/${1}x${2}_${arch}"
	fi

	sed \
		-e "s/%%WIDTH%%/$1/g" \
		-e "s/%%HEIGHT%%/$2/g" \
	"resizable_sources/${arch:0:-4}_resizable.xml" > "$out_file"
done

echo "done"
