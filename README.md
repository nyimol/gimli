gimli
=====

Gimli is a small lib to hook functions using ruby on linux

build : clang++ -ogimli.so -shared -g -fPIC -Wno-c++11-compat-deprecated-writable-strings -I/usr/include/ruby-1.9.1/ -I/usr/include/ruby-1.9.1/x86_64-linux/ -lruby-1.9.1 -DLINUX -Dx86_64 gimli.cpp smallelfparser.cpp

probebly you need to change the ruby lib path and include files
put hook.rb in ~/

example :
$LD_PRELOAD=./gimli.so ./example/getpid
pid is 1

$LD_PRELOAD=./gimli.so ./example/strstr
... GIMLI ...
