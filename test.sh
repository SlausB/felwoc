rm test/haxe/Assets/xls2xj.bin
cp out/haxe/xls2xj.bin test/haxe/Assets

rm -rf test/haxe/Source/design
cp -r out/haxe test/haxe/Source/design
rm test/haxe/Source/design/xls2xj.bin

haxelib run openfl test ./test/haxe neko