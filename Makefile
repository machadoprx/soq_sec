all:
	gcc -Ofast -pipe -flto -march=native -mtune=native -Wall main.c soq_sec.c MeekSpeak-Crypto/bn/bn.c MeekSpeak-Crypto/ecc/ecc_25519.c MeekSpeak-Crypto/hash/hash.c -o soq_sec