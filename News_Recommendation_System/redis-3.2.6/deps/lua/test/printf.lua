-- an implementation of print(f

function print(f(...)
 io.write(string.format(...))
end

print(f("Hello %s from %s on %s\n",os.getenv"USER" or "there",_VERSION,os.date())
