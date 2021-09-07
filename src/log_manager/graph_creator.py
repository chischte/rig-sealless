from os import sep
import matplotlib.pyplot as plt
import pandas as pd
import csv
from datetime import datetime
import pandas.plotting._matplotlib #required for pyinstaller

class graph_creator():

    def __init__(self):
        return

    def plot_graph(self):
        # CREATE DATAFRAOME FROM CSV
        df = pd.read_csv('logs.csv', delimiter=';')

        # # SET HEADER ROW
        df.columns = df. iloc[4]

        # # SELECT DATA
        df = df.iloc[6:]

        # # CREATE CSV FROM DATAFRAME
        # # df.to_csv('logs_processed.csv',sep=';')

        pd.set_option("display.max.columns", None)

        df.head()
        pd.to_datetime(df["TIMESTAMP"])
        df["CYCLES TOTAL"]=df["CYCLES TOTAL"].astype(float)
        df["CYCLES RESET"]=df["CYCLES RESET"].astype(float)
        df["TENSION FORCE"]=df["TENSION FORCE"].astype(float)
        df["TENSION CURRENT"]=df["TENSION CURRENT"].astype(float)
        df["CRIMP CURRENT"]=df["CRIMP CURRENT"].astype(float)

        df.plot(x="TIMESTAMP", y=["CYCLES RESET","TENSION FORCE","TENSION CURRENT","CRIMP CURRENT"])
        plt.xticks(rotation=90)
        plt.show()

if __name__ == '__main__':
    graph_creator=graph_creator()
    graph_creator.plot_graph()






