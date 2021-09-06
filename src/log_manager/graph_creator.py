import matplotlib.pyplot as plt
import pandas as pd

# READ CSV
df = pd.read_csv('logs.csv', delimiter=';')

# SET HEADER ROW
df.columns = df. iloc[4]

# SELECT DATA
df = df.iloc[5:]
print(df)
