
import pandas as pd
import matplotlib.pyplot as plt

df_ref = pd.read_csv(r'./../resources/embrace/ref_results/sim_results.csv')
df_res = pd.read_csv(r'./result.csv')


df_ref.columns = [x.replace("model.root.", "") for x in df_ref.columns]
df_ref = df_ref[df_ref.columns]

# print("df_res:")
# [print(x) for x in df_res.columns]
# print("df_ref:")
# [print(x) for x in df_ref.columns]

common_columns = [col for col in df_res.columns if col in df_ref.columns]

df_ref = df_ref[common_columns]
df_res = df_res[common_columns]

df_res = df_res.ffill()
df_res = df_res.sort_values("time")

# print(df_ref.head())
print(df_res.head())

df_ref.to_csv("./ref_common.csv")
df_res.to_csv("./res_common.csv")

# print(df_ref.columns)
# print(df_res.columns)