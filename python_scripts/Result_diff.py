import pandas as pd
import numpy as np

file1 = "../output/HA_Marabou_1.00.csv"
file2 = "../output/HA_reverse_Marabou_1.00.csv"
features = ["age","sex","cp","trtbps","chol","fbs","restecg","thalachh","exng","oldpeak","slp","caa","thall"]
#
data1 = pd.read_csv(file1, header=0).dropna(axis=1)

datapoints1 = data1["datapoint"]
exp1 = data1[features]
exp1_size = exp1.sum(axis=1)
diff_sum = exp1.abs().sum()
print("regular order:")
print(diff_sum)

data2 = pd.read_csv(file2).dropna(axis=1)
datapoints2 = data2["datapoint"]
exp2 = data2[features]
exp2_size = exp2.sum(axis=1)
diff_sum = exp2.abs().sum()
print("reverse order:")
print(diff_sum)

out_df = pd.DataFrame()
out_df["datapoint"] = datapoints1
out_df["exp1_size"] = exp1_size
out_df["exp2_size"] = exp2_size

diffs_val = exp1 - exp2
# append  scalar_product to diff
diff = diffs_val.abs().sum(axis=1)
out_df["diff"] = diff

# append diffs_val to out_df
out_df = pd.concat([out_df, diffs_val], axis=1)

# compute the sum of each column in features
data_diff = out_df[features]
diff_sum = data_diff.abs().sum()
print(diff_sum)


diff_file_name = "../output/diff_order" + file1.split("/")[-1].split("_")[1] + file1.split("/")[-1].split("_")[2]
out_df.to_csv(diff_file_name, index=False)

