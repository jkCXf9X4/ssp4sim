
rm -R ./results/*

# cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/explicit_delay_con.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/explicit_delay_mod_con.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/explicit_delay_mod_con_long.json
# cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/explicit_delay_mod.json
# cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/explicit_no_delay.json

# cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/implicit_delay_con.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/implicit_delay_mod_con.json
# cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/implicit_delay_mod.json
# cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/implicit_no_delay.json
cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/native.json


python3 ./resources/delay_sys/reference/compare.py

cp ./resources/delay_sys/reference/reference_system_native.csv ./results/reference_results.csv