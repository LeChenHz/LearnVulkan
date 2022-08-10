from distutils.log import error
import optparse
import re
import os
import matplotlib.pyplot as plt

vsync_limit = 60
frametime_avg = 1
    
output_frametime_file = "frametime.png"
output_frametime_hist_file = "frametime_hist.png"

file_lists=[]
# Collect data
class frametime_set:
    def __init__(self):
        self.name = ""
        self.raw = []
        self.align_raw = []
        self.frametime = []
        self.frametime_avg = 0.0
        self.frametime_99th = 0.0
        self.frame = []
        self.duration = 0.0
        self.max_frametime = 0.0

frametime_sets=[]

def parse_option():
    global vsync_limit
    global frametime_avg
    opt_parser = optparse.OptionParser()
    opt_parser.add_option("-d", "--dir", dest='target_dir',
        help="Read all suitable files in DIR", metavar="DIR", type="string")
    opt_parser.add_option("-r", dest="target_dir_regex",
        help="The files in DIR with name matching the regex will be read", metavar="DIR_REGEX", type="string")
    opt_parser.add_option("-f", "--file", dest='target_file',
        help="Read files", metavar="FILE", type="string")
    opt_parser.add_option("-l", "--vsync-limit", dest='vsync_limit',
        help="Current vsync HZ", metavar="VSYNC", type="int")
    opt_parser.add_option("-a", "--avg", dest='frametime_avg',
        help="Average frames", metavar="AVG", type="int")

    (options, args) = opt_parser.parse_args()

    if options.target_dir and not options.target_dir_regex:
        opt_parser.error("If -d is specified, -r must be provided")
    if options.vsync_limit:
        vsync_limit = options.vsync_limit
    if options.frametime_avg:
        frametime_avg = options.frametime_avg
    if options.target_dir:
        for filename in os.listdir(options.target_dir):
            filename = os.path.join(options.target_dir, filename)
            if not os.path.isfile(filename):
                continue
            if not re.match(options.target_dir_regex, filename):
                continue
            file_lists.append(filename)
        
    if options.target_file:
        for filename in options.target_file.split(','):
            filename = os.path.abspath(filename)
            if not os.path.isfile(filename):
                continue
            file_lists.append(filename)

def fetch_data():
    for filename in file_lists:
        fts = frametime_set()
        fts.name = os.path.basename(filename).rsplit('.', maxsplit=1)[0]
        try:
            with open(filename) as f:
                fts.raw=[int(line.strip()) / 1000 for line in f]
        except:
            #print("File " + filename + " has invalid data, failed...")
            continue
        if len(fts.raw) < 2:
            #print("File " + filename + " has no data, failed...")
            continue
        first = fts.raw[0]
        align_raw = [int((a - first) * vsync_limit / 1000.0) * 1000 / vsync_limit for a in fts.raw]
        count = len(align_raw)
        for i in range(count):
            if i == 0 or align_raw[i] > align_raw[i - 1]:
                fts.align_raw.append(align_raw[i])
        
        fts.duration = fts.align_raw[-1]
        count = len(fts.align_raw)
        min_point = 1000
        cur_avg = 0.0
        cur_frame = fts.align_raw[0]
        raw_points = []
        for i in range(count - 1):
            point = fts.align_raw[i + 1] - fts.align_raw[i]
            raw_points.append(point)
            cur_avg = cur_avg + point
            if (i + 1) % frametime_avg == 0 or i == count - 2:
                cur_avg = cur_avg / float((i % frametime_avg) + 1)
                fts.frametime.append(cur_avg)
                fts.frame.append(cur_frame)
                cur_avg = 0.0
                cur_frame = fts.align_raw[i]
                if fts.max_frametime < fts.frametime[-1]:
                    fts.max_frametime = fts.frametime[-1]
                if min_point > fts.frametime[-1]:
                    min_point = fts.frametime[-1]
        
        raw_points.sort()
        fts.frametime_avg = sum(raw_points) / float(len(raw_points))
        fts.frametime_99th = raw_points[int(float(len(raw_points)) * 0.99)]
        frametime_sets.append(fts)

    if len(frametime_sets) == 0:
        error("No input data")
        exit(2)


# Ploting
def draw_frametime():
    size_scale = 1
    line_graph = plt.figure(num="frametime_graph", figsize=[11 * size_scale, 4.3 * size_scale])
    line_graph_ax = plt.subplot()
    max_duration = 0
    max_frametime = 0
    for fts in frametime_sets:
        if fts.max_frametime > max_frametime:
            max_frametime = fts.max_frametime
        line_graph_ax.step(fts.frame, fts.frametime, linewidth=1.0)
        if fts.duration > max_duration:
            max_duration = fts.duration 

    plt.legend([fts.name for fts in frametime_sets], loc="upper left")
        
    plot_max_frametime = max_frametime * 1.1
    if plot_max_frametime > 1000:
        plot_max_frametime = 1000
    line_graph_ax.set_xlim(left = 0, right = max_duration)
    line_graph_ax.set_ylim(bottom = 0, top = plot_max_frametime)
    line_graph_ax.set_xlabel("Time(ms)")
    line_graph_ax.set_ylabel("FrameTime(ms)")

    plt.savefig(output_frametime_file)
    #print("Done saving frametime graph to " + output_frametime_file)

# histogram
def draw_histogram(): 
    hist_bins = [i * 1000.0 / vsync_limit for i in range(vsync_limit + 1)]
    hist_bins.append(1100)
    fts_count = len(frametime_sets)
    rows = int((fts_count + 1) / 2)
    cols = 2
    (fig, ax) = plt.subplots(rows, cols, figsize=(12, 6))

    axs = []
    if rows > 1:
        for i in range(rows):
            axs.append(ax[i][0])
            axs.append(ax[i][1])
    else:
        axs.append(ax[0])
        axs.append(ax[1])
    i = 0
    for fts in frametime_sets:
        axs[i].hist(bins = hist_bins, x = [t if t <= 1000 else 1050 for t in fts.frametime], density = True)
        axs[i].set_xlabel("FrameTime Bin(ms)")
        axs[i].set_ylabel("Distribution")
        axs[i].set_title(fts.name)
        i = i + 1

    fig.tight_layout()
    plt.savefig(output_frametime_hist_file)
    #print("Done saving frametime hist graph to " + output_frametime_file)

def print_stats():
    for fts in frametime_sets:
        print(str(fts.frametime_avg) + " " +
            str(1000.0 / fts.frametime_avg) + " "
            str(fts.frametime_99th) + " "
            str(1000.0 / fts.frametime_99th))
        #print(fts.name + " Stats:")
        #print("    FrameTime Avg : " + str(fts.frametime_avg))
        #print("    FPS Avg       : " + str(1000.0 / fts.frametime_avg))
        #print("    FrameTime 99th: " + str(fts.frametime_99th))
        #print("    FPS 99th      : " + str(1000.0 / fts.frametime_99th))



if __name__ == "__main__":
    parse_option()
    fetch_data()
    draw_frametime()
    draw_histogram()
    print_stats()