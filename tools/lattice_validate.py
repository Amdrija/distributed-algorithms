#!/usr/bin/env python3

import argparse
import os
import sys
from pathlib import Path
from typing import Generator


class Config:
    def __init__(self, file_name, p, vs, ds, proposals):
        self.file_name = file_name
        self.p = p
        self.vs = vs
        self.ds = ds
        self.proposals = proposals

    @staticmethod
    def from_file(path):
        with open(path, "r") as f:
            lines = [l.rstrip('\n') for l in f.readlines()]
            header, *proposals = lines
            p, vs, ds = map(int, header.split())
            return Config(
                str(path),
                p,
                vs,
                ds,
                [list(map(int, x.split())) for x in proposals],
            )


class Output:
    def __init__(self, file_name, decide_sets):
        self.file_name = file_name
        self.decide_sets = decide_sets

    @staticmethod
    def from_file(path):
        with open(path, "r") as f:
            lines = [l.rstrip('\n') for l in f.readlines()]
            return Output(str(path), [list(map(int, x.split())) for x in lines])

def file_exists(value: str) -> Path:
    path = Path(value)
    if not path.exists():
        raise argparse.ArgumentTypeError(f"`{value}` does not exist")
    if not path.is_file():
        raise argparse.ArgumentTypeError(f"`{value}` is not a file")
    return path


def zip_union(p, items):
    for i in range(p):
        values: set[int] = set()
        for j in range(len(items)):
            values |= set(items[j][i])
        yield values


def check_configs(configs):
    params = set((x.p, x.vs, x.ds) for x in configs)
    if len(params) > 1:
        raise SyntaxError("Config files do not have the same header")

    p, vs, ds = params.pop()

    for config in configs:
        if len(config.proposals) != p:
            raise SyntaxError(
                f"Config file {config.file_name}: the amount of proposals is not equal to `p`"
            )

    for config in configs:
        for n, proposal in enumerate(config.proposals):
            if len(proposal) > vs:
                raise SyntaxError(
                    f"Config file {config.file_name}, proposal nr {n+1}: the amount of values exceeds `vs`"
                )

    for n, proposal in enumerate(zip_union(p, [x.proposals for x in configs])):
        if len(proposal) > ds:
            raise SyntaxError(
                f"Proposal nr {n+1}: there are over `ds` unique values among all processes"
            )


def check_outputs(configs, outputs):
    p = configs[0].p

    for output in outputs:
        # property: we decide not more times than the amount of agreements
        if len(output.decide_sets) > p:
            raise SyntaxError(
                f"Output file {output.file_name}: there are more decide sets than proposals"
            )

        for n, decided_set in enumerate(output.decide_sets):
            # property: we decide unique values
            if len(decided_set) != len(set(decided_set)):
                raise SyntaxError(
                    f"Output file {output.file_name}, agreement nr {n+1}: the decided set contains duplicate values"
                )

    unique = list(zip_union(p, [x.proposals for x in configs]))

    for config, output in zip(configs, outputs):
        for n, (proposed, decided) in enumerate(
            zip(config.proposals, output.decide_sets)
        ):
            # property: self-validity I ⊆ O
            if not set(proposed).issubset(set(decided)):
                raise SyntaxError(
                    f"Output file {output.file_name}, agreement nr {n+1}: the decided set is not a superset of the proposed set of this process"
                )
            # property: global-validity O ⊆ union(I_j)
            if not set(decided).issubset(unique[n]):
                raise SyntaxError(
                    f"Output file {output.file_name}, agreement nr {n+1}: the decided set is not a subset of the union of all proposed sets"
                )

    for output1 in outputs:
        for output2 in outputs:
            for n, (decided1, decided2) in enumerate(
                zip(output1.decide_sets, output2.decide_sets)
            ):
                # property: consistency O1 ⊆ O2 or O2 ⊆ O1
                if not (
                    set(decided1).issubset(set(decided2))
                    or set(decided2).issubset(set(decided1))
                ):
                    raise SyntaxError(
                        f"Output files {output1.file_name} and {output2.file_name}, agreement nr {n+1}: none of the decided sets are a subset of each other"
                    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Passed config and output files have to be in respective order of a process"
    )

    parser.add_argument(
        "-p",
        "--processes",
        required=True,
        type=int,
        dest="processes",
        help="NUmber of processes"
    )

    parser.add_argument(
        "-c",
        "--configs",
        required=True,
        type=str,
        dest="configs",
        help="The directory for config files",
    )
    parser.add_argument(
        "-o",
        "--outputs",
        required=True,
        type=str,
        dest="outputs",
        help="The directory for output files",
    )

    results = parser.parse_args()

    outputs = []
    configs = []
    for i in range(1, results.processes + 1):
        proc = "proc{:02d}".format(i)
        output_file = os.path.realpath("{}/{}.output".format(results.outputs, proc))
        outputs.append(Output.from_file(output_file))

        config_file = os.path.realpath("{}/{}.config".format(results.configs, proc))
        configs.append(Config.from_file(config_file))

    try:
        check_configs(configs)
        check_outputs(configs, outputs)
    except SyntaxError as err:
        print(err, file=sys.stderr)
        exit(1)