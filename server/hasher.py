global hash
hash = 0b01010101



def hash_chunk(data):
    for byte in data:
        global hash
        # perform XOR on byte with hash
        hash ^= byte
        print(f"Byte: {byte:08b} Hash: {hash:08b}")

if __name__ == "__main__":
    
    data = b"helloworldss"
    hash_chunk(data)
    print(f"Final Hash: {hash:08b}")