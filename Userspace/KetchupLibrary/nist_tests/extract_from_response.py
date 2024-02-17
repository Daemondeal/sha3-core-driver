import os

def main():

    try:
        os.mkdir("./input_files")
    except:
        pass

    for response_file_path in os.listdir("./response_files"):


        response_path = f"./response_files/{response_file_path}"
        input_path = "./input_files/" + response_file_path.split(".")[0] + ".in"

        if "Monte" in response_file_path:
            extract_seed_from_monte_resp(response_path, input_path)
        else:
            extract_msg_from_resp(response_path, input_path)

def extract_seed_from_monte_resp(resp_path: str, out_path: str):
    with open(resp_path, "r") as resp:
        with open(out_path, "w+") as out:
            for line in resp:
                line = line.lower().strip()

                if line.startswith("[l = "):
                    start = len("[l = ")
                    out.write(line[start:start+3] + "\n")
                if line.startswith("seed = "):
                    start = len("seed = ")
                    out.write(line[start:] + "\n")


def extract_msg_from_resp(resp_path: str, out_path: str):
    with open(resp_path, "r") as resp:
        with open(out_path, "w+") as out:
            for line in resp:
                line = line.lower().strip()

                if line.startswith("[l = "):
                    start = len("[l = ")
                    out.write(line[start:start+3] + "\n")
                if line.startswith("len = "):
                    out.write(line[len("len = "):] + " ")
                if line.startswith("msg = "):
                    if line == "msg = 00":
                        out.write("\n")
                    else:
                        out.write(line[len("msg = "):] + "\n")


if __name__ == "__main__":
    main()
