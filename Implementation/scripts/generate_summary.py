import glob
from pathlib import Path


def split_strip(s):
    return s.split('=')[1].strip()


def heuristic(lines, file, name):
    wo = split_strip(lines[0])
    wsn = split_strip(lines[1])
    z = split_strip(lines[2])
    obj = split_strip(lines[3])
    wall_time = split_strip(lines[4])
    best_iter = split_strip(lines[7])
    time_iter_avg = split_strip(lines[15])
    time_iter_dev = split_strip(lines[16])
    time_greedy_avg = split_strip(lines[17])
    time_greedy_dev = split_strip(lines[18])
    time_ls_avg = split_strip(lines[19])
    time_ls_dev = split_strip(lines[20])

    file.write(f"{name} {wo} {wsn} {z} {obj} {wall_time}"
               # f" {best_iter}"
               # f" {time_iter_avg} {time_iter_dev}"
               # f" {time_greedy_avg} {time_greedy_dev}"
               # f" {time_ls_avg} {time_ls_dev}"
               "\n")


def ampl(lines, file, name):
    wo = split_strip(lines[0])
    wsn = -float(split_strip(lines[2])) + 0.0
    zp = float(split_strip(lines[4]))
    zn = float(split_strip(lines[6]))
    z = zp - zn
    cpu_time = split_strip(lines[8])
    gap = lines[14].strip()

    file.write(f"{name} {wo} {wsn} {z} {cpu_time} {gap}\n")


def main():
    files = sorted(glob.glob("../../TestScenarios/output/heur-better-v1-v2/*.out"))
    current_kf = "KXX_FYY"
    print(files)
    print(len(files))

    with open("../../TestScenarios/output/heur-better-v1-v2/_summary.txt", "w") as summary_file:
        for filename in files:
            with open(filename) as f:
                name = Path(filename).stem
                kf = name[:7]

                if kf != current_kf:
                    # summary_file.write(f"{kf}\n")
                    current_kf = kf

                lines = f.readlines()

                heuristic(lines, summary_file, name)
                # ampl(lines, summary_file, name)


if __name__ == '__main__':
    main()
