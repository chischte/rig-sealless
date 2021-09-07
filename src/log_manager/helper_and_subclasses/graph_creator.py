from os import sep
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import csv
from datetime import datetime
import pandas.plotting._matplotlib #required for pyinstaller
from matplotlib.ticker import (MultipleLocator, AutoMinorLocator)

sns.set_style('darkgrid') # darkgrid, white grid, dark, white and ticks
sns.color_palette('deep')
plt.rc('axes', titlesize=14)     # fontsize of the axes title
plt.rc('axes', labelsize=9)    # fontsize of the x and y labels
plt.rc('xtick', labelsize=9)    # fontsize of the tick labels
plt.rc('ytick', labelsize=12)    # fontsize of the tick labels
plt.rc('legend', fontsize=12)    # legend fontsize
plt.rc('font', size=12)          # controls default text sizes

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

        # CREATE TICKS
        timestamp_array=[]
        index_array=[]
        index=0
        resolution=30
        counter=0
        print(df['TIMESTAMP'])
        for ts in df['TIMESTAMP']:
            if counter == 0:
                timestamp_array.append(ts)
                index_array.append(index)
                counter=resolution
            index +=1
            counter-=1

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
        df.plot(y=["TENSION FORCE","TENSION CURRENT","CRIMP CURRENT"])

        plt.title('LOGS SEALLES TESTRIG')
        plt.legend(labels=['F_Tens [N]', 'I_Tens [A]', 'I_Crimp [A]'])
        plt.xticks(index_array, timestamp_array,rotation=45)
        plt.tight_layout()
        plt.show()

if __name__ == '__main__':
    graph_creator=graph_creator()
    graph_creator.plot_graph()






