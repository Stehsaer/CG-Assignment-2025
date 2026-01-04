import gzip
import argparse


def _compress_file(path: str) -> bytes:
    with open(path, "rb") as f:
        data: bytes = f.read()
    return gzip.compress(data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compress a file using gzip.")
    parser.add_argument("input", type=str, help="Path to the input file")
    parser.add_argument("output", type=str, help="Path to the output compressed file")
    args = parser.parse_args()
    input_path = args.input
    output_path = args.output

    compressed_data = _compress_file(input_path)
    with open(output_path, "wb") as f:
        f.write(compressed_data)
