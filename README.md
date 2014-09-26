<h1>gimli</h1>
<p>Gimli is a small lib to hook functions using ruby on linux</p>
<p>you need to write a class in ruby and pass an intance of the class to addHook method of Gimli class.
evertime the hooked function get called, gimli will invoke hook method of the class instead of the actual call
cpu registers are also available through instance variables like @rdi, @r8, ... but they are readonly.</p>
<p>build : <br /><code>clang++ -ogimli.so -shared -g -fPIC -Wno-c++11-compat-deprecated-writable-strings -I/usr/include/ruby-1.9.1/ -I/usr/include/ruby-1.9.1/x86_64-linux/ -lruby-1.9.1 -DLINUX -Dx86_64 gimli.cpp smallelfparser.cpp</code></p>
<p>probebly you need to change the ruby lib path and include files
put hook.rb in ~/</p>
<p>hook.rb example : 
<pre><code>class Hookgetpid
    def hook 
        return 1
    end
end
gimli = Gimli.new
gimli.addHook "getpid", Hookgetpid.new</code></pre></p>
<p>example :
<pre><code>$LD_PRELOAD=./gimli.so ./example/getpid
pid is 1

$LD_PRELOAD=./gimli.so ./example/strstr
... GIMLI ...</code></pre></p>
