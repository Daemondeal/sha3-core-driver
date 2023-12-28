import hashlib
import os

test_strings = [
    b"Hello",
    b"Hello World!",
    b"Hi there",
    b"Very long message",
    b"TODO: ADD MORE MESSAGES",
    b"Did you ever hear the tragedy of Darth Plagueis The Wise? I thought not. It's not a story the Jedi would tell you. It's a Sith legend. Darth Plagueis was a Dark Lord of the Sith, so powerful and so wise he could use the Force to influence the midichlorians to create life... He had such a knowledge of the dark side that he could even keep the ones he cared about from dying. The dark side of the Force is a pathway to many abilities some consider to be unnatural. He became so powerful... the only thing he was afraid of was losing his power, which eventually, of course, he did. Unfortunately, he taught his apprentice everything he knew, then his apprentice killed him in his sleep. Ironic, he could save others from death, but not himself.",
]

hash_sizes = [
    ("512", hashlib.sha3_512),
    ("384", hashlib.sha3_384),
    ("256", hashlib.sha3_256),
    ("224", hashlib.sha3_224),
]

try:
    os.mkdir("./testvectors/")
except:
    pass

for hash_length, hash_func in hash_sizes:
    with open(f"./testvectors/{hash_length}.mem", "w") as outfile:
        for string in test_strings:
            digest = hash_func(string).hexdigest()
            outfile.write(f"{len(string)} {string.decode("utf-8")} {digest}\n")
        outfile.write("0")
