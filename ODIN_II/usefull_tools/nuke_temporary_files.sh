#/bin/bash

VTR_ROOT=$(git rev-parse --show-toplevel)

cd ${VTR_ROOT}

EXCLUDES="\
-e build \
-e CMake \
-e libs \
-e blifexplorer/blifexplorer \
-e abc/abc \
-e vpr/vpr \
-e vpr/libvpr.a \
-e ace2/ace \
-e abc/libabc.a \
-e ODIN_II/odin_II \
-e ODIN_II/libodin_ii.a \
"

while [[ 1 ]]
do
	echo "The folowing will be deleted using git clean"
	echo "============================================"
	echo ""
	TEST_DELETE="git clean -dxnf $EXCLUDES"
	if [ "_$(git clean -dxnf ${EXCLUDES} | sed 's/[[:space:]]//g')" == "_" ]
	then
		echo "Seems like theres nothing to clean! Exiting"
		exit 0
	else
		git clean -dxnf ${EXCLUDES}
	fi

	echo "Would you like to include more to exclude? (yes/no) default [no] "
	
	read CLEAN_IT
	case "_${CLEAN_IT}" in
		_yes)
			echo "insert the regexes to exclude (git style using \'-e [pattern]\')" 
			read ADD_THIS
			EXCLUDES="${EXCLUDES} $ADD_THIS"
			;;
		*)
			echo "Confirm deletion: (yes/no) [no]"
			read CONFIRM_DELETION
			case "_${CONFIRM_DELETION}" in
				_yes)
					git clean -dxf ${EXCLUDES}
					exit 0
					;;
				*)
					echo "Aborting"
					exit 0
					;;
			esac
			;;	
	esac
done
