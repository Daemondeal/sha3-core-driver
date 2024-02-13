import os
import pathlib

def main():
    os.mkdir("./input_files")
    for response_file_path in os.listdir("./response_files"):
        if "Monte" in response_file_path:
            continue

        response_path = f"./response_files/{response_file_path}"
        input_path = "./input_files/" + response_file_path.split(".")[0] + ".in"

        extract_msg_from_resp(response_path, input_path)


def extract_msg_from_resp(resp_path: str, out_path: str):
    with open(resp_path, "r") as resp:
        with open(out_path, "w+") as out:
            for line in resp:
                line = line.lower().strip()
                if line.startswith("len = "):
                    out.write(line[len("len = "):] + " ")
                if line.startswith("msg = "):
                    if line == "msg = 00":
                        out.write("\n")
                    else:
                        out.write(line[len("msg = "):] + "\n")


if __name__ == "__main__":
    main()
