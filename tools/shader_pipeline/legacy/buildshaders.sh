(set -o igncr) 2>/dev/null && set -o igncr; # comment is needed on Windows to ignore this lines trailing \r

if [ "$(uname)" == "Linux" ]; then
	exit 0
elif [ "$(uname)" != "Darwin" ]; then
	# Windows has a number of compatibility layers with different uname results.
	EXE=.exe
fi

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SOURCE_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)
"$SOURCE_ROOT/Rendering/ShaderCompiler/ShaderCompiler$EXE" /P "$SOURCE_ROOT/shaders" glsl glsl3 glsles glsles3
