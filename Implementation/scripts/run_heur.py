import glob
import subprocess
from pathlib import Path


def main():
    input_file_names = sorted(glob.glob("../../TestScenarios/input/*.dat"))
    print(input_file_names)
    print(len(input_file_names))

    for input_file_name in input_file_names:
        name = Path(input_file_name).stem
        output_file_name = f"../../TestScenarios/output/heur-new/{name}.out"

        print(f"-----> {name} <-----")

        subprocess.run(args=[
            "../build/release/AerialResourceScheduler.exe",
            "-i", input_file_name,
            "-o", output_file_name,
            "-f", "ampl",
            "-t", "20",
            "--iters", "40"
        ], check=True)


if __name__ == '__main__':
    main()
