
tblgen="tblgen"

$tblgen ../../../src/World/Blocks.tg -block-definitions /Users/Jonas/mineshaft/cmake-build-debug/libmineshaft-tblgens.dylib > ../../../include/mineshaft/World/Blocks.def

$tblgen ../../../src/World/Blocks.tg -block-functions /Users/Jonas/mineshaft/cmake-build-debug/libmineshaft-tblgens.dylib > ../../../src/World/BlockFunctions.inc