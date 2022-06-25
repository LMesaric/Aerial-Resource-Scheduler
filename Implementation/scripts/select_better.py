import glob
import shutil


def split_strip(s):
    return s.split('=')[1].strip()


def get_objective(filename):
    with open(filename) as f:
        return float(split_strip(f.readlines()[3]))


def main():
    files1 = sorted(glob.glob("../../TestScenarios/output/heur-v1/*.out"))
    files2 = sorted(glob.glob("../../TestScenarios/output/heur-v2/*.out"))
    out_dir = "../../TestScenarios/output/heur-better-v1-v2"
    print(len(files1), len(files2))

    for fn1, fn2 in zip(files1, files2):
        o1 = get_objective(fn1)
        o2 = get_objective(fn2)

        to_copy = fn1 if o1 >= o2 else fn2
        print(to_copy)
        shutil.copy2(to_copy, out_dir)

    return


if __name__ == '__main__':
    main()
