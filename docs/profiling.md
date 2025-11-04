
# Profiling

## Build profiling
https://github.com/nico/ninjatracing
Use ninjatracing to convert 


```
build/.ninja_log into 

ninjatracing build/.ninja_log > build/trace.json
```


ninja -C build clean

CCACHE_DISABLE=1 ninja -C build -d stats > build/build-stats.log

CCACHE_DISABLE=1 ninja -C build -v > build/commands.log

ninja -C build -t browse
## Runtime

```
sudo sysctl kernel.perf_event_paranoid=1


perf record -F 99 -g -o build/perf.data ./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json


perf report --dsos=sim_app  -i ./build/perf.data

```

self.OMS.setCommandLineOption("--emitEvents=false")

what does this oms flag do?


