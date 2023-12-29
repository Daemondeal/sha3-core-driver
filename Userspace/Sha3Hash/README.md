# Sha3Hash

Small demo program that uses the Ketchup library to hash either one of its arguments, or `stdin`.

## Usage

```sh
sha3hash {512, 384, 256, 224} [input]
```

If `input` is given, it will hash that. Otherwise, it will accept data from the stdin until it finds an EOF, and then it returns that hash.

## Examples

```sh
> sha3hash 512 "Hello World"
3d58a719c6866b0214f96b0a67b37e51a91e233ce0be126a08f35fdf4c043c6126f40139bfbc338d44eb2a03de9f7bb8eff0ac260b3629811e389a5fbee8a894%
> sha3hash 256 < example.txt
d0e47486bbf4c16acac26f8b653592973c1362909f90262877089f9c8a4536af
```