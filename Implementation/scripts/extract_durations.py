import glob
from pathlib import Path


def main():
    files = sorted(glob.glob("../../TestScenarios/output/heur-v2/*.out"))
    print(files)
    print(len(files))

    with open("../../TestScenarios/output/heur-v2/_rawDurations.txt", "w") as summary_file:
        for filename in files:
            with open(filename) as f:
                name = Path(filename).stem
                durations = f.readlines()[21].split('=')[1].strip()
                summary_file.write(f"{name} {durations}\n")


if __name__ == '__main__':
    main()
