BUILD
==========
```bash
# Build the plugin
export Clang_DIR=<installation/dir/of/clang/16>
export THIS_DIR=<This dir>
mkdir build
cd build
cmake -DCT_Clang_INSTALL_DIR=$Clang_DIR $THIS_DIR/HelloWorld/
make
# Run the plugin
$Clang_DIR/bin/clang -cc1 -load ./libsnip.so -plugin snipper $THIS_DIR/test/HelloWorld-basic.c
```
