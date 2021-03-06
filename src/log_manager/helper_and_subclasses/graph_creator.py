import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
from datetime import datetime
import os
from seaborn.palettes import color_palette  # required for pyinstaller

sns.set_style('darkgrid')  # darkgrid, white grid, dark, white and ticks
# plt.style.use(['dark_background'])

# https://matplotlib.org/stable/gallery/color/named_colors.html

# sns.color_palette('deep') # seems to have no effect

# https://matplotlib.org/stable/api/matplotlib_configuration_api.html#matplotlib.rcParams
plt.rc('axes', titlesize=14)     # fontsize of the axes title
plt.rc('axes', labelsize=9)    # fontsize of the x and y labels
plt.rc('xtick', labelsize=8)    # fontsize of the tick labels
plt.rc('ytick', labelsize=8)    # fontsize of the tick labels
plt.rc('legend', fontsize=12)    # legend fontsize
plt.rc('font', size=12)          # controls default text sizes
plt.rc('lines', linewidth=1.7)

plt.rc('axes', facecolor='#2e2e2e')
plt.rc('text', color='white')
# plt.rc('lines', color='white')
# plt.rc('axes', grid='true')
# plt.rc('grid', color='white')
# plt.rc('xtick', color='white')
# plt.rc('ytick', color='white')


class graph_creator():

    def __init__(self):
        return

    def get_user_path(self):
        return os.environ['USERPROFILE']

    def get_filepath(self):
        return self.get_user_path()+"/AppData/Roaming/SeallessLog/Logs.csv"

    def plot_graph(self):
        # CREATE DATAFRAOME FROM CSV
        df = pd.read_csv(self.get_filepath(), delimiter=';')

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

        x_offset = 1032

        # CREATE TENSION FORCE VALUES
        f_tens_array = []

        for x in range(x_offset):
            f_tens_array.append(0)

        for ts in df['TENSION FORCE']:
            f_tens_array.append(ts)

        # CREATE TENSION CURRENT VALUES

        i_tens_array = []

        for x in range(x_offset):
            i_tens_array.append(0)

        for ts in df['TENSION CURRENT']:
            i_tens_array.append(ts)

        # CREATE CRIMP CURRENT VALUES
        i_crimp_array = []

        for x in range(x_offset):
            i_crimp_array.append(0)

        for ts in df['CRIMP CURRENT']:
            i_crimp_array.append(ts)

        # CREATE CYCLES RESET VALUES
        n_reset_count_array = []

        for x in range(x_offset):
            n_reset_count_array.append(0)

        for ts in df['CYCLES RESET']:
            n_reset_count_array.append(ts)

        # CREATE CYCLES TOTAL VALUES
        n_total_count_array = []

        for x in range(x_offset):
            n_total_count_array.append(0)

        for ts in df['CYCLES TOTAL']:
            n_total_count_array.append(ts)

        # CREATE TICKS
        ticks_timestamp_array = []
        ticks_index_array = []
        index = 0
        resolution = 1000
        counter = 0
        # for ts in range(10000):
        for ts in df['TIMESTAMP']:
            if counter == 0:
                ticks_timestamp_array.append(ts)
                ticks_index_array.append(index)
                counter = resolution
            index += 1
            counter -= 1

        # CREATE PLOTS

        col_off_black = '#2e2e2e'
        col_turkis = '#2dbda8'
        col_green = '#84b647'
        col_yellow = '#d9a322'
        col_orange = '#e76d3b'
        col_red = '#cc3e4a'
        col_pink = '#d96eae'
        col_purple = '#7049a3'

        fig, (ax_f_tens, ax_i_tens, ax_i_crimp, ax_n_count, ax_n_tot_count) = plt.subplots(5, 1, figsize=(14, 8))

        ax_f_tens.plot(f_tens_array, color=col_green)
        ax_f_tens.legend(['Tensioning Force [N]'], loc='upper center')
        ax_f_tens.set_xlim(left=0)
        # ax_f_tens.set_xticks([])
        # ax_f_tens.set_xticks(ticks_index_array)
        ax_f_tens.set_ylim(bottom=0, top=6000)

        ax_i_tens.plot(i_tens_array, color=col_yellow)
        ax_i_tens.legend(['Tensioning Current [A]'], loc='upper center')
        ax_i_tens.set_xlim(left=0)
        # ax_i_tens.set_xticks([])
        # ax_i_tens.set_xticks(ticks_index_array)
        ax_i_tens.set_ylim(bottom=15, top=65)
        # ax_i_tens.set_yticks([20, 30, 40, 50, 60])

        ax_i_crimp.plot(i_crimp_array, color=col_orange)
        # ax_i_crimp.plot(i_crimp_array)
        ax_i_crimp.legend(['Crimping Current [A]'], loc='upper center')
        ax_i_crimp.set_xlim(left=0)
        # ax_i_crimp.set_xticks([])
        # ax_i_crimp.set_xticks(ticks_index_array)
        ax_i_crimp.set_ylim(bottom=15, top=65)
        # ax_i_crimp.set_yticks([20, 30, 40, 50, 60])

        ax_n_count.plot(n_reset_count_array, color=col_purple)
        ax_n_count.legend(['Cycles Reset'], loc='upper center')
        ax_n_count.set_xlim(left=0)
        # ax_n_count.set_xticks([])
        # ax_n_count.set_xticks(ticks_index_array)

        ax_n_tot_count.plot(n_total_count_array, color=col_yellow)
        ax_n_tot_count.legend(['Cycles Total'], loc='upper center')
        ax_n_tot_count.set_xlim(left=0)
        # ax_n_tot_count.set_xticks([])
        # ax_n_tot_count.set_xticks(ticks_index_array)
        # ax_n_tot_count.set_xticklabels(ticks_timestamp_array, rotation=90)

        plt.tight_layout()  # arrange graphs more compact
        plt.subplots_adjust(hspace=0.16)  # even more compact
        plt.show()


if __name__ == '__main__':
    graph_creator = graph_creator()
    graph_creator.plot_graph()
