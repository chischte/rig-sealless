import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
from datetime import datetime
import pandas.plotting._matplotlib  # required for pyinstaller


sns.set_style('darkgrid')  # darkgrid, white grid, dark, white and ticks

# https://matplotlib.org/stable/gallery/color/named_colors.html
# sns.set_style({"axes.facecolor": "black"})

# sns.color_palette('deep') # seems to have no effect
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

        pd.set_option("display.max.columns", None)

        df.head()
        # ASSIGN TYPES TO VALUES
        pd.to_datetime(df["TIMESTAMP"])
        df["CYCLES TOTAL"] = df["CYCLES TOTAL"].astype(float)
        df["CYCLES RESET"] = df["CYCLES RESET"].astype(float)
        df["TENSION FORCE"] = df["TENSION FORCE"].astype(float)
        df["TENSION CURRENT"] = df["TENSION CURRENT"].astype(float)
        df["CRIMP CURRENT"] = df["CRIMP CURRENT"].astype(float)
        df.plot(y=["TENSION FORCE", "TENSION CURRENT", "CRIMP CURRENT"],figsize=(15, 5))

        # CREATE TICKS
        ticks_timestamp_array = []
        ticks_index_array = []
        index = 0
        resolution = 30
        counter = 0
        print(df['TIMESTAMP'])
        for ts in df['TIMESTAMP']:
            if counter == 0:
                ticks_timestamp_array.append(ts)
                ticks_index_array.append(index)
                counter = resolution
            index += 1
            counter -= 1

        # CREATE TENSION FORCE VALUES
        f_tens_array = []
        for ts in df['TENSION FORCE']:
            f_tens_array.append(ts)

        # CREATE TENSION CURRENT VALUES
        i_tens_array = []
        for ts in df['TENSION CURRENT']:
            i_tens_array.append(ts)

        # CREATE CRIMP CURRENT VALUES
        i_crimp_array = []
        for ts in df['CRIMP CURRENT']:
            i_crimp_array.append(ts)

        # CREATE CRIMP CURRENT VALUES
        n_reset_count_array = []
        for ts in df['CYCLES RESET']:
            n_reset_count_array.append(ts)

        # CREATE X AXIS
        x_array = []
        timestamp_array = []
        x_index = 0
        for ts in df['TIMESTAMP']:
            timestamp_array.append(ts)
            x_array.append(x_index)
            x_index += 1

        fig, (ax1, ax2,ax3,ax4) = plt.subplots(4, 1, figsize=(14, 8))
        
        ax1.plot(x_array, f_tens_array)
        ax1.legend(['Tensioning Force [N]'])
        
          
        ax2.plot(x_array, i_tens_array)
        ax2.legend(['Tensioning Current [A]'])

        ax3.plot(x_array, i_crimp_array)
        ax3.legend(['Crimping Current [A]'])
        
        ax4.plot(x_array, n_reset_count_array)
        ax4.legend(['Cycle Count Reset'])

        # plt.xticks(ticks_index_array, ticks_timestamp_array, rotation=45)
        plt.tight_layout() # arrange graphs more compact
        plt.show()


if __name__ == '__main__':
    graph_creator = graph_creator()
    graph_creator.plot_graph()
