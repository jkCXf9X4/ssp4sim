
rm -R ./results/*

cmake --build build

./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_native_0.01ms.json
./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_native_1ms.json
./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_native_2ms.json
./build/public/ssp4sim_app/sim_app ./resources/delay_sys/3_explicit_delay_1ms.json
./build/public/ssp4sim_app/sim_app ./resources/delay_sys/3_explicit_delay_2ms.json


./build/public/ssp4sim_app/sim_app ./resources/delay_sys/native_no_delay_1ms.json
./build/public/ssp4sim_app/sim_app ./resources/delay_sys/native_no_delay_2ms.json

cp ./resources/delay_sys/reference/reference_system_native.csv ./results/reference_results.csv

python3 ./resources/delay_sys/reference/compare.py
