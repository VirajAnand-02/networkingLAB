# XOR operation for binary strings
def xor_op(a, b):
    result = ""
    for i in range(1, len(b)):
        result += '0' if a[i] == b[i] else '1'
    return result

# Calculate CRC 
def calc_crc(msg, key):
    keyLen = len(key)
    appended = msg + '0' * (keyLen - 1)
    div = appended[0:keyLen]

    for i in range(keyLen, len(appended) + 1):
        if div[0] == '1':
            div = xor_op(div, key)
        else:
            div = xor_op(div, '0' * keyLen)
        if i < len(appended):
            div += appended[i]
    return div

# Calculate checksum
def calc_checksum(data, block_size):
    blocks = [data[i:i+block_size] for i in range(0, len(data), block_size)]

    # Pad last block if needed
    if len(blocks[-1]) < block_size:
        blocks[-1] = blocks[-1].ljust(block_size, '0')

    total = 0
    for block in blocks:
        total += int(block, 2)

    # Handle overflow (carry)
    while total >> block_size:
        total = (total & ((1 << block_size) - 1)) + (total >> block_size)

    checksum = bin(~total & ((1 << block_size) - 1))[2:].zfill(block_size)
    return checksum

# Flip a bit to simulate transmission error
def flip_bit(data, pos):
    if 0 <= pos < len(data):
        data_list = list(data)
        data_list[pos] = '0' if data[pos] == '1' else '1'
        return ''.join(data_list)
    return data

def main():
    print("Choose method:")
    print("1. Checksum")
    print("2. CRC")
    method = int(input("Enter choice (1 or 2): ").strip())

    data = input("\nEnter binary data: ").strip()

    if method == 1:
        block_size = int(input("Enter block size for checksum (e.g., 8, 16): "))
        checksum = calc_checksum(data, block_size)
        tx_data = data + checksum
        print("\nTransmitted Data (Data + Checksum):", tx_data)
    elif method == 2:
        poly = input("Enter generator polynomial (in binary): ").strip()
        crc = calc_crc(data, poly)
        tx_data = data + crc
        print("\nTransmitted Data (Data + CRC):", tx_data)
    else:
        print("Invalid choice!")
        return

    # Error simulation
    choice = input("\nIntroduce error in transmission? (y/n): ").strip()

    if choice.lower() == 'y':
        err_pos = int(input("Enter bit position to flip (0-based index): "))
        tx_data = flip_bit(tx_data, err_pos)
        print("Data after introducing error:", tx_data)

    # Receiver side - error detection
    print("\n--- Receiver Side ---")
    if method == 1:
        received_data = tx_data
        # Extract blocks
        blocks = [received_data[i:i+block_size] for i in range(0, len(received_data), block_size)]

        if len(blocks[-1]) < block_size:
            blocks[-1] = blocks[-1].ljust(block_size, '0')

        total = 0
        for block in blocks:
            total += int(block, 2)

        while total >> block_size:
            total = (total & ((1 << block_size) - 1)) + (total >> block_size)

        if total == (1 << block_size) - 1:
            print("No error detected at receiver.")
        else:
            print("Error detected at receiver!")
    else:
        received_crc = calc_crc(tx_data[:len(tx_data) - len(poly) + 1], poly)
        if received_crc == crc:
            print("No error detected at receiver.")
        else:
            print("Error detected at receiver!")

if __name__ == "__main__":
    main()
