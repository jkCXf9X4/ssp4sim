
rm -R ./results/*

cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/delay_con.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/explicit_delay_mod_con.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/explicit_delay_mod.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/implicit_delay_mod_con.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/implicit_delay_mod.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/no_delay.json

p3 ./resources/delay_sys/reference/compare.py