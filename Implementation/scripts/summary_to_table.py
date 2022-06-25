import itertools


def parse_line(line):
    p = line.strip().split()
    return (p[0],) + tuple(map(float, p[1:4]))


def main():
    with open("../../TestScenarios/output/heur-better-v1-v2/_summary.txt") as summary_file:
        lines = summary_file.readlines()
        parts = [parse_line(line) for line in lines]

    with open("../../TestScenarios/output/heur-better-v1-v2/_table.txt", "w") as output_file:
        parts = sorted(parts, key=lambda p: int(p[0][-6:-4]))
        for cf, g in itertools.groupby(parts, lambda p: p[0][-6:-4]):
            output_file.write(f"CF={cf}\n\n")

            for size, g1 in itertools.groupby(g, lambda p: p[0][:7]):
                g1_list = list(g1)

                for i in (2, 3, 1):
                    for distribution, g2 in itertools.groupby(g1_list, lambda p: p[0][8:-7]):
                        g2_list = list(g2)

                        temp = [r[i] for r in g2_list]
                        output_file.write(f"{sum(temp) / len(temp)} ")
                    output_file.write('\n')
            output_file.write('\n')


if __name__ == '__main__':
    main()
