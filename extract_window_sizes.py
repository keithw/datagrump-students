
# extract window sizes from sender-stderr

import sys
from pylab import *



def extract_window_sizes(input_file, output_file):
    f = open(input_file)
    lines = f.readlines()
    f.close()
    g = open(output_file, 'w')

    time = ''
    window_size = ''
    is_initial_time = True
    initial_time = 0
    for line in lines:
        if 'window_size' in line:
            splt = line.split()
            time = int(splt[2][:-1]) - initial_time
            if is_initial_time:
                is_initial_time = False
                initial_time = time
                time = 0
            window_size = splt[6][:-1]
            g.write(str(time) + '\t' + window_size + '\n')

    g.close()


# input file has times and window sizes, tab-separated
def plot_window_size(input_file, output_file):
    f = open(input_file)
    lines = f.readlines()
    f.close()
    times = []
    sizes = []
    for line in lines:
        splt = line.split()
        times.append(splt[0])
        sizes.append(splt[1])

    np_arr_times = np.asfarray(times)
    np_arr_sizes = np.asfarray(sizes)
    scatter(np_arr_times, np_arr_sizes, s=1, lw=0)
    xlim(xmin=0)
    ylim(ymin=0)
    xlabel('time (ms)')
    ylabel('window size')
    title('Change of window size in time')
    savefig(output_file, dpi=72)



def run(filename):
    extract_window_sizes(filename, 'window_sizes.txt')
    plot_window_size('window_sizes.txt', 'window_sizes.png')


def main():
    run(sys.argv[1])
    


if __name__=="__main__":
    main()
