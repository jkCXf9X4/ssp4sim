


from pathlib import Path


def main() -> None:
    script_dir = Path(__file__).resolve().parent
    print ( script_dir)

    variants = [
        "explicit_delay_con",
        "explicit_delay_mod_con",
        "explicit_delay_mod",
        "explicit_no_delay",
        "implicit_delay_con",
        "implicit_delay_mod_con",
        "implicit_delay_mod",
        "implicit_no_delay",]

    for v in variants:
        text = f"""
{{
    "simulation" :
    {{
        "ssp": "./resources/delay_sys/ssp_delay_fmi2",
        "ssd": "{v}.ssd",
        "start_time":0.0,
        "stop_time":0.1,
        "timestep": 0.001,
        "tolerance": 1e-4,

        "executor": "jacobi",
        "thread_pool_workers": 5,
        "forward_derivatives": false,
        "jacobi":
        {{
            "parallel": false,
            "method" : 1
        }},
        "seidel":
        {{
            "parallel": false
        }},
        
        "enable_recording": true,
        "log_file": "./results/{v}.log",
        "result_file": "./results/{v}.csv",
        "ensure_results": true,
        "result_interval": 0.001
    }}
}}
    """
        with open(script_dir / f"{v}.json", "w") as f:
            f.write(text)

        print(f"cmake --build build && ./build/public/ssp4sim_app/sim_app ./resources/delay_sys/{v}.json")

if __name__ == "__main__":
    main()
