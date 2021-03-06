import argparse
import mirheo

def run(args):
    """The `mirheo run` command.

    Usage:
        run [--ranks NX NY NZ]
            ([--num-timesteps NUM]|[--num-timesteps-attr ATTR_NAME])
            snapshot_path
    """
    if bool(args.num_timesteps is not None) == bool(args.num_timesteps_attr):
        raise ValueError("Exactly one of --num-timesteps and --num-timesteps-attr must be set.")

    u = mirheo.Mirheo(args.ranks, snapshot=args.snapshot, debug_level=3, log_filename='log')
    if args.num_timesteps_attr:
        num_timesteps = int(u.getAttribute(args.num_timesteps_attr))
    else:
        num_timesteps = args.num_timesteps
    print(num_timesteps)
    u.run(num_timesteps)


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--version', action='store_true', default=False)

    subparsers = parser.add_subparsers(title="command", dest='command', help="sub-command help")

    runparser = subparsers.add_parser('run', help="Load and run a simulation from a given snapshot.")
    runparser.add_argument('--ranks', type=int, nargs=3, default=(1, 1, 1),
                           metavar=("NX", "NY", "NZ"), help="Number of ranks per dimension.")
    runparser.add_argument('--num-timesteps', type=int, metavar="NUM",
                           help="Number of timesteps.")
    runparser.add_argument('--num-timesteps-attr', type=str, metavar="ATTR_NAME",
                           help="Name of the attribute to read the number of timesteps from.")
    runparser.add_argument('snapshot', type=str, metavar="snapshot_path",
                           help="Run the snapshot at the given path.")
    args = parser.parse_args()

    if args.version:
        print(mirheo.version)
        return

    if args.command == 'run':
        run(args)


if __name__ == '__main__':
    import sys
    main(sys.argv[1:])
