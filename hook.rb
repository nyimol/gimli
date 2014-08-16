class Hookgetpid
    def hook 
        return 1
    end
end

class Hookstrstr
    def hook
        return "... GIMLI ..."
    end
end

gimli = Gimli.new
gimli.addHook "getpid", Hookgetpid.new
gimli.addHook "strstr", Hookstrstr.new
