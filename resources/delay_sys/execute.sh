
rm -R ./results/*

cmake --build build

# ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_jacobi_0.01ms.json
# ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_jacobi_1ms.json
# ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_jacobi_2ms.json
# ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_seidel_0.01ms.json
# ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_seidel_1ms.json
# ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_seidel_2ms.json

./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_jacobi_3ms.json
./build/public/ssp4sim_app/sim_app ./resources/delay_sys/1_seidel_3ms.json
./build/public/ssp4sim_app/sim_app ./resources/delay_sys/2_grouping_7ms.json
# ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/3_grouping_partial.json

cp ./resources/delay_sys/reference/reference_system_native.csv ./results/reference_results.csv

# python3 scripts/filter_results.py --window-size 3

python3 ./resources/delay_sys/reference/compare.py
