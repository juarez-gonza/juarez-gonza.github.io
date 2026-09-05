import matplotlib.pyplot as plt

import argparse
import itertools as it


def plot_histogram(values, bins=20, title="Distribution", filename="histogram.png"):
    """
    Plot and save a histogram of the given values.

    Parameters
    ----------
    values : list or array-like
        Values to plot.
    bins : int
        Number of histogram bins.
    title : str
        Plot title.
    filename : str
        Path/name of the output image file.
    """
    plt.figure(figsize=(8, 5))

    plt.hist(values, bins=bins, edgecolor="black")

    plt.xlabel("Value")
    plt.ylabel("Frequency")
    plt.title(title)

    plt.grid(axis="y", alpha=0.3)

    # Save the figure
    plt.savefig(filename, dpi=300, bbox_inches="tight")

    # Close the figure
    plt.close()

def cantor_pair(x, y):
    return (0.5 * (x + y) * (x + y + 1) + y)

def szudzik_pair(x, y):
    return x * x + x + y if x >= y else y * y + x

def generate_pairs(pairing_function, x_upperbound, y_upperbound):
    return it.starmap(pairing_function, it.product(range(x_upperbound), range(y_upperbound)))

def main():
    parser = argparse.ArgumentParser(
        description="Generate pairing-function values and plot their distribution."
    )

    parser.add_argument(
        "--pairing",
        choices=["cantor", "szudzik"],
        required=True,
        help="Pairing function to use."
    )

    parser.add_argument(
        "--x-upper",
        type=int,
        required=True,
        help="Upper bound for x."
    )

    parser.add_argument(
        "--y-upper",
        type=int,
        required=True,
        help="Upper bound for y."
    )

    parser.add_argument(
        "--output",
        required=True,
        help="Output filename for the histogram."
    )

    parser.add_argument(
        "--bins",
        type=int,
        default=20,
        help="Number of histogram bins (default: 20)."
    )

    args = parser.parse_args()

    # Select pairing function
    pairing_functions = {
        "cantor": cantor_pair,
        "szudzik": szudzik_pair,
    }

    pairing_function = pairing_functions[args.pairing]

    # Generate paired values
    values = list(generate_pairs(
        pairing_function,
        args.x_upper,
        args.y_upper
    ))

    # Plot distribution
    plot_histogram(
        values,
        bins=args.bins,
        title=f"{args.pairing.capitalize()} Pairing Distribution",
        filename=args.output
    )

if __name__ == "__main__":
    main()
