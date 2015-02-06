# Simple HTTP 1.0 Proxy

## Compilation
```bash
cd src
gcc proxy.c setup.c strlib.c -o proxy.exe
# TO Debug --> default port 12345
gcc proxy.c setup.c strlib.c -DDEBUG -o proxy.exe
```
## Run
```bash
./proxy.exe $PORTNUM
```
